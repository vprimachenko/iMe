#include <wx/filedlg.h>
#include "gui.h"

void MyFrame::switchToMode() {

	// Disable mode switch button
	switchToModeButton->Enable(false);
	
	// Set new mode
	operatingModes newOperatingMode = switchToModeButton->GetLabel() == "Switch to firmware mode" ? FIRMWARE : BOOTLOADER;

	// Lock
	wxCriticalSectionLocker lock(criticalLock);

	// Append thread start callback to queue
	threadStartCallbackQueue.push([=]() -> void {
	
		// Disable connection controls
		enableConnectionControls(false);
	
		// Disable firmware controls
		enableFirmwareControls(false);
		enableCommandControls(false);
		
		// Disable movement controls
		enableMovementControls(false);
		
		// Disable settings controls
		enableSettingsControls(false);
		
		// Disable miscellaneous controls
		enableMiscellaneousControls(false);
		
		// Disable calibration controls
		enableCalibrationControls(false);
	});
	
	// Append thread task to queue
	threadTaskQueue.push([=]() -> ThreadTaskResponse {
		return workflows.switchToMode(newOperatingMode);
	});
	
	// Append thread complete callback to queue
	threadCompleteCallbackQueue.push([=](ThreadTaskResponse response) -> void {
		
		// Enable connection button
		connectionButton->Enable(true);

		// Check if connected to printer
		if(printer.isConnected()) {

			// Enable firmware controls
			enableFirmwareControls(true);
			enableCommandControls(true);
		}

		// Otherwise
		else

			// Enable connection controls
			enableConnectionControls(true);
		
		// Display message
		wxMessageBox(response.message, "M33 Manager", response.style);
	});
}

void MyFrame::installImeFirmware() {

	// Lock
	wxCriticalSectionLocker lock(criticalLock);

	// Append thread start callback to queue
	threadStartCallbackQueue.push([=]() -> void {
	
		// Stop status timer
		statusTimer->Stop();

		// Disable connection controls
		enableConnectionControls(false);
	
		// Disable firmware controls
		enableFirmwareControls(false);
		enableCommandControls(false);
		
		// Disable movement controls
		enableMovementControls(false);
		
		// Disable settings controls
		enableSettingsControls(false);
		
		// Disable miscellaneous controls
		enableMiscellaneousControls(false);
		
		// Disable calibration controls
		enableCalibrationControls(false);
	
		// Set status text
		statusText->SetLabel("Installing firmware");
		statusText->SetForegroundColour(wxColour(255, 180, 0));
		
		// Clear allow enabling controls
		allowEnablingControls = false;
	});
	
	// Append thread task to queue
	threadTaskQueue.push([=]() -> ThreadTaskResponse {
		return workflows.installImeFirmware();
	});
	
	// Append thread complete callback to queue
	threadCompleteCallbackQueue.push([=](ThreadTaskResponse response) -> void {
	
		// Set allow enabling controls
		allowEnablingControls = true;
	
		// Enable connection button
		connectionButton->Enable(true);

		// Check if connected to printer
		if(printer.isConnected()) {

			// Enable firmware controls
			enableFirmwareControls(true);
			enableCommandControls(true);
			
			// Log printer mode
			logToConsole(static_cast<string>("Printer is in ") + (printer.getOperatingMode() == BOOTLOADER ? "bootloader" : "firmware") + " mode");
		}

		// Otherwise
		else

			// Enable connection controls
			enableConnectionControls(true);
		
		// Start status timer
		statusTimer->Start(100);
		
		// Check if not fixing invalid values
		if(!fixingInvalidValues) {
		
			// Display message
			wxMessageBox(response.message, "M33 Manager", response.style);
			
			// Check invalid values
			checkInvalidValues();
		}
	});
}

