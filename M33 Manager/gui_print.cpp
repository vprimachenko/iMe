#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include "gui.h"

namespace {

bool printJobBlocksUiState(PrintJobState state) {
	return state == PRINT_JOB_RUNNING || state == PRINT_JOB_PAUSED || state == PRINT_JOB_STOPPING;
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

	printPauseRequested = false;
	printStopRequested = false;

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

	wxCriticalSectionLocker lock(criticalLock);
	threadStartCallbackQueue.push([=]() -> void {});
	threadTaskQueue.push([=]() -> ThreadTaskResponse {
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
				{
					wxCriticalSectionLocker taskLock(criticalLock);
					stopRequested = printStopRequested;
					pauseRequested = printPauseRequested;
				}

				if(stopRequested)
					break;
				if(!pauseRequested)
					break;
				sleepUs(50000);
			}

			{
				wxCriticalSectionLocker taskLock(criticalLock);
				if(printStopRequested)
					break;
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

			workflows.executePreparedPrintCommand(job.printableCommands[i], [=](const string &message) -> void {
				logToConsole(message);
			});

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
			progressStatus.statusText = printPauseRequested ? "Paused" : "Printing...";
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
	});
	threadCompleteCallbackQueue.push([=](ThreadTaskResponse response) -> void {
		allowEnablingControls = true;
		printPauseRequested = false;
		printStopRequested = false;
		statusTimer->Start(100);
		restoreControlsForCurrentState();
		if(!response.message.empty())
			wxMessageBox(response.message, "M33 Manager", response.style);
	});
}

void MyFrame::pausePrintJob() {
	if(printJobStatus.state != PRINT_JOB_RUNNING)
		return;

	printPauseRequested = true;
	PrintJobStatus status = printJobStatus;
	status.state = PRINT_JOB_PAUSED;
	status.statusText = "Paused";
	applyPrintJobStatus(status);
}

void MyFrame::resumePrintJob() {
	if(printJobStatus.state != PRINT_JOB_PAUSED)
		return;

	printPauseRequested = false;
	PrintJobStatus status = printJobStatus;
	status.state = PRINT_JOB_RUNNING;
	status.statusText = "Printing";
	applyPrintJobStatus(status);
}

void MyFrame::stopPrintJob() {
	if(printJobStatus.state != PRINT_JOB_RUNNING && printJobStatus.state != PRINT_JOB_PAUSED)
		return;

	printStopRequested = true;
	printPauseRequested = false;
	PrintJobStatus status = printJobStatus;
	status.state = PRINT_JOB_STOPPING;
	status.statusText = "Stopping";
	applyPrintJobStatus(status);
}

PrintJobStatus MyFrame::getPrintJobStatus() const {
	return printJobStatus;
}

const PreparedPrintJob &MyFrame::getPreparedPrintJob() const {
	return preparedPrintJob;
}

void MyFrame::applyPrintJobStatus(const PrintJobStatus &status) {
	printJobStatus = status;
	printTabController.applyPrintJobStatus(status);
	updatePrintUiAvailability();

	if(status.state == PRINT_JOB_RUNNING || status.state == PRINT_JOB_PAUSED || status.state == PRINT_JOB_STOPPING) {
		setStatusRowVisible(true);
		statusText->SetLabel(status.statusText.empty() ? "Printing..." : status.statusText);
		statusText->SetForegroundColour(status.state == PRINT_JOB_PAUSED ? wxColour(255, 180, 0) : wxColour(80, 170, 255));
	}
	else if(status.state == PRINT_JOB_COMPLETED) {
		setStatusRowVisible(true);
		statusText->SetLabel("Print completed");
		statusText->SetForegroundColour(wxColour(0, 200, 0));
	}
	else if(status.state == PRINT_JOB_FAILED) {
		setStatusRowVisible(true);
		statusText->SetLabel(status.errorText.empty() ? "Print failed" : status.errorText);
		statusText->SetForegroundColour(wxColour(255, 0, 0));
	}

	if(printJobBlocksUiState(status.state)) {
		enableConnectionControls(false);
		enableFirmwareControls(false);
		enableCommandControls(false);
		enableMovementControls(false);
		enableSettingsControls(false);
		enableMiscellaneousControls(false);
		enableCalibrationControls(false);
		connectionButton->Enable(false);
	}

	else
		restoreControlsForCurrentState();
}

void MyFrame::restoreControlsForCurrentState() {
	if(isPrintJobBlockingUi())
		return;

	if(!printer.isConnected()) {
		enableFirmwareControls(false);
		enableCommandControls(false);
		enableMovementControls(false);
		enableSettingsControls(false);
		enableMiscellaneousControls(false);
		enableCalibrationControls(false);
		enableConnectionControls(true);
		updatePrintUiAvailability();
		return;
	}

	enableConnectionControls(false);
	connectionButton->Enable(true);
	enableFirmwareControls(true);
	enableCommandControls(true);

	if(printer.getOperatingMode() == BOOTLOADER) {
		enableMovementControls(false);
		enableSettingsControls(true);
		enableMiscellaneousControls(false);
		enableCalibrationControls(false);
	}
	else {
		enableMovementControls(true);
		enableSettingsControls(false);
		enableMiscellaneousControls(true);
		enableCalibrationControls(true);
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
