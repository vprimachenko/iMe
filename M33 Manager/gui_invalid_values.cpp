#include <wx/msgdlg.h>
#include "gui.h"

void MyFrame::checkInvalidValues() {

	// Check if printer is connected
	if(printer.isConnected()) {

		enqueueBackgroundTask(nullptr, [=]() -> ThreadTaskResponse {
			workflows.ensureMode(BOOTLOADER);
			return {"", 0};
		}, [=](ThreadTaskResponse response) -> void {

			// Check if printer is still connected
			if(printer.isConnected()) {

				InvalidValuesReport report = workflows.inspectInvalidValues();
		
				// Set calibrate bed orientation dialog
				function<void()> calibrateBedOrientationDialog = [=]() -> void {

					// Check if printer's bed orientation is invalid
					if(report.hasInvalidBedOrientation) {
	
						// Display bed orientation calibration dialog
						wxMessageDialog dial(nullptr, "Bed orientation is invalid. Calibrate?", "M33 Manager", wxYES_NO | wxYES_DEFAULT | wxICON_QUESTION);
		
						// Check if calibrating bed orientation
						if(dial.ShowModal() == wxID_YES) {
		
							enqueueBackgroundTask([=]() -> void {
					
								// Stop status timer
								statusTimer->Stop();
					
								// Set fixing invalid values
								fixingInvalidValues = true;
				
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
								statusText->SetLabel("Calibrating bed orientation");
								statusText->SetForegroundColour(wxColour(255, 180, 0));
							},
							[=]() -> ThreadTaskResponse {
								workflows.ensureMode(FIRMWARE);
								return {"", 0};
							},
							[=](ThreadTaskResponse response) -> void {
					
								// Clear fixing invalid values
								fixingInvalidValues = false;
					
								// Start status timer
								statusTimer->Start(100);
					
								// Check if printer is still connected
								if(printer.isConnected()) {
					
									// Stop status timer
									statusTimer->Stop();
						
									// Set fixing invalid values
									fixingInvalidValues = true;
					
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
					
									// Calibrate bed orientation
									runCalibrateBedOrientation();
						
									enqueueBackgroundTask(nullptr,
									[=]() -> ThreadTaskResponse {
										return {"", 0};
									},
									[=](ThreadTaskResponse response) -> void {
						
										// Clear fixing invalid values
										fixingInvalidValues = false;
							
										// Start status timer
										statusTimer->Start(100);
						
										// Enable connection button
										connectionButton->Enable(true);
					
										// Check if printer is still connected
										if(printer.isConnected()) {
							
											// Enable connection button
											connectionButton->Enable(true);

											// Enable firmware controls
											enableFirmwareControls(true);
											enableCommandControls(true);
								
											// Enable movement controls
											enableMovementControls(true);
		
											// Enable miscellaneous controls
											enableMiscellaneousControls(true);
											
											// Enable calibration controls
											enableCalibrationControls(true);
									
											// Log completion and printer mode
											logToConsole("Done checking printer's invalid values");
											logToConsole(static_cast<string>("Printer is in ") + (printer.getOperatingMode() == BOOTLOADER ? "bootloader" : "firmware") + " mode");
								
											// Display message
											wxMessageBox("Bed orientation successfully calibrated", "M33 Manager", wxOK | wxICON_INFORMATION | wxCENTRE);
										}
							
										// Otherwise
										else {
							
											// Enable connection controls
											enableConnectionControls(true);
								
											// Display message
											wxMessageBox("Failed to calibrate bed orientation", "M33 Manager", wxOK | wxICON_ERROR | wxCENTRE);
										}
									});
								}
					
								// Otherwise
								else
					
									// Display message
									wxMessageBox("Failed to calibrate bed orientation", "M33 Manager", wxOK | wxICON_ERROR | wxCENTRE);
							});
						}
				
						// Otherwise
						else {
				
							// Log completion and printer mode
							logToConsole("Done checking printer's invalid values");
							logToConsole(static_cast<string>("Printer is in ") + (printer.getOperatingMode() == BOOTLOADER ? "bootloader" : "firmware") + " mode");
						}
					}
			
					// Otherwise
					else {
			
						// Log completion and printer mode
						logToConsole("Done checking printer's invalid values");
						logToConsole(static_cast<string>("Printer is in ") + (printer.getOperatingMode() == BOOTLOADER ? "bootloader" : "firmware") + " mode");
					}
				};
		
				// Set calibrate bed position dialog
				function<void()> calibrateBedPositionDialog = [=]() -> void {

					// Check if printer's bed position is invalid
					if(report.hasInvalidBedPosition) {
	
						// Display bed position calibration dialog
						wxMessageDialog dial(nullptr, "Bed position is invalid. Calibrate?", "M33 Manager", wxYES_NO | wxYES_DEFAULT | wxICON_QUESTION);
		
						// Check if calibrating bed position
						if(dial.ShowModal() == wxID_YES) {
				
							enqueueBackgroundTask([=]() -> void {
					
								// Stop status timer
								statusTimer->Stop();
					
								// Set fixing invalid values
								fixingInvalidValues = true;
				
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
								statusText->SetLabel("Calibrating bed position");
								statusText->SetForegroundColour(wxColour(255, 180, 0));
							},
							[=]() -> ThreadTaskResponse {
								workflows.ensureMode(FIRMWARE);
								return {"", 0};
							},
							[=](ThreadTaskResponse response) -> void {
					
								// Clear fixing invalid values
								fixingInvalidValues = false;
					
								// Start status timer
								statusTimer->Start(100);
					
								// Check if printer is still connected
								if(printer.isConnected()) {
					
									// Stop status timer
									statusTimer->Stop();
						
									// Set fixing invalid values
									fixingInvalidValues = true;
					
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
					
									// Calibrate bed position
									runCalibrateBedPosition();
						
									enqueueBackgroundTask(nullptr,
									[=]() -> ThreadTaskResponse {
										return {"", 0};
									},
									[=](ThreadTaskResponse response) -> void {
						
										// Clear fixing invalid values
										fixingInvalidValues = false;
							
										// Start status timer
										statusTimer->Start(100);
						
										// Enable connection button
										connectionButton->Enable(true);
					
										// Check if printer is still connected
										if(printer.isConnected()) {
							
											// Enable connection button
											connectionButton->Enable(true);

											// Enable firmware controls
											enableFirmwareControls(true);
											enableCommandControls(true);
								
											// Enable movement controls
											enableMovementControls(true);
		
											// Enable miscellaneous controls
											enableMiscellaneousControls(true);
											
											// Enable calibration controls
											enableCalibrationControls(true);
											
											// Display message
											wxMessageBox("Bed position successfully calibrated", "M33 Manager", wxOK | wxICON_INFORMATION | wxCENTRE);
						
											// Display calibrate bed orientation dialog
											calibrateBedOrientationDialog();
										}
							
										// Otherwise
										else {
							
											// Enable connection controls
											enableConnectionControls(true);
								
											// Display message
											wxMessageBox("Failed to calibrate bed position", "M33 Manager", wxOK | wxICON_ERROR | wxCENTRE);
										}
									});
								}
					
								// Otherwise
								else
					
									// Display message
									wxMessageBox("Failed to calibrate bed position", "M33 Manager", wxOK | wxICON_ERROR | wxCENTRE);
							});
						}
				
						// Otherwise
						else
			
							// Display calibrate bed orientation dialog
							calibrateBedOrientationDialog();
					}
			
					// Otherwise
					else
			
						// Display calibrate bed orientation dialog
						calibrateBedOrientationDialog();
				};
		
				// Set install firmware dialog
				function<void()> installFirmwareDialog = [=]() -> void {

					// Check if printer's firmware is corrupt
					if(report.hasInvalidFirmware) {
						// Display firmware installation dialog
						wxMessageDialog dial(nullptr, "Firmware is corrupt. Install " + workflows.getFirmwareDisplayName(report.firmwareAction) + "?", "M33 Manager", wxYES_NO | wxYES_DEFAULT | wxICON_QUESTION);
		
						// Check if installing firmware
						if(dial.ShowModal() == wxID_YES) {
				
							// Set fixing invalid values
							fixingInvalidValues = true;
		
							// Check if installing M3D firmware
							if(report.firmwareAction == FIRMWARE_REMEDIATION_INSTALL_M3D) {
			
								// Install M3D firmware
								installM3dFirmware();
							}
			
							// Otherwise
							else {
			
								// Install iMe firmware
								installImeFirmware();
							}
					
							enqueueBackgroundTask(nullptr,
							[=]() -> ThreadTaskResponse {
								return {"", 0};
							},
							[=](ThreadTaskResponse response) -> void {
					
								// Clear fixing invalid values
								fixingInvalidValues = false;
					
								// Check if printer is still connected
								if(printer.isConnected()) {
								
									// Display message
									wxMessageBox("Firmware successfully installed", "M33 Manager", wxOK | wxICON_INFORMATION | wxCENTRE);
						
									// Display calibrate bed position dialog
									calibrateBedPositionDialog();
								}
						
								// Otherwise
								else
						
									// Display message
									wxMessageBox("Failed to update firmware", "M33 Manager", wxOK | wxICON_ERROR | wxCENTRE);
							});
						}
						
						// Otherwise
						else
			
							// Disconnect printer
							printer.disconnect();
					}
						
					// Otherwise check if firmware is incompatible or outdated
					else if(report.firmwareUpdateAvailable) {
						// Display firmware installation dialog
						wxMessageDialog dial(nullptr, static_cast<string>(report.firmwareIsIncompatible ? "Firmware is incompatible" : "Newer firmware available") + ". Update to " + workflows.getFirmwareDisplayName(report.firmwareAction) + "?", "M33 Manager", wxYES_NO | wxYES_DEFAULT | wxICON_QUESTION);
	
						// Check if installing firmware
						if(dial.ShowModal() == wxID_YES) {
			
							// Set fixing invalid values
							fixingInvalidValues = true;
	
							// Check if installing M3D firmware
							if(report.firmwareAction == FIRMWARE_REMEDIATION_INSTALL_M3D) {
		
								// Install M3D firmware
								installM3dFirmware();
							}
		
							// Otherwise
							else {
		
								// Install iMe firmware
								installImeFirmware();
							}
				
							enqueueBackgroundTask(nullptr,
							[=]() -> ThreadTaskResponse {
								return {"", 0};
							},
							[=](ThreadTaskResponse response) -> void {
				
								// Clear fixing invalid values
								fixingInvalidValues = false;
				
								// Check if printer is still connected
								if(printer.isConnected()) {
							
									// Display message
									wxMessageBox("Firmware successfully installed", "M33 Manager", wxOK | wxICON_INFORMATION | wxCENTRE);
					
									// Display calibrate bed position dialog
									calibrateBedPositionDialog();
								}
					
								// Otherwise
								else
					
									// Display message
									wxMessageBox("Failed to update firmware", "M33 Manager", wxOK | wxICON_ERROR | wxCENTRE);
							});
						}
						
						// Otherwise check if firmware is incompatible
						else if(report.firmwareIsIncompatible)
						
							// Disconnect printer
							printer.disconnect();
				
						// Otherwise
						else
				
							// Display calibrate bed position dialog
							calibrateBedPositionDialog();
					}
			
					// Otherwise
					else
			
						// Display calibrate bed position dialog
						calibrateBedPositionDialog();
				};
		
				// Check if printer has at least one invalid value
				if(report.hasInvalidBedOrientation || report.hasInvalidBedPosition || report.hasInvalidFirmware || report.firmwareUpdateAvailable)
		
					// Display install firmware dialog
					installFirmwareDialog();
			}
		
			// Otherwise
			else

				// Display message
				wxMessageBox("Failed to check the printer's invalid values", "M33 Manager", wxOK | wxICON_ERROR | wxCENTRE);
		});
	}
}