void MyFrame::installM3dFirmware() {

	// Lock
	wxCriticalSectionLocker lock(criticalLock);

	// Append thread start callback to queue
	threadStartCallbackQueue.push([=]() -> void {
	
		// Stop status timer
		statusTimer->Stop();

		// Disable connection controls
		enableConnectionControls(false);
	
		// Disable firmware controls
		enableFirmwareControls(false);
		enableCommandControls(false);
		
		// Disable movement controls
		enableMovementControls(false);
		
		// Disable settings controls
		enableSettingsControls(false);
		
		// Disable miscellaneous controls
		enableMiscellaneousControls(false);
		
		// Disable calibration controls
		enableCalibrationControls(false);
	
		// Set status text
		statusText->SetLabel("Installing firmware");
		statusText->SetForegroundColour(wxColour(255, 180, 0));
		
		// Clear allow enabling controls
		allowEnablingControls = false;
	});
	
	// Append thread task to queue
	threadTaskQueue.push([=]() -> ThreadTaskResponse {
		return workflows.installM3dFirmware();
	});
	
	// Append thread complete callback to queue
	threadCompleteCallbackQueue.push([=](ThreadTaskResponse response) -> void {
	
		// Set allow enabling controls
		allowEnablingControls = true;
	
		// Enable connection button
		connectionButton->Enable(true);

		// Check if connected to printer
		if(printer.isConnected()) {

			// Enable firmware controls
			enableFirmwareControls(true);
			enableCommandControls(true);
			
			// Log printer mode
			logToConsole(static_cast<string>("Printer is in ") + (printer.getOperatingMode() == BOOTLOADER ? "bootloader" : "firmware") + " mode");
		}

		// Otherwise
		else

			// Enable connection controls
			enableConnectionControls(true);
		
		// Start status timer
		statusTimer->Start(100);
		
		// Check if not fixing invalid values
		if(!fixingInvalidValues) {
		
			// Display message
			wxMessageBox(response.message, "M33 Manager", response.style);
			
			// Check invalid values
			checkInvalidValues();
		}
	});
}

