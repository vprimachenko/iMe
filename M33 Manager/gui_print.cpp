#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include "gui.h"

namespace {

bool printJobBlocksUiState(PrintJobState state) {
	return state == PRINT_JOB_RUNNING || state == PRINT_JOB_PAUSED || state == PRINT_JOB_STOPPING;
}

int extractTrackedNozzleTarget(const Gcode &gcode) {
	if(!gcode.hasValue('M'))
		return 0;

	const string mValue = gcode.getValue('M');
	if(mValue != "104" && mValue != "109")
		return 0;

	if(!gcode.hasValue('S'))
		return 0;

	try {
		const int targetTemperature = stoi(gcode.getValue('S'));
		return targetTemperature > 0 ? targetTemperature : 0;
	}
	catch(...) {
		return 0;
	}
}

string describePrintPhase(const string &command) {
	if(command.rfind("M109", 0) == 0 || command.rfind("M104", 0) == 0)
		return "Heating the nozzle...";

	if(command.rfind("G28", 0) == 0)
		return "Homing...";

	if(command.rfind("G92", 0) == 0)
		return "Zeroing the extruder...";

	if(command.rfind("M106", 0) == 0 || command.rfind("M107", 0) == 0)
		return "Adjusting cooling...";

	if(command.rfind("M104 S0", 0) == 0)
		return "Cooling down...";

	if(command.rfind("M18", 0) == 0)
		return "Finishing...";

	if(command.rfind("G0", 0) == 0 || command.rfind("G1", 0) == 0) {
		const bool movesXY = command.find('X') != string::npos || command.find('Y') != string::npos;
		const bool movesZ = command.find('Z') != string::npos;
		const bool extrudes = command.find('E') != string::npos;

		if(extrudes && !movesXY && !movesZ)
			return "Priming the nozzle...";

		if(extrudes)
			return "Printing...";

		if(movesZ && !movesXY)
			return "Adjusting Z height...";

		if(movesXY || movesZ)
			return "Positioning the print head...";
	}

	return "Sending printer command...";
}

}

