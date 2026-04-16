#include <wx/filedlg.h>
#include "gui.h"

void MyFrame::onSwitchToModeButton(wxCommandEvent& event) {
	switchToMode();
}

void MyFrame::switchToMode() {

	// Disable mode switch button
	switchToModeButton->Enable(false);
	
	// Set new mode
	wxString buttonLabel = switchToModeButton->GetLabel();
	operatingModes newOperatingMode = buttonLabel == "Switch to firmware mode" ? FIRMWARE : BOOTLOADER;

	enqueueBackgroundTask([=]() -> void {
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
	},
	[=]() -> ThreadTaskResponse {
		return workflows.switchToMode(newOperatingMode);
	},
	[=](ThreadTaskResponse response) -> void {
		
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

	enqueueBackgroundTask([=]() -> void {
	
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
	},
	[=]() -> ThreadTaskResponse {
		return workflows.installImeFirmware();
	},
	[=](ThreadTaskResponse response) -> void {
	
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

	enqueueBackgroundTask([=]() -> void {
	
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
	},
	[=]() -> ThreadTaskResponse {
		return workflows.installM3dFirmware();
	},
	[=](ThreadTaskResponse response) -> void {
	
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
	
	wxFileDialog openFileDialog(this, "Open firmware file", wxEmptyString, wxEmptyString, "Firmware files|*.hex;*.bin;*.rom|All files|*", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	
	// Check if a file was selected
	if(openFileDialog.ShowModal() == wxID_OK) {
	
		// Set firmware location
		string firmwareLocation = static_cast<string>(openFileDialog.GetPath());
		
		enqueueBackgroundTask([=]() -> void {
		
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
		},
		[=]() -> ThreadTaskResponse {
			return workflows.installFirmwareFromFile(firmwareLocation);
		},
		[=](ThreadTaskResponse response) -> void {
		
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

	enqueueBackgroundTask([=]() -> void {
		// Disable install drivers button
		firmwareTabController.setInstallDriversEnabled(false);
	},
	[=]() -> ThreadTaskResponse {
		return workflows.installDrivers();
	},
	[=](ThreadTaskResponse response) -> void {
	
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

	enqueueBackgroundTask(threadStartCallback,
	[=]() -> ThreadTaskResponse {
		return workflows.executeManualCommand(command, [=](const string &message) -> void {
			logToConsole(message);
		});
	},
	threadCompleteCallback);
}

void MyFrame::savePrinterSetting(const string &settingName, const string &value) {
	enqueueBackgroundTask([=]() -> void {
		enableSettingsControls(false);
	},
	[=]() -> ThreadTaskResponse {
		return workflows.savePrinterSetting(settingName, value);
	},
	[=](ThreadTaskResponse response) -> void {
		if(printer.isConnected())
			enableSettingsControls(true);

		wxMessageBox(response.message, "M33 Manager", response.style);
	});
}

void MyFrame::runMoveX(bool positive, double distanceMm, int feedRate) {
	enqueueBackgroundTask(nullptr,
	[=]() -> ThreadTaskResponse {
		return workflows.moveX(positive, distanceMm, feedRate, [=](const string &message) -> void {
			logToConsole(message);
		});
	},
	nullptr);
}

void MyFrame::runMoveY(bool positive, double distanceMm, int feedRate) {
	enqueueBackgroundTask(nullptr,
	[=]() -> ThreadTaskResponse {
		return workflows.moveY(positive, distanceMm, feedRate, [=](const string &message) -> void {
			logToConsole(message);
		});
	},
	nullptr);
}

void MyFrame::runMoveZ(bool positive, double distanceMm, int feedRate) {
	enqueueBackgroundTask(nullptr,
	[=]() -> ThreadTaskResponse {
		return workflows.moveZ(positive, distanceMm, feedRate, [=](const string &message) -> void {
			logToConsole(message);
		});
	},
	nullptr);
}

void MyFrame::runHome() {
	enqueueBackgroundTask(nullptr,
	[=]() -> ThreadTaskResponse {
		return workflows.home([=](const string &message) -> void {
			logToConsole(message);
		});
	},
	nullptr);
}

void MyFrame::runMotorsOn() {
	enqueueBackgroundTask(nullptr,
	[=]() -> ThreadTaskResponse {
		return workflows.motorsOn([=](const string &message) -> void {
			logToConsole(message);
		});
	},
	nullptr);
}

void MyFrame::runMotorsOff() {
	enqueueBackgroundTask(nullptr,
	[=]() -> ThreadTaskResponse {
		return workflows.motorsOff([=](const string &message) -> void {
			logToConsole(message);
		});
	},
	nullptr);
}

void MyFrame::runFanOn() {
	enqueueBackgroundTask(nullptr,
	[=]() -> ThreadTaskResponse {
		return workflows.fanOn([=](const string &message) -> void {
			logToConsole(message);
		});
	},
	nullptr);
}

void MyFrame::runFanOff() {
	enqueueBackgroundTask(nullptr,
	[=]() -> ThreadTaskResponse {
		return workflows.fanOff([=](const string &message) -> void {
			logToConsole(message);
		});
	},
	nullptr);
}

void MyFrame::runHeatNozzle() {
	enqueueBackgroundTask(nullptr,
	[=]() -> ThreadTaskResponse {
		return workflows.setManualNozzleTemperature(getManualNozzleTemperatureC(), [=](const string &message) -> void {
			logToConsole(message);
		});
	},
	nullptr);
}

void MyFrame::runCoolDownNozzle() {
	enqueueBackgroundTask(nullptr,
	[=]() -> ThreadTaskResponse {
		return workflows.coolDownNozzle([=](const string &message) -> void {
			logToConsole(message);
		});
	},
	nullptr);
}

void MyFrame::runCalibrateBedPosition() {
	enqueueBackgroundTask(nullptr,
	[=]() -> ThreadTaskResponse {
		return workflows.calibrateBedPosition([=](const string &message) -> void {
			logToConsole(message);
		});
	},
	nullptr);
}

void MyFrame::runCalibrateBedOrientation() {
	enqueueBackgroundTask(nullptr,
	[=]() -> ThreadTaskResponse {
		return workflows.calibrateBedOrientation([=](const string &message) -> void {
			logToConsole(message);
		});
	},
	nullptr);
}

void MyFrame::runSaveCurrentPositionAsHome() {
	enqueueBackgroundTask(nullptr,
	[=]() -> ThreadTaskResponse {
		return workflows.saveCurrentPositionAsHome([=](const string &message) -> void {
			logToConsole(message);
		});
	},
	[=](ThreadTaskResponse response) -> void {
		wxMessageBox(response.message, "M33 Manager", response.style);
	});
}

void MyFrame::runSaveZAsZero() {
	enqueueBackgroundTask(nullptr,
	[=]() -> ThreadTaskResponse {
		return workflows.saveZAsZero([=](const string &message) -> void {
			logToConsole(message);
		});
	},
	[=](ThreadTaskResponse response) -> void {
		wxMessageBox(response.message, "M33 Manager", response.style);
	});
}