void MyFrame::installFirmwareFromFile() {
	
	// Display file dialog
	wxFileDialog *openFileDialog = new wxFileDialog(this, "Open firmware file", wxEmptyString, wxEmptyString, "Firmware files|*.hex;*.bin;*.rom|All files|*", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	
	// Check if a file was selected
	if(openFileDialog->ShowModal() == wxID_OK) {
	
		// Set firmware location
		string firmwareLocation = static_cast<string>(openFileDialog->GetPath());
		
		// Lock
		wxCriticalSectionLocker lock(criticalLock);

		// Append thread start callback to queue
		threadStartCallbackQueue.push([=]() -> void {
		
			// Stop status timer
			statusTimer->Stop();

			// Disable connection controls
			enableConnectionControls(false);
	
			// Disable firmware controls
			enableFirmwareControls(false);
			enableCommandControls(false);
			
			// Disable movement controls
			enableMovementControls(false);
			
			// Disable settings controls
			enableSettingsControls(false);
			
			// Disable miscellaneous controls
			enableMiscellaneousControls(false);
			
			// Disable calibration controls
			enableCalibrationControls(false);
	
			// Set status text
			statusText->SetLabel("Installing firmware");
			statusText->SetForegroundColour(wxColour(255, 180, 0));
			
			// Clear allow enabling controls
			allowEnablingControls = false;
		});
		
		// Append thread task to queue
		threadTaskQueue.push([=]() -> ThreadTaskResponse {
			return workflows.installFirmwareFromFile(firmwareLocation);
		});
	
		// Append thread complete callback to queue
		threadCompleteCallbackQueue.push([=](ThreadTaskResponse response) -> void {
		
			// Set allow enabling controls
			allowEnablingControls = true;
	
			// Enable connection button
			connectionButton->Enable(true);

			// Check if connected to printer
			if(printer.isConnected()) {

				// Enable firmware controls
				enableFirmwareControls(true);
				enableCommandControls(true);
				
				// Log printer mode
				logToConsole(static_cast<string>("Printer is in ") + (printer.getOperatingMode() == BOOTLOADER ? "bootloader" : "firmware") + " mode");
			}

			// Otherwise
			else

				// Enable connection controls
				enableConnectionControls(true);
		
			// Start status timer
			statusTimer->Start(100);
		
			// Check if not fixing invalid values
			if(!fixingInvalidValues) {
		
				// Display message
				wxMessageBox(response.message, "M33 Manager", response.style);
			
				// Check invalid values
				checkInvalidValues();
			}
		});
	}
}

void MyFrame::installDrivers() {

	// Lock
	wxCriticalSectionLocker lock(criticalLock);

	// Append thread start callback to queue
	threadStartCallbackQueue.push([=]() -> void {
	
		// Disable install drivers button
		firmwareTabController.setInstallDriversEnabled(false);
	});
	
	// Append thread task to queue
	threadTaskQueue.push([=]() -> ThreadTaskResponse {
		return workflows.installDrivers();
	});
	
	// Append thread complete callback to queue
	threadCompleteCallbackQueue.push([=](ThreadTaskResponse response) -> void {
	
		// Enable install drivers button
		firmwareTabController.setInstallDriversEnabled(true);
	
		// Display message
		wxMessageBox(response.message, "M33 Manager", response.style);
	});
}

vector<string> MyFrame::getPrinterSettingNames() {
	return workflows.getPrinterSettingNames();
}

string MyFrame::loadPrinterSettingValue(const string &settingName) {
	return workflows.loadPrinterSettingValue(settingName);
}

void MyFrame::runManualCommand(const string &command) {

	function<void()> threadStartCallback = nullptr;
	function<void(ThreadTaskResponse response)> threadCompleteCallback = nullptr;

	if(workflows.prepareManualCommand(command).requiresModeTransitionHandling) {
		threadStartCallback = [=]() -> void {
			enableConnectionControls(false);
			enableFirmwareControls(false);
			enableCommandControls(false);
			enableMovementControls(false);
			enableMiscellaneousControls(false);
			enableCalibrationControls(false);
			enableSettingsControls(false);
		};

		threadCompleteCallback = [=](ThreadTaskResponse response) -> void {
			connectionButton->Enable(true);

			if(printer.isConnected()) {
				enableFirmwareControls(true);
				enableCommandControls(true);
			}

			else
				enableConnectionControls(true);
		};
	}

	wxCriticalSectionLocker lock(criticalLock);
	threadStartCallbackQueue.push(threadStartCallback ? threadStartCallback : [=]() -> void {});
	threadTaskQueue.push([=]() -> ThreadTaskResponse {
		return workflows.executeManualCommand(command, [=](const string &message) -> void {
			logToConsole(message);
		});
	});
	threadCompleteCallbackQueue.push(threadCompleteCallback ? threadCompleteCallback : [=](ThreadTaskResponse response) -> void {});
}

void MyFrame::savePrinterSetting(const string &settingName, const string &value) {

	threadStartCallbackQueue.push([=]() -> void {
		enableSettingsControls(false);
	});

	threadTaskQueue.push([=]() -> ThreadTaskResponse {
		return workflows.savePrinterSetting(settingName, value);
	});

	threadCompleteCallbackQueue.push([=](ThreadTaskResponse response) -> void {
		if(printer.isConnected())
			enableSettingsControls(true);

		wxMessageBox(response.message, "M33 Manager", response.style);
	});
}

void MyFrame::runMoveX(bool positive, double distanceMm, int feedRate) {
	wxCriticalSectionLocker lock(criticalLock);
	threadStartCallbackQueue.push([=]() -> void {});
	threadTaskQueue.push([=]() -> ThreadTaskResponse {
		return workflows.moveX(positive, distanceMm, feedRate, [=](const string &message) -> void {
			logToConsole(message);
		});
	});
	threadCompleteCallbackQueue.push([=](ThreadTaskResponse response) -> void {});
}

void MyFrame::runMoveY(bool positive, double distanceMm, int feedRate) {
	wxCriticalSectionLocker lock(criticalLock);
	threadStartCallbackQueue.push([=]() -> void {});
	threadTaskQueue.push([=]() -> ThreadTaskResponse {
		return workflows.moveY(positive, distanceMm, feedRate, [=](const string &message) -> void {
			logToConsole(message);
		});
	});
	threadCompleteCallbackQueue.push([=](ThreadTaskResponse response) -> void {});
}

void MyFrame::runMoveZ(bool positive, double distanceMm, int feedRate) {
	wxCriticalSectionLocker lock(criticalLock);
	threadStartCallbackQueue.push([=]() -> void {});
	threadTaskQueue.push([=]() -> ThreadTaskResponse {
		return workflows.moveZ(positive, distanceMm, feedRate, [=](const string &message) -> void {
			logToConsole(message);
		});
	});
	threadCompleteCallbackQueue.push([=](ThreadTaskResponse response) -> void {});
}

void MyFrame::runHome() {
	wxCriticalSectionLocker lock(criticalLock);
	threadStartCallbackQueue.push([=]() -> void {});
	threadTaskQueue.push([=]() -> ThreadTaskResponse {
		return workflows.home([=](const string &message) -> void {
			logToConsole(message);
		});
	});
	threadCompleteCallbackQueue.push([=](ThreadTaskResponse response) -> void {});
}

void MyFrame::runMotorsOn() {
	wxCriticalSectionLocker lock(criticalLock);
	threadStartCallbackQueue.push([=]() -> void {});
	threadTaskQueue.push([=]() -> ThreadTaskResponse {
		return workflows.motorsOn([=](const string &message) -> void {
			logToConsole(message);
		});
	});
	threadCompleteCallbackQueue.push([=](ThreadTaskResponse response) -> void {});
}

void MyFrame::runMotorsOff() {
	wxCriticalSectionLocker lock(criticalLock);
	threadStartCallbackQueue.push([=]() -> void {});
	threadTaskQueue.push([=]() -> ThreadTaskResponse {
		return workflows.motorsOff([=](const string &message) -> void {
			logToConsole(message);
		});
	});
	threadCompleteCallbackQueue.push([=](ThreadTaskResponse response) -> void {});
}

void MyFrame::runFanOn() {
	wxCriticalSectionLocker lock(criticalLock);
	threadStartCallbackQueue.push([=]() -> void {});
	threadTaskQueue.push([=]() -> ThreadTaskResponse {
		return workflows.fanOn([=](const string &message) -> void {
			logToConsole(message);
		});
	});
	threadCompleteCallbackQueue.push([=](ThreadTaskResponse response) -> void {});
}

void MyFrame::runFanOff() {
	wxCriticalSectionLocker lock(criticalLock);
	threadStartCallbackQueue.push([=]() -> void {});
	threadTaskQueue.push([=]() -> ThreadTaskResponse {
		return workflows.fanOff([=](const string &message) -> void {
			logToConsole(message);
		});
	});
	threadCompleteCallbackQueue.push([=](ThreadTaskResponse response) -> void {});
}

void MyFrame::runCalibrateBedPosition() {
	wxCriticalSectionLocker lock(criticalLock);
	threadStartCallbackQueue.push([=]() -> void {});
	threadTaskQueue.push([=]() -> ThreadTaskResponse {
		return workflows.calibrateBedPosition([=](const string &message) -> void {
			logToConsole(message);
		});
	});
	threadCompleteCallbackQueue.push([=](ThreadTaskResponse response) -> void {});
}

void MyFrame::runCalibrateBedOrientation() {
	wxCriticalSectionLocker lock(criticalLock);
	threadStartCallbackQueue.push([=]() -> void {});
	threadTaskQueue.push([=]() -> ThreadTaskResponse {
		return workflows.calibrateBedOrientation([=](const string &message) -> void {
			logToConsole(message);
		});
	});
	threadCompleteCallbackQueue.push([=](ThreadTaskResponse response) -> void {});
}

void MyFrame::runSaveZAsZero() {
	wxCriticalSectionLocker lock(criticalLock);
	threadStartCallbackQueue.push([=]() -> void {});
	threadTaskQueue.push([=]() -> ThreadTaskResponse {
		return workflows.saveZAsZero([=](const string &message) -> void {
			logToConsole(message);
		});
	});
	threadCompleteCallbackQueue.push([=](ThreadTaskResponse response) -> void {});
}