void MyFrame::loadPrintFile() {
	wxFileDialog openFileDialog(this, "Open G-code file", wxEmptyString, wxEmptyString, "G-code files|*.gcode;*.gc;*.nc;*.txt|All files|*", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	if(openFileDialog.ShowModal() != wxID_OK)
		return;

	PreparedPrintJob job = workflows.preparePrintJob(static_cast<string>(openFileDialog.GetPath()));
	preparedPrintJob = job;
	hasTrackedPrintNozzleTarget = false;
	lastPrintNozzleTargetC = 0;
	printPauseStandbyApplied = false;

	PrintJobStatus status;
	status.filePath = job.filePath;
	status.totalCommands = job.acceptedCommandCount;
	status.currentCommandIndex = 0;
	status.summaryText = job.summaryText;

	if(job.valid) {
		status.state = PRINT_JOB_LOADED;
		status.statusText = "Ready to print";
		applyPrintJobStatus(status);
	}

	else {
		status.state = PRINT_JOB_FAILED;
		status.statusText = "Failed to load print file";
		status.errorText = job.errorText;
		applyPrintJobStatus(status);
		wxMessageBox(job.errorText, "M33 Manager", wxOK | wxICON_ERROR | wxCENTRE);
	}
}

void MyFrame::startPrintJob() {
	if(!printer.isConnected() || !preparedPrintJob.valid || preparedPrintJob.acceptedCommandCount == 0)
		return;

	{
		wxCriticalSectionLocker lock(criticalLock);
		printPauseRequested = false;
		printStopRequested = false;
		printPauseStandbyApplied = false;
		hasTrackedPrintNozzleTarget = false;
		lastPrintNozzleTargetC = 0;
	}

	PrintJobStatus status = printJobStatus;
	status.filePath = preparedPrintJob.filePath;
	status.totalCommands = preparedPrintJob.acceptedCommandCount;
	status.summaryText = preparedPrintJob.summaryText;
	status.errorText.clear();
	status.statusText = "Printing";
	if(printJobStatus.state == PRINT_JOB_COMPLETED || printJobStatus.state == PRINT_JOB_FAILED)
		status.currentCommandIndex = 0;
	if(status.currentCommandIndex >= status.totalCommands)
		status.currentCommandIndex = 0;
	status.state = PRINT_JOB_RUNNING;
	applyPrintJobStatus(status);

	statusTimer->Stop();
	allowEnablingControls = false;

	enqueueBackgroundTask(nullptr, [=]() -> ThreadTaskResponse {
		PreparedPrintJob job;
		size_t startIndex = 0;
		{
			wxCriticalSectionLocker taskLock(criticalLock);
			job = preparedPrintJob;
			startIndex = printJobStatus.currentCommandIndex;
		}

		for(size_t i = startIndex; i < job.printableCommands.size(); i++) {
			while(true) {
				bool stopRequested = false;
				bool pauseRequested = false;
				bool standbyApplied = false;
				{
					wxCriticalSectionLocker taskLock(criticalLock);
					stopRequested = printStopRequested;
					pauseRequested = printPauseRequested;
					standbyApplied = printPauseStandbyApplied;
				}

				if(stopRequested)
					break;
				if(!pauseRequested)
					break;

				if(!standbyApplied) {
					const int standbyTemperature = getPauseStandbyTemperatureC();
					PrintJobStatus pauseTransitionStatus;
					pauseTransitionStatus.filePath = job.filePath;
					pauseTransitionStatus.totalCommands = job.acceptedCommandCount;
					pauseTransitionStatus.currentCommandIndex = i;
					pauseTransitionStatus.summaryText = job.summaryText;
					pauseTransitionStatus.statusText = "Pausing and cooling nozzle to " + to_string(standbyTemperature) + "C...";
					pauseTransitionStatus.state = PRINT_JOB_PAUSED;
					{
						wxCriticalSectionLocker taskLock(criticalLock);
						printJobStatus = pauseTransitionStatus;
						printJobUpdateQueue.push(pauseTransitionStatus);
					}

					ThreadTaskResponse standbyResponse = workflows.setNozzleStandbyTemperature(standbyTemperature, [=](const string &message) -> void {
						logToConsole(message);
					});
					if(standbyResponse.style & wxICON_ERROR) {
						PrintJobStatus failedStatus;
						failedStatus.state = PRINT_JOB_FAILED;
						failedStatus.filePath = job.filePath;
						failedStatus.totalCommands = job.acceptedCommandCount;
						failedStatus.currentCommandIndex = i;
						failedStatus.summaryText = job.summaryText;
						failedStatus.statusText = "Print failed";
						failedStatus.errorText = standbyResponse.message.empty() ? "Failed to cool the nozzle for pause" : standbyResponse.message;
						{
							wxCriticalSectionLocker taskLock(criticalLock);
							printJobStatus = failedStatus;
							printJobUpdateQueue.push(failedStatus);
						}
						return standbyResponse;
					}

					PrintJobStatus pausedStatus;
					pausedStatus.filePath = job.filePath;
					pausedStatus.totalCommands = job.acceptedCommandCount;
					pausedStatus.currentCommandIndex = i;
					pausedStatus.summaryText = job.summaryText;
					pausedStatus.statusText = "Paused at " + to_string(standbyTemperature) + "C";
					pausedStatus.state = PRINT_JOB_PAUSED;
					{
						wxCriticalSectionLocker taskLock(criticalLock);
						printPauseStandbyApplied = true;
						printJobStatus = pausedStatus;
						printJobUpdateQueue.push(pausedStatus);
					}
				}
				sleepUs(50000);
			}

			{
				wxCriticalSectionLocker taskLock(criticalLock);
				if(printStopRequested)
					break;
			}

			bool reheatNeeded = false;
			int resumeTargetTemperature = 0;
			{
				wxCriticalSectionLocker taskLock(criticalLock);
				reheatNeeded = printPauseStandbyApplied && hasTrackedPrintNozzleTarget;
				resumeTargetTemperature = lastPrintNozzleTargetC;
			}
			if(reheatNeeded) {
				PrintJobStatus reheatStatus;
				reheatStatus.filePath = job.filePath;
				reheatStatus.totalCommands = job.acceptedCommandCount;
				reheatStatus.currentCommandIndex = i;
				reheatStatus.summaryText = job.summaryText;
				reheatStatus.statusText = "Reheating nozzle to " + to_string(resumeTargetTemperature) + "C...";
				reheatStatus.state = PRINT_JOB_RUNNING;
				{
					wxCriticalSectionLocker taskLock(criticalLock);
					printJobStatus = reheatStatus;
					printJobUpdateQueue.push(reheatStatus);
				}

				ThreadTaskResponse reheatResponse = workflows.reheatNozzleForResume(resumeTargetTemperature, [=](const string &message) -> void {
					logToConsole(message);
				});
				if(reheatResponse.style & wxICON_ERROR) {
					PrintJobStatus failedStatus;
					failedStatus.state = PRINT_JOB_FAILED;
					failedStatus.filePath = job.filePath;
					failedStatus.totalCommands = job.acceptedCommandCount;
					failedStatus.currentCommandIndex = i;
					failedStatus.summaryText = job.summaryText;
					failedStatus.statusText = "Print failed";
					failedStatus.errorText = reheatResponse.message.empty() ? "Failed to reheat the nozzle before resuming" : reheatResponse.message;
					{
						wxCriticalSectionLocker taskLock(criticalLock);
						printJobStatus = failedStatus;
						printJobUpdateQueue.push(failedStatus);
					}
					return reheatResponse;
				}

				{
					wxCriticalSectionLocker taskLock(criticalLock);
					printPauseStandbyApplied = false;
				}
			}
			else {
				wxCriticalSectionLocker taskLock(criticalLock);
				printPauseStandbyApplied = false;
			}

			PrintJobStatus phaseStatus;
			phaseStatus.filePath = job.filePath;
			phaseStatus.totalCommands = job.acceptedCommandCount;
			phaseStatus.currentCommandIndex = i;
			phaseStatus.summaryText = job.summaryText;
			phaseStatus.statusText = (i < job.statusMessages.size() && !job.statusMessages[i].empty()) ? job.statusMessages[i] : describePrintPhase(job.printableCommands[i]);
			phaseStatus.state = PRINT_JOB_RUNNING;
			{
				wxCriticalSectionLocker taskLock(criticalLock);
				printJobStatus = phaseStatus;
				printJobUpdateQueue.push(phaseStatus);
			}

			ThreadTaskResponse commandResponse = workflows.executePreparedPrintCommand(job.printableCommands[i], [=](const string &message) -> void {
				logToConsole(message);
			});
			if(commandResponse.style & wxICON_ERROR) {
				PrintJobStatus failedStatus;
				failedStatus.state = PRINT_JOB_FAILED;
				failedStatus.filePath = job.filePath;
				failedStatus.totalCommands = job.acceptedCommandCount;
				failedStatus.currentCommandIndex = i;
				failedStatus.summaryText = job.summaryText;
				failedStatus.statusText = "Print failed";
				failedStatus.errorText = commandResponse.message.empty() ? "Printer command failed during print" : commandResponse.message;
				{
					wxCriticalSectionLocker taskLock(criticalLock);
					printJobStatus = failedStatus;
					printJobUpdateQueue.push(failedStatus);
				}
				return commandResponse;
			}

			const int trackedNozzleTarget = extractTrackedNozzleTarget(job.parsedCommands[i]);
			if(trackedNozzleTarget > 0) {
				wxCriticalSectionLocker taskLock(criticalLock);
				hasTrackedPrintNozzleTarget = true;
				lastPrintNozzleTargetC = trackedNozzleTarget;
			}

			if(!printer.isConnected()) {
				PrintJobStatus failedStatus;
				failedStatus.state = PRINT_JOB_FAILED;
				failedStatus.filePath = job.filePath;
				failedStatus.totalCommands = job.acceptedCommandCount;
				failedStatus.currentCommandIndex = i;
				failedStatus.summaryText = job.summaryText;
				failedStatus.statusText = "Print failed";
				failedStatus.errorText = "Printer disconnected during print";
				{
					wxCriticalSectionLocker taskLock(criticalLock);
					printJobStatus = failedStatus;
					printJobUpdateQueue.push(failedStatus);
				}
				return {"Printer disconnected during print", wxOK | wxICON_ERROR | wxCENTRE};
			}

			PrintJobStatus progressStatus;
			progressStatus.filePath = job.filePath;
			progressStatus.totalCommands = job.acceptedCommandCount;
			progressStatus.currentCommandIndex = i + 1;
			progressStatus.summaryText = job.summaryText;
			progressStatus.statusText = phaseStatus.statusText;
			progressStatus.state = printPauseRequested ? PRINT_JOB_PAUSED : PRINT_JOB_RUNNING;
			{
				wxCriticalSectionLocker taskLock(criticalLock);
				printJobStatus = progressStatus;
				printJobUpdateQueue.push(progressStatus);
			}
		}

		PrintJobStatus finalStatus;
		ThreadTaskResponse response = {"Print completed", wxOK | wxICON_INFORMATION | wxCENTRE};
		finalStatus.filePath = job.filePath;
		finalStatus.totalCommands = job.acceptedCommandCount;
		finalStatus.summaryText = job.summaryText;

		{
			wxCriticalSectionLocker taskLock(criticalLock);
			if(printStopRequested) {
				if(printer.isConnected()) {
					PrintJobStatus stopStatus;
					stopStatus.filePath = job.filePath;
					stopStatus.totalCommands = job.acceptedCommandCount;
					stopStatus.currentCommandIndex = printJobStatus.currentCommandIndex;
					stopStatus.summaryText = job.summaryText;
					stopStatus.statusText = "Stopping and cooling nozzle...";
					stopStatus.state = PRINT_JOB_STOPPING;
					printJobStatus = stopStatus;
					printJobUpdateQueue.push(stopStatus);
				}
			}
		}

		if(printStopRequested && printer.isConnected()) {
			ThreadTaskResponse cooldownResponse = workflows.coolDownNozzle([=](const string &message) -> void {
				logToConsole(message);
			});
			if(cooldownResponse.style & wxICON_ERROR && response.message.empty())
				response = cooldownResponse;
		}

		{
			wxCriticalSectionLocker taskLock(criticalLock);
			if(printStopRequested) {
				finalStatus.state = PRINT_JOB_LOADED;
				finalStatus.currentCommandIndex = 0;
				finalStatus.statusText = "Ready to print";
				response = {"Print stopped", wxOK | wxICON_INFORMATION | wxCENTRE};
			}
			else {
				finalStatus.state = PRINT_JOB_COMPLETED;
				finalStatus.currentCommandIndex = job.acceptedCommandCount;
				finalStatus.statusText = "Print completed";
			}

			printJobStatus = finalStatus;
			printJobUpdateQueue.push(finalStatus);
		}

		return response;
	}, [=](ThreadTaskResponse response) -> void {
		{
			wxCriticalSectionLocker lock(criticalLock);
			allowEnablingControls = true;
			printPauseRequested = false;
			printStopRequested = false;
			printPauseStandbyApplied = false;
		}
		statusTimer->Start(100);
		restoreControlsForCurrentState();
		if(!response.message.empty())
			wxMessageBox(response.message, "M33 Manager", response.style);
	});
}

void MyFrame::pausePrintJob() {
	if(printJobStatus.state != PRINT_JOB_RUNNING)
		return;

	const int standbyTemperature = getPauseStandbyTemperatureC();
	PrintJobStatus status;
	{
		wxCriticalSectionLocker lock(criticalLock);
		printPauseRequested = true;
		status = printJobStatus;
		status.state = PRINT_JOB_PAUSED;
		status.statusText = "Pausing and cooling nozzle to " + to_string(standbyTemperature) + "C...";
	}
	applyPrintJobStatus(status);
}

void MyFrame::resumePrintJob() {
	if(printJobStatus.state != PRINT_JOB_PAUSED)
		return;

	PrintJobStatus status;
	{
		wxCriticalSectionLocker lock(criticalLock);
		printPauseRequested = false;
		status = printJobStatus;
		status.state = PRINT_JOB_RUNNING;
		status.statusText = hasTrackedPrintNozzleTarget ? "Reheating nozzle to " + to_string(lastPrintNozzleTargetC) + "C..." : "Resuming print...";
	}
	applyPrintJobStatus(status);
}

void MyFrame::stopPrintJob() {
	if(printJobStatus.state != PRINT_JOB_RUNNING && printJobStatus.state != PRINT_JOB_PAUSED)
		return;

	PrintJobStatus status;
	{
		wxCriticalSectionLocker lock(criticalLock);
		printStopRequested = true;
		printPauseRequested = false;
		status = printJobStatus;
		status.state = PRINT_JOB_STOPPING;
		status.statusText = "Stopping and cooling nozzle...";
	}
	applyPrintJobStatus(status);
}

PrintJobStatus MyFrame::getPrintJobStatus() const {
	wxCriticalSectionLocker lock(const_cast<wxCriticalSection &>(criticalLock));
	return printJobStatus;
}

const PreparedPrintJob &MyFrame::getPreparedPrintJob() const {
	return preparedPrintJob;
}

void MyFrame::applyPrintJobStatus(const PrintJobStatus &status) {
	{
		wxCriticalSectionLocker lock(criticalLock);
		printJobStatus = status;
	}
	printTabController.applyPrintJobStatus(status);
	updatePrintUiAvailability();
	refreshFooterStatus();

	if(status.state == PRINT_JOB_RUNNING || status.state == PRINT_JOB_PAUSED || status.state == PRINT_JOB_STOPPING) {
		setStatusRowVisible(true);
	}
	else if(status.state == PRINT_JOB_COMPLETED) {
		setStatusRowVisible(true);
	}
	else if(status.state == PRINT_JOB_FAILED) {
		setStatusRowVisible(true);
	}

	if(printJobBlocksUiState(status.state)) {
		enableConnectionControls(false);
		enableFirmwareControls(false);
		enableCommandControls(false);
		enableMovementControls(false);
		enableSettingsControls(false);
		enableMiscellaneousControls(false);
		enableCalibrationControls(false);
		enableStandbyControls(false);
		connectionButton->Enable(false);
	}

	else
		restoreControlsForCurrentState();
}

void MyFrame::restoreControlsForCurrentState() {
	if(isPrintJobBlockingUi())
		return;

	if(!printer.isConnected()) {
		switchToModeButton->SetLabel("Switch to bootloader mode");
		enableFirmwareControls(false);
		enableCommandControls(false);
		enableMovementControls(false);
		enableSettingsControls(false);
		enableMiscellaneousControls(false);
		enableCalibrationControls(false);
		enableStandbyControls(true);
		enableConnectionControls(true);
		updatePrintUiAvailability();
		return;
	}

	enableConnectionControls(false);
	connectionButton->Enable(true);
	enableFirmwareControls(true);
	enableCommandControls(true);

	if(printer.getOperatingMode() == BOOTLOADER) {
		switchToModeButton->SetLabel("Switch to firmware mode");
		enableMovementControls(false);
		enableSettingsControls(true);
		enableMiscellaneousControls(false);
		enableCalibrationControls(false);
		enableStandbyControls(true);
	}
	else {
		switchToModeButton->SetLabel("Switch to bootloader mode");
		enableMovementControls(true);
		enableSettingsControls(false);
		enableMiscellaneousControls(true);
		enableCalibrationControls(true);
		enableStandbyControls(true);
	}

	updatePrintUiAvailability();
}

bool MyFrame::isPrintJobBlockingUi() const {
	return printJobBlocksUiState(printJobStatus.state);
}

void MyFrame::updatePrintUiAvailability() {
	if(isPrintJobBlockingUi()) {
		printTabController.setInteractiveEnabled(true);
		return;
	}

	if(!printer.isConnected()) {
		printTabController.setInteractiveEnabled(false, "Connect to a printer to prepare a print.");
		return;
	}

	if(printer.getOperatingMode() == BOOTLOADER) {
		printTabController.setInteractiveEnabled(false, "Printing is unavailable in bootloader mode.");
		return;
	}

	printTabController.setInteractiveEnabled(true);
}
