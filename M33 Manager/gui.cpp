// Header files
#include <fstream>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <wx/mstream.h>
#include "gui.h"
#ifdef WINDOWS
	#include <setupapi.h>
	#include <dbt.h>
#endif


// Custom events
wxDEFINE_EVENT(wxEVT_THREAD_TASK_START, wxThreadEvent);
wxDEFINE_EVENT(wxEVT_THREAD_TASK_COMPLETE, wxThreadEvent);


// Supporting function implementation
wxBitmap loadImage(const unsigned char *data, unsigned long long size, int width = -1, int height = -1, int offsetX = 0, int offsetY = 0) {
	
	// Load image from data
	wxMemoryInputStream imageStream(data, size);
	wxImage image;
	image.LoadFile(imageStream, wxBITMAP_TYPE_PNG);
	
	// Resize image
	if(width == -1 || height == -1)
		image.Rescale(width == -1 ? image.GetWidth() : width, height == -1 ? image.GetHeight() : height, wxIMAGE_QUALITY_HIGH);
	
	// Offset image
	if(offsetX || offsetY)
		image.Resize(wxSize(image.GetWidth() + offsetX, image.GetHeight() + offsetY), wxPoint(offsetX, offsetY));
	
	// Return bitmap
	return wxBitmap(image);
}

MyFrame::MyFrame(const wxString& title, const wxPoint& pos, const wxSize& size, long style) :
	wxFrame(nullptr, wxID_ANY, title, pos, size, style)
{
	// Set printer's log function
	printer.setLogFunction([=](const string &message) -> void {
	
		// Log message to console
		logToConsole(message);
	});
	
	// Set allow enabling controls
	allowEnablingControls = true;
	
	// Clear fixing invalid values
	fixingInvalidValues = false;

	// Initialize PNG image handler
	wxImage::AddHandler(new wxPNGHandler);
	
	// Set icon
	#ifdef WINDOWS
		SetIcon(wxICON(MAINICON));
	#endif
	
	#ifdef LINUX
		wxIcon icon;
		icon.CopyFromBitmap(loadImage(iconPngData, sizeof(iconPngData))); 
		SetIcon(icon);
	#endif

	// Add menu bar
	#ifdef MACOS
		wxMenuBar *menuBar = new wxMenuBar;
		SetMenuBar(menuBar);
	#endif
	
	// Close event
	Bind(wxEVT_CLOSE_WINDOW, &MyFrame::close, this);
	
	ui = buildMainLayout(this);

	// Build connection section
	wxBoxSizer *connectionRowSizer = new wxBoxSizer(wxHORIZONTAL);
	connectionRowSizer->Add(new wxStaticText(ui.connectionSection, wxID_ANY, "Serial Port"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

	serialPortChoice = new wxChoice(ui.connectionSection, wxID_ANY, wxDefaultPosition, wxDefaultSize, getAvailableSerialPorts());
	serialPortChoice->SetSelection(serialPortChoice->FindString("Auto"));
	serialPortChoice->SetMinSize(wxSize(300, -1));
	connectionRowSizer->Add(serialPortChoice, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

	refreshSerialPortsButton = new wxBitmapButton(ui.connectionSection, wxID_ANY, loadImage(refreshPngData, sizeof(refreshPngData),
	#ifdef WINDOWS
		-1, -1, 0, 0
	#endif
	#ifdef MACOS
		-1, -1, 0, 2
	#endif
	#ifdef LINUX
		-1, -1, 0, 0
	#endif
	));
	refreshSerialPortsButton->Bind(wxEVT_BUTTON, &MyFrame::refreshSerialPorts, this);
	connectionRowSizer->Add(refreshSerialPortsButton, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

	connectionButton = new wxButton(ui.connectionSection, wxID_ANY, "Connect");
	connectionButton->Bind(wxEVT_BUTTON, &MyFrame::changePrinterConnection, this);
	connectionRowSizer->Add(connectionButton, 0, wxALIGN_CENTER_VERTICAL);

	ui.connectionContentSizer->Add(connectionRowSizer, 0, wxEXPAND);

	ui.statusRowSizer->Add(new wxStaticText(ui.statusRow, wxID_ANY, "Status:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
	statusText = new wxStaticText(ui.statusRow, wxID_ANY, "Not connected");
	statusText->SetForegroundColour(wxColour(255, 0, 0));
	ui.statusRowSizer->Add(statusText, 1, wxALIGN_CENTER_VERTICAL);
	ui.connectionContentSizer->Add(ui.statusRow, 0, wxEXPAND | wxTOP, 8);
	
	// Create log timer
	wxTimer *logTimer = new wxTimer(this, wxID_ANY);
	Bind(wxEVT_TIMER, &MyFrame::updateLog, this, logTimer->GetId());
	logTimer->Start(100);
	
	// Create status timer
	statusTimer = new wxTimer(this, wxID_ANY);
	Bind(wxEVT_TIMER, &MyFrame::updateStatus, this, statusTimer->GetId());
	statusTimer->Start(100);
	
	// Check if not using macOS
	#ifndef MACOS
	
		// Create install drivers button
		installDriversButton = new wxButton(ui.firmwareSection, wxID_ANY, "Install drivers");
		installDriversButton->Bind(wxEVT_BUTTON, &MyFrame::installDrivers, this);
	#endif

	// Create firmware section
	string iMeVersion = static_cast<string>(TOSTRING(IME_ROM_VERSION_STRING)).substr(2);
	for(uint8_t i = 0; i < 3; i++)
		iMeVersion.insert(i * 2 + 2 + i, ".");
	installImeFirmwareButton = new wxButton(ui.firmwareSection, wxID_ANY, "Install iMe V" + iMeVersion);
	installImeFirmwareButton->Enable(false);
	installImeFirmwareButton->Bind(wxEVT_BUTTON, &MyFrame::installImeFirmware, this);
	
	installM3dFirmwareButton = new wxButton(ui.firmwareSection, wxID_ANY, "Install M3D V" TOSTRING(M3D_ROM_VERSION_STRING));
	installM3dFirmwareButton->Enable(false);
	installM3dFirmwareButton->Bind(wxEVT_BUTTON, &MyFrame::installM3dFirmware, this);
	
	installFirmwareFromFileButton = new wxButton(ui.firmwareSection, wxID_ANY, "Install firmware from file");
	installFirmwareFromFileButton->Enable(false);
	installFirmwareFromFileButton->Bind(wxEVT_BUTTON, &MyFrame::installFirmwareFromFile, this);
	
	switchToModeButton = new wxButton(ui.firmwareSection, wxID_ANY, "Switch to bootloader mode");
	switchToModeButton->Enable(false);
	switchToModeButton->Bind(wxEVT_BUTTON, &MyFrame::switchToMode, this);

	wxBoxSizer *firmwareRowsSizer = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer *firmwareRow1Sizer = new wxBoxSizer(wxHORIZONTAL);
	firmwareRow1Sizer->Add(installImeFirmwareButton, 1, wxEXPAND | wxRIGHT, 8);
	firmwareRow1Sizer->Add(installM3dFirmwareButton, 1, wxEXPAND);
	wxBoxSizer *firmwareRow2Sizer = new wxBoxSizer(wxHORIZONTAL);
	firmwareRow2Sizer->Add(installFirmwareFromFileButton, 1, wxEXPAND | wxRIGHT, 8);
	firmwareRow2Sizer->Add(switchToModeButton, 1, wxEXPAND);
	firmwareRowsSizer->Add(firmwareRow1Sizer, 0, wxEXPAND | wxBOTTOM, 8);
	firmwareRowsSizer->Add(firmwareRow2Sizer, 0, wxEXPAND);
	#ifndef MACOS
		firmwareRowsSizer->Add(installDriversButton, 0, wxTOP | wxEXPAND, 8);
	#endif
	ui.firmwareSizer->Add(firmwareRowsSizer, 1, wxEXPAND | wxALL, 8);

	// Create console section
	consoleOutput = new wxTextCtrl(ui.consoleSection, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH);
	consoleOutput->SetMinSize(wxSize(520, 220));
	consoleOutput->SetFont(wxFont(
	#ifdef WINDOWS
		8
	#endif
	#ifdef MACOS
		11
	#endif
	#ifdef LINUX
		8
	#endif
	, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
	
	commandInput = new wxTextCtrl(ui.consoleSection, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
	commandInput->SetHint("Command");
	commandInput->Bind(wxEVT_TEXT_ENTER, &MyFrame::sendCommandManually, this);
	
	sendCommandButton = new wxButton(ui.consoleSection, wxID_ANY, "Send");
	sendCommandButton->Enable(false);
	sendCommandButton->Bind(wxEVT_BUTTON, &MyFrame::sendCommandManually, this);

	wxBoxSizer *consoleSectionSizer = new wxBoxSizer(wxVERTICAL);
	consoleSectionSizer->Add(consoleOutput, 1, wxEXPAND | wxBOTTOM, 8);
	wxBoxSizer *consoleCommandSizer = new wxBoxSizer(wxHORIZONTAL);
	consoleCommandSizer->Add(commandInput, 1, wxEXPAND | wxRIGHT, 8);
	consoleCommandSizer->Add(sendCommandButton, 0, wxEXPAND);
	consoleSectionSizer->Add(consoleCommandSizer, 0, wxEXPAND);
	ui.consoleSizer->Add(consoleSectionSizer, 1, wxEXPAND | wxALL, 8);

	// Create movement section
	backwardMovementButton = new wxButton(ui.movementSection, wxID_ANY, "↑");
	backwardMovementButton->Enable(false);
	backwardMovementButton->Bind(wxEVT_BUTTON, [=](wxCommandEvent& event) {
	
		// Send commands
		sendCommand("G91");
		sendCommand("G0 Y" + to_string(static_cast<double>(distanceMovementSlider->GetValue()) / 1000) + " F" + to_string(printer.convertFeedRate(feedRateMovementSlider->GetValue())));
	});
	
	leftMovementButton = new wxButton(ui.movementSection, wxID_ANY, "←");
	leftMovementButton->Enable(false);
	leftMovementButton->Bind(wxEVT_BUTTON, [=](wxCommandEvent& event) {
	
		// Send commands
		sendCommand("G91");
		sendCommand("G0 X-" + to_string(static_cast<double>(distanceMovementSlider->GetValue()) / 1000) + " F" + to_string(printer.convertFeedRate(feedRateMovementSlider->GetValue())));
	});
	
	homeMovementButton = new wxButton(ui.movementSection, wxID_ANY, "Home");
	homeMovementButton->SetMinSize(wxSize(60, -1));
	homeMovementButton->Enable(false);
	homeMovementButton->Bind(wxEVT_BUTTON, [=](wxCommandEvent& event) {
	
		// Send commands
		sendCommand("G28");
	});
	
	rightMovementButton = new wxButton(ui.movementSection, wxID_ANY, "→");
	rightMovementButton->Enable(false);
	rightMovementButton->Bind(wxEVT_BUTTON, [=](wxCommandEvent& event) {
	
		// Send commands
		sendCommand("G91");
		sendCommand("G0 X" + to_string(static_cast<double>(distanceMovementSlider->GetValue()) / 1000) + " F" + to_string(printer.convertFeedRate(feedRateMovementSlider->GetValue())));
	});
	
	forwardMovementButton = new wxButton(ui.movementSection, wxID_ANY, "↓");
	forwardMovementButton->Enable(false);
	forwardMovementButton->Bind(wxEVT_BUTTON, [=](wxCommandEvent& event) {
	
		// Send commands
		sendCommand("G91");
		sendCommand("G0 Y-" + to_string(static_cast<double>(distanceMovementSlider->GetValue()) / 1000) + " F" + to_string(printer.convertFeedRate(feedRateMovementSlider->GetValue())));
	});
	
	upMovementButton = new wxButton(ui.movementSection, wxID_ANY, "Z+");
	upMovementButton->Enable(false);
	upMovementButton->Bind(wxEVT_BUTTON, [=](wxCommandEvent& event) {
	
		// Send commands
		sendCommand("G91");
		sendCommand("G0 Z" + to_string(static_cast<double>(distanceMovementSlider->GetValue()) / 1000) + " F" + to_string(printer.convertFeedRate(feedRateMovementSlider->GetValue())));
	});
	
	downMovementButton = new wxButton(ui.movementSection, wxID_ANY, "Z-");
	downMovementButton->Enable(false);
	downMovementButton->Bind(wxEVT_BUTTON, [=](wxCommandEvent& event) {
	
		// Send commands
		sendCommand("G91");
		sendCommand("G0 Z-" + to_string(static_cast<double>(distanceMovementSlider->GetValue()) / 1000) + " F" + to_string(printer.convertFeedRate(feedRateMovementSlider->GetValue())));
	});
	
	distanceMovementText = new wxStaticText(ui.movementSection, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL | wxST_NO_AUTORESIZE);
	distanceMovementText->SetMinSize(wxSize(180, -1));

	distanceMovementSlider = new wxSlider(ui.movementSection, wxID_ANY, 10 * 1000, 0.001 * 1000, 100 * 1000);
	distanceMovementSlider->SetMinSize(wxSize(180, -1));
	distanceMovementSlider->Enable(false);
	distanceMovementSlider->Bind(wxEVT_COMMAND_SLIDER_UPDATED, [=](wxCommandEvent& event) {
	
		// Update distance movement text
		updateDistanceMovementText();
	});
	updateDistanceMovementText();
	
	feedRateMovementText = new wxStaticText(ui.movementSection, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL | wxST_NO_AUTORESIZE);
	feedRateMovementText->SetMinSize(wxSize(180, -1));

	feedRateMovementSlider = new wxSlider(ui.movementSection, wxID_ANY, DEFAULT_X_SPEED, 1, 4800);
	feedRateMovementSlider->SetMinSize(wxSize(180, -1));
	feedRateMovementSlider->Enable(false);
	feedRateMovementSlider->Bind(wxEVT_COMMAND_SLIDER_UPDATED, [=](wxCommandEvent& event) {
	
		// Update feed rate movement text
		updateFeedRateMovementText();
	});
	updateFeedRateMovementText();

	wxGridBagSizer *movementGridSizer = new wxGridBagSizer(8, 8);
	movementGridSizer->Add(backwardMovementButton, wxGBPosition(0, 1));
	movementGridSizer->Add(leftMovementButton, wxGBPosition(1, 0));
	movementGridSizer->Add(homeMovementButton, wxGBPosition(1, 1));
	movementGridSizer->Add(rightMovementButton, wxGBPosition(1, 2));
	movementGridSizer->Add(forwardMovementButton, wxGBPosition(2, 1));
	movementGridSizer->Add(upMovementButton, wxGBPosition(0, 3));
	movementGridSizer->Add(downMovementButton, wxGBPosition(1, 3));
	movementGridSizer->AddGrowableCol(0);
	movementGridSizer->AddGrowableCol(1);
	movementGridSizer->AddGrowableCol(2);
	movementGridSizer->AddGrowableCol(3);
	wxBoxSizer *movementSectionSizer = new wxBoxSizer(wxVERTICAL);
	movementSectionSizer->Add(movementGridSizer, 0, wxALIGN_CENTER_HORIZONTAL | wxBOTTOM, 8);
	movementSectionSizer->Add(distanceMovementText, 0, wxEXPAND | wxBOTTOM, 4);
	movementSectionSizer->Add(distanceMovementSlider, 0, wxEXPAND | wxBOTTOM, 8);
	movementSectionSizer->Add(feedRateMovementText, 0, wxEXPAND | wxBOTTOM, 4);
	movementSectionSizer->Add(feedRateMovementSlider, 0, wxEXPAND);
	ui.movementSizer->Add(movementSectionSizer, 1, wxEXPAND | wxALL, 8);

	// Create settings section
	
	// Create printer settings
	wxArrayString printerSettings;
	vector<string> temp = printer.getEepromSettingsNames();
	for(uint8_t i = 0; i < temp.size(); i++)
		printerSettings.Add(temp[i]);

	printerSettingChoice = new wxChoice(ui.settingsSection, wxID_ANY, wxDefaultPosition, wxDefaultSize, printerSettings);
	printerSettingChoice->SetSelection(printerSettingChoice->FindString(printerSettings[0]));
	printerSettingChoice->Enable(false);
	printerSettingChoice->Bind(wxEVT_CHOICE, [=](wxCommandEvent& event) {
	
		// Set printer settings value
		setPrinterSettingValue();
	});

	printerSettingInput = new wxTextCtrl(ui.settingsSection, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
	printerSettingInput->SetHint("Value");
	printerSettingInput->Bind(wxEVT_TEXT_ENTER, &MyFrame::savePrinterSetting, this);
	
	savePrinterSettingButton = new wxButton(ui.settingsSection, wxID_ANY, "Save");
	savePrinterSettingButton->Enable(false);
	savePrinterSettingButton->Bind(wxEVT_BUTTON, &MyFrame::savePrinterSetting, this);

	wxBoxSizer *settingsSectionSizer = new wxBoxSizer(wxVERTICAL);
	settingsSectionSizer->Add(new wxStaticText(ui.settingsSection, wxID_ANY, "Printer Setting"), 0, wxBOTTOM, 4);
	settingsSectionSizer->Add(printerSettingChoice, 0, wxEXPAND | wxBOTTOM, 8);
	wxBoxSizer *settingsInputSizer = new wxBoxSizer(wxHORIZONTAL);
	settingsInputSizer->Add(printerSettingInput, 1, wxEXPAND | wxRIGHT, 8);
	settingsInputSizer->Add(savePrinterSettingButton, 0, wxEXPAND);
	settingsSectionSizer->Add(settingsInputSizer, 0, wxEXPAND);
	ui.settingsSizer->Add(settingsSectionSizer, 1, wxEXPAND | wxALL, 8);

	// Create miscellaneous section
	motorsOnButton = new wxButton(ui.miscellaneousSection, wxID_ANY, "Motors on");
	motorsOnButton->Enable(false);
	motorsOnButton->Bind(wxEVT_BUTTON, [=](wxCommandEvent& event) {
	
		// Send commands
		sendCommand("M17");
	});
	
	motorsOffButton = new wxButton(ui.miscellaneousSection, wxID_ANY, "Motors off");
	motorsOffButton->Enable(false);
	motorsOffButton->Bind(wxEVT_BUTTON, [=](wxCommandEvent& event) {
	
		// Send commands
		sendCommand("M18");
	});
	
	fanOnButton = new wxButton(ui.miscellaneousSection, wxID_ANY, "Fan on");
	fanOnButton->Enable(false);
	fanOnButton->Bind(wxEVT_BUTTON, [=](wxCommandEvent& event) {
	
		// Send commands
		sendCommand("M106 S255");
	});
	
	fanOffButton = new wxButton(ui.miscellaneousSection, wxID_ANY, "Fan off");
	fanOffButton->Enable(false);
	fanOffButton->Bind(wxEVT_BUTTON, [=](wxCommandEvent& event) {
	
		// Send commands
		sendCommand("M107");
	});

	wxBoxSizer *miscellaneousSectionSizer = new wxBoxSizer(wxVERTICAL);
	miscellaneousSectionSizer->Add(motorsOnButton, 0, wxEXPAND | wxBOTTOM, 8);
	miscellaneousSectionSizer->Add(motorsOffButton, 0, wxEXPAND | wxBOTTOM, 8);
	miscellaneousSectionSizer->Add(fanOnButton, 0, wxEXPAND | wxBOTTOM, 8);
	miscellaneousSectionSizer->Add(fanOffButton, 0, wxEXPAND);
	ui.miscellaneousSizer->Add(miscellaneousSectionSizer, 1, wxEXPAND | wxALL, 8);

	// Create calibration section
	calibrateBedPositionButton = new wxButton(ui.calibrationSection, wxID_ANY, "Calibrate bed position");
	calibrateBedPositionButton->Enable(false);
	calibrateBedPositionButton->Bind(wxEVT_BUTTON, [=](wxCommandEvent& event) {
	
		// Send commands
		sendCommand("G91");
		sendCommand("G0 Z3 F90");
		sendCommand("G90");
		sendCommand("M109 S150");
		sendCommand("M104 S0");
		sendCommand("M107");
		sendCommand("G30");
	});
	
	calibrateBedOrientationButton = new wxButton(ui.calibrationSection, wxID_ANY, "Calibrate bed orientation");
	calibrateBedOrientationButton->Enable(false);
	calibrateBedOrientationButton->Bind(wxEVT_BUTTON, [=](wxCommandEvent& event) {
	
		// Send commands
		sendCommand("G90");
		sendCommand("G0 Z3 F90");
		sendCommand("M109 S150");
		sendCommand("M104 S0");
		sendCommand("M107");
		sendCommand("G32");
	});
	
	saveZAsZeroButton = new wxButton(ui.calibrationSection, wxID_ANY, "Save Z as 0");
	saveZAsZeroButton->Enable(false);
	saveZAsZeroButton->Bind(wxEVT_BUTTON, [=](wxCommandEvent& event) {
	
		//sendCommand("M618 S" + eepromOffsets["bedHeightOffset"]["offset"] + " T" + eepromOffsets["bedHeightOffset"]["bytes"] + " P" + floatToBinary(0),
		//sendCommand("M619 S" + eepromOffsets["bedHeightOffset"]["offset"] + " T" + eepromOffsets["bedHeightOffset"]["bytes"],
	
		// Send commands
		if(printer.getFirmwareType() == M3D || printer.getFirmwareType() == M3D_MOD) {
			sendCommand("G91");
			sendCommand("G0 Z0.1 F90");
		}
	
		sendCommand("G33");
	});

	wxBoxSizer *calibrationSectionSizer = new wxBoxSizer(wxVERTICAL);
	calibrationSectionSizer->Add(calibrateBedPositionButton, 0, wxEXPAND | wxBOTTOM, 8);
	calibrationSectionSizer->Add(calibrateBedOrientationButton, 0, wxEXPAND | wxBOTTOM, 8);
	calibrationSectionSizer->Add(saveZAsZeroButton, 0, wxEXPAND);
	ui.calibrationSizer->Add(calibrationSectionSizer, 1, wxEXPAND | wxALL, 8);

	// Create version text
	versionText = new wxStaticText(ui.footerSection, wxID_ANY, "M33 Manager V" TOSTRING(VERSION), wxDefaultPosition, wxSize(
	#ifdef WINDOWS
		-1, 15
	#endif
	#ifdef MACOS
		-1, 14
	#endif
	#ifdef LINUX
		-1, 13
	#endif
	), wxALIGN_RIGHT);
	wxFont font = versionText->GetFont();
	font.SetPointSize(
	#ifdef WINDOWS
		9
	#endif
	#ifdef MACOS
		11
	#endif
	#ifdef LINUX
		9
	#endif
	);
	versionText->SetFont(font);
	ui.footerSizer->AddStretchSpacer(1);
	ui.footerSizer->Add(versionText, 0, wxALIGN_RIGHT);

	setStatusRowVisible(false);
	setConnectedUiVisible(false);
	Layout();
	
	// Thread task events
	Bind(wxEVT_THREAD_TASK_START, &MyFrame::threadTaskStart, this);
	Bind(wxEVT_THREAD_TASK_COMPLETE, &MyFrame::threadTaskComplete, this);
	
	// Check if creating thread failed
	wxThreadError createThreadResult = CreateThread(wxTHREAD_JOINABLE);
	wxThreadError runThreadResult = createThreadResult == wxTHREAD_NO_ERROR ? GetThread()->Run() : wxTHREAD_NO_RESOURCE;
	if(createThreadResult != wxTHREAD_NO_ERROR || runThreadResult != wxTHREAD_NO_ERROR)
	
		// Close
		Close();
	
	// Check if using Windows
	#ifdef WINDOWS
	
		// Monitor device notifications
		DEV_BROADCAST_DEVICEINTERFACE notificationFilter;
		SecureZeroMemory(&notificationFilter, sizeof(notificationFilter));
		notificationFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
		notificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
		RegisterDeviceNotification(GetHWND(), &notificationFilter, DEVICE_NOTIFY_WINDOW_HANDLE | DEVICE_NOTIFY_ALL_INTERFACE_CLASSES);
	#endif
}

void MyFrame::show(wxShowEvent &event) {
	event.Skip();
}

wxThread::ExitCode MyFrame::Entry() {

	// Loop until destroyed
	while(!GetThread()->TestDestroy()) {
		
		// Initialize thread task
		function<ThreadTaskResponse()> threadTask;
		
		{
			// Lock
			wxCriticalSectionLocker lock(criticalLock);
			
			// Check if a thread task exists
			if(!threadTaskQueue.empty()) {
	
				// Set thread task to next thread task function
				threadTask = threadTaskQueue.front();
				threadTaskQueue.pop();
			}
		}
		
		// Check if thread task exists
		if(threadTask) {
		
			// Trigger thread start event
			wxQueueEvent(GetEventHandler(), new wxThreadEvent(wxEVT_THREAD_TASK_START));
			
			// Perform thread task
			ThreadTaskResponse response = threadTask();
		
			// Trigger thread complete event
			wxThreadEvent *event = new wxThreadEvent(wxEVT_THREAD_TASK_COMPLETE);
			event->SetPayload(response);
			wxQueueEvent(GetEventHandler(), event);
		}
		
		// Delay
		sleepUs(10000);
	}
	
	// Return
	return EXIT_SUCCESS;
}

void MyFrame::threadTaskStart(wxThreadEvent& event) {

	// Initialize callback function
	function<void()> callbackFunction;
	
	{
		// Lock
		wxCriticalSectionLocker lock(criticalLock);
		
		// Check if a thread start callback is specified and one exists
		if(!threadStartCallbackQueue.empty()) {
	
			// Set callback function to next start callback function
			callbackFunction = threadStartCallbackQueue.front();
			threadStartCallbackQueue.pop();
		}
	}

	// Run callback function if it exists
	if(callbackFunction)
		callbackFunction();
}

void MyFrame::threadTaskComplete(wxThreadEvent& event) {

	// Initialize callback function
	function<void(ThreadTaskResponse response)> callbackFunction;
	
	{
		// Lock
		wxCriticalSectionLocker lock(criticalLock);
		
		// Check if a thread complete callback is specified and one exists
		if(!threadCompleteCallbackQueue.empty()) {
	
			// Set callback function to next complete callback function
			callbackFunction = threadCompleteCallbackQueue.front();
			threadCompleteCallbackQueue.pop();
		}
	}

	// Run callback function if it exists
	if(callbackFunction)
		callbackFunction(event.GetPayload<ThreadTaskResponse>());
}

void MyFrame::close(wxCloseEvent& event) {
	// Stop status timer
	statusTimer->Stop();

	// Disconnect printer
	printer.disconnect();
	
	// Check if thread is running
	if(GetThread() && GetThread()->IsRunning())
	
		// Destroy thread
		GetThread()->Delete();
	
	// Destroy self
	Destroy();
}

void MyFrame::changePrinterConnection(wxCommandEvent& event) {
	
	// Disable button that triggered event
	FindWindowById(event.GetId())->Enable(false);

	// Check if connecting to printer
	if(connectionButton->GetLabel() == "Connect") {
		
		// Get current serial port choice
		string currentChoice = static_cast<string>(serialPortChoice->GetString(serialPortChoice->GetSelection()));

		// Lock
		wxCriticalSectionLocker lock(criticalLock);

		// Append thread start callback to queue
		threadStartCallbackQueue.push([=]() -> void {
		
			// Stop status timer
			statusTimer->Stop();
		
			// Disable connection controls
			enableConnectionControls(false);

			// Set status text
			setStatusRowVisible(true);
			statusText->SetLabel("Connecting");
			statusText->SetForegroundColour(wxColour(255, 180, 0));
			refreshWindowLayout();
			
			// Clear allow enabling controls
			allowEnablingControls = false;
		});
		
		// Append thread task to queue
		threadTaskQueue.push([=]() -> ThreadTaskResponse {
		
			// Connect to printer
			printer.connect(currentChoice != "Auto" ? currentChoice : "");
			
			// Return empty response
			return {"", 0};
		});

		// Append thread complete callback to queue
		threadCompleteCallbackQueue.push([=](ThreadTaskResponse response) -> void {
		
			// Set allow enabling controls
			allowEnablingControls = true;
	
			// Enable connection button
			connectionButton->Enable(true);

			// Check if connected to printer
			if(printer.isConnected()) {

				// Show connected-only content
				setStatusRowVisible(true);
				setConnectedUiVisible(true);

				// Enable firmware controls
				enableFirmwareControls(true);
				
				// Change connection button to disconnect	
				connectionButton->SetLabel("Disconnect");
				refreshWindowLayout();
				
				// Log printer mode
				logToConsole(static_cast<string>("Printer is in ") + (printer.getOperatingMode() == BOOTLOADER ? "bootloader" : "firmware") + " mode");
				
				// Start status timer
				statusTimer->Start(100);
				
				// Check invalid values
				checkInvalidValues();
			}
			
			// Otherwise
			else {
			
				// Restore compact disconnected layout
				setConnectedUiVisible(false);
				setStatusRowVisible(false);

				// Enable connection controls
				enableConnectionControls(true);
				refreshWindowLayout();
				
				// Start status timer
				statusTimer->Start(100);
			}
		});
	}
	
	// Otherwise
	else {
		
		// Lock
		wxCriticalSectionLocker lock(criticalLock);

		// Append thread start callback to queue
		threadStartCallbackQueue.push([=]() -> void {
		
			// Stop status timer
			statusTimer->Stop();
		});
		
		// Append thread task to queue
		threadTaskQueue.push([=]() -> ThreadTaskResponse {
		
			// Disconnect printer
			printer.disconnect();
			
			// Return empty response
			return {"", 0};
		});

		// Append thread complete callback to queue
		threadCompleteCallbackQueue.push([=](ThreadTaskResponse response) -> void {
	
			// Restore compact disconnected layout
			setConnectedUiVisible(false);
			setStatusRowVisible(false);

			// Disable firmware controls
			enableFirmwareControls(false);
			
			// Disable movement controls
			enableMovementControls(false);
			
			// Disable settings controls
			enableSettingsControls(false);
			
			// Disable miscellaneous controls
			enableMiscellaneousControls(false);
			
			// Disable calibration controls
			enableCalibrationControls(false);
		
			// Change connection button to connect
			connectionButton->SetLabel("Connect");
		
			// Enable connection controls
			enableConnectionControls(true);
			refreshWindowLayout();
		
			// Start status timer
			statusTimer->Start(100);
		});
	}
}

void MyFrame::switchToMode(wxCommandEvent& event) {

	// Disable button that triggered event
	FindWindowById(event.GetId())->Enable(false);
	
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
	
		// Check if switching to firmware mode
		if(newOperatingMode == FIRMWARE)
	
			// Put printer into firmware mode
			printer.switchToFirmwareMode();
		
		// Otherwise
		else
		
			// Put printer into bootloader mode
			printer.switchToBootloaderMode();

		// Check if printer isn't connected
		if(!printer.isConnected())
		
			// Return message
			return {static_cast<string>("Failed to switch printer into ") + (newOperatingMode == FIRMWARE ? "firmware" : "bootloader") + " mode", wxOK | wxICON_ERROR | wxCENTRE};
	
		// Otherwise
		else
		
			// Return message
			return {static_cast<string>("Printer has been successfully switched into ") + (newOperatingMode == FIRMWARE ? "firmware" : "bootloader") + " mode and is connected at " + printer.getCurrentSerialPort() + " running at a baud rate of " TOSTRING(PRINTER_BAUD_RATE), wxOK | wxICON_INFORMATION | wxCENTRE};
	});
	
	// Append thread complete callback to queue
	threadCompleteCallbackQueue.push([=](ThreadTaskResponse response) -> void {
		
		// Enable connection button
		connectionButton->Enable(true);

		// Check if connected to printer
		if(printer.isConnected())

			// Enable firmware controls
			enableFirmwareControls(true);

		// Otherwise
		else

			// Enable connection controls
			enableConnectionControls(true);
		
		// Display message
		wxMessageBox(response.message, "M33 Manager", response.style);
	});
}

void MyFrame::installImeFirmware(wxCommandEvent& event) {

	// Disable button that triggered event
	FindWindowById(event.GetId())->Enable(false);

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
	
		// Set firmware location
		string firmwareLocation = getTemporaryLocation() + "iMe " TOSTRING(IME_ROM_VERSION_STRING) ".hex";

		// Check if creating iMe ROM failed
		ofstream fout(firmwareLocation, ios::binary);
		if(fout.fail())
		
			// Return message
			return {"Failed to unpack iMe firmware", wxOK | wxICON_ERROR | wxCENTRE};

		// Otherwise
		else {

			// Unpack iMe ROM
			for(uint64_t i = 0; i < IME_HEX_SIZE; i++)
				fout.put(IME_HEX_DATA[i]);
			fout.close();
		
			// Return install firmware's message
			return installFirmware(firmwareLocation);
		}
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

void MyFrame::installM3dFirmware(wxCommandEvent& event) {

	// Disable button that triggered event
	FindWindowById(event.GetId())->Enable(false);

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
	
		// Set firmware location
		string firmwareLocation = getTemporaryLocation() + "M3D " TOSTRING(M3D_ROM_VERSION_STRING) ".hex";

		// Check if creating M3D ROM failed
		ofstream fout(firmwareLocation, ios::binary);
		if(fout.fail())
		
			// Return message
			return {"Failed to unpack M3D firmware", wxOK | wxICON_ERROR | wxCENTRE};

		// Otherwise
		else {

			// Unpack M3D ROM
			for(uint64_t i = 0; i < M3D_HEX_SIZE; i++)
				fout.put(M3D_HEX_DATA[i]);
			fout.close();
		
			// Return install firmware's message
			return installFirmware(firmwareLocation);
		}
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

void MyFrame::installFirmwareFromFile(wxCommandEvent& event) {
	
	// Display file dialog
	wxFileDialog *openFileDialog = new wxFileDialog(this, "Open firmware file", wxEmptyString, wxEmptyString, "Firmware files|*.hex;*.bin;*.rom|All files|*", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	
	// Check if a file was selected
	if(openFileDialog->ShowModal() == wxID_OK) {
	
		// Set firmware location
		string firmwareLocation = static_cast<string>(openFileDialog->GetPath());
		
		// Disable button that triggered event
		FindWindowById(event.GetId())->Enable(false);

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
		
			// Return install firmware's message
			return installFirmware(firmwareLocation);
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

void MyFrame::installDrivers(wxCommandEvent& event) {

	// Disable button that triggered event
	FindWindowById(event.GetId())->Enable(false);

	// Lock
	wxCriticalSectionLocker lock(criticalLock);

	// Append thread start callback to queue
	threadStartCallbackQueue.push([=]() -> void {
	
		// Disable install drivers button
		installDriversButton->Enable(false);
	});
	
	// Append thread task to queue
	threadTaskQueue.push([=]() -> ThreadTaskResponse {
	
		// Check if using Windows
		#ifdef WINDOWS

			// Check if creating drivers file failed
			ofstream fout(getTemporaryLocation() + "M3D_v2.cat", ios::binary);
			if(fout.fail())
			
				// Return message
				return {"Failed to unpack drivers", wxOK | wxICON_ERROR | wxCENTRE};

			// Otherwise
			else {

				// Unpack drivers
				for(uint64_t i = 0; i < m3DV2CatSize; i++)
					fout.put(m3DV2CatData[i]);
				fout.close();

				// Check if creating drivers file failed
				fout.open(getTemporaryLocation() + "M3D_v2.inf", ios::binary);
				if(fout.fail())
				
					// Return message
					return {"Failed to unpack drivers", wxOK | wxICON_ERROR | wxCENTRE};

				// Otherwise
				else {

					// Unpack drivers
					for(uint64_t i = 0; i < m3DV2InfSize; i++)
						fout.put(m3DV2InfData[i]);
					fout.close();

					// Check if creating process failed
					TCHAR buffer[MAX_PATH];
					GetWindowsDirectory(buffer, MAX_PATH);
					wstring path = buffer;

					string executablePath;
					ifstream file(path + "\\sysnative\\pnputil.exe", ios::binary);
					executablePath = file.good() ? "sysnative" : "System32";
					file.close();

					STARTUPINFO startupInfo;
					SecureZeroMemory(&startupInfo, sizeof(startupInfo));
					startupInfo.cb = sizeof(startupInfo);

					PROCESS_INFORMATION processInfo;
					SecureZeroMemory(&processInfo, sizeof(processInfo));

					TCHAR command[MAX_PATH];
					_tcscpy(command, (path + "\\" + executablePath + "\\pnputil.exe -i -a \"" + getTemporaryLocation() + "M3D_v2.inf\"").c_str());

					if(!CreateProcess(nullptr, command, nullptr, nullptr, false, CREATE_NO_WINDOW, nullptr, nullptr, &startupInfo, &processInfo))
					
						// Return message
						return {"Failed to install drivers", wxOK | wxICON_ERROR | wxCENTRE};

					// Otherwise
					else {

						// Wait until process finishes
						WaitForSingleObject(processInfo.hProcess, INFINITE);

						// Check if installing drivers failed
						DWORD exitCode;
						GetExitCodeProcess(processInfo.hProcess, &exitCode);

						// Close process and thread handles. 
						CloseHandle(processInfo.hProcess);
						CloseHandle(processInfo.hThread);

						if(!exitCode)
						
							// Return message
							return {"Failed to install drivers", wxOK | wxICON_ERROR | wxCENTRE};

						// Otherwise
						else
						
							// Return message
							return {"Drivers successfully installed. You might need to reconnect the printer to the computer for the drivers to take effect.", wxOK | wxICON_INFORMATION | wxCENTRE};
					}
				}
			}
		#endif

		// Otherwise check if using Linux
		#ifdef LINUX

			// Check if user is not root
			if(getuid())
			
				// Return message
				return {"Elevated privileges required", wxOK | wxICON_ERROR | wxCENTRE};

			// Otherwise
			else {

				// Check if creating udev rule failed
				ofstream fout("/etc/udev/rules.d/90-micro-3d-local.rules", ios::binary);
				if(fout.fail())
				
					// Return message
					return {"Failed to unpack udev rule", wxOK | wxICON_ERROR | wxCENTRE};

				// Otherwise
				else {

					// Unpack udev rule
					for(uint64_t i = 0; i < _90Micro3dLocalRulesSize; i++)
						fout.put(_90Micro3dLocalRulesData[i]);
					fout.close();

					// Check if creating udev rule failed
					fout.open("/etc/udev/rules.d/91-micro-3d-heatbed-local.rules", ios::binary);
					if(fout.fail())
				
						// Return message
						return {"Failed to unpack udev rule", wxOK | wxICON_ERROR | wxCENTRE};

					// Otherwise
					else {

						// Unpack udev rule
						for(uint64_t i = 0; i < _91Micro3dHeatbedLocalRulesSize; i++)
							fout.put(_91Micro3dHeatbedLocalRulesData[i]);
						fout.close();
						
						// Check if creating udev rule failed
						fout.open("/etc/udev/rules.d/92-m3d-pro-local.rules", ios::binary);
						if(fout.fail())
				
							// Return message
							return {"Failed to unpack udev rule", wxOK | wxICON_ERROR | wxCENTRE};

						// Otherwise
						else {

							// Unpack udev rule
							for(uint64_t i = 0; i < _92M3dProLocalRulesSize; i++)
								fout.put(_92M3dProLocalRulesData[i]);
							fout.close();
							
							// Check if creating udev rule failed
							fout.open("/etc/udev/rules.d/93-micro+-local.rules", ios::binary);
							if(fout.fail())
				
								// Return message
								return {"Failed to unpack udev rule", wxOK | wxICON_ERROR | wxCENTRE};

							// Otherwise
							else {

								// Unpack udev rule
								for(uint64_t i = 0; i < _93Micro_localRulesSize; i++)
									fout.put(_93Micro_localRulesData[i]);
								fout.close();
						
								// Check if applying udev rule failed
								if(system("udevadm control --reload-rules") || system("udevadm trigger"))
					
									// Return message
									return {"Failed to apply udev rule", wxOK | wxICON_ERROR | wxCENTRE};
	
								// Otherwise
								else
					
									// Return message
									return {"Drivers successfully installed. You might need to reconnect the printer to the computer for the drivers to take effect.", wxOK | wxICON_INFORMATION | wxCENTRE};
							}
						}
					}
				}
			}
		#endif
		
		// Return empty response
		return {"", 0};
	});
	
	// Append thread complete callback to queue
	threadCompleteCallbackQueue.push([=](ThreadTaskResponse response) -> void {
	
		// Enable install drivers button
		installDriversButton->Enable(true);
	
		// Display message
		wxMessageBox(response.message, "M33 Manager", response.style);
	});
}

void MyFrame::logToConsole(const string &message) {

	// Lock
	wxCriticalSectionLocker lock(criticalLock);

	// Append message to log queue
	logQueue.push(message);
}

void MyFrame::updateLog(wxTimerEvent& event) {

	// Loop forever
	while(true) {
	
		// Initialize message
		string message;
	
		{
			// Lock
			wxCriticalSectionLocker lock(criticalLock);
			
			// Check if no messages exists in log queue
			if(logQueue.empty())
			
				// Break
				break;
			
			// Set message to next message in log queue
			message = logQueue.front();
			logQueue.pop();
		}
		
		// Check if message exists
		if(!message.empty()) {
		
			// Check if message is to remove last line
			if(message == "Remove last line") {
		
				// Remove last line
				size_t end = consoleOutput->GetValue().Length();
				size_t start = consoleOutput->GetValue().find_last_of('\n');
				consoleOutput->Remove(start != string::npos ? start : 0, end);
			}
		
			// Otherwise
			else {
			
				// Check if not fixing invalid values
				if(!fixingInvalidValues) {
			
					// Check if printer is switching modes
					if(message == "Switching printer into bootloader mode" || message == "Switching printer into firmware mode") {
				
						// Disable movement controls
						enableMovementControls(false);
					
						// Disable settings controls
						enableSettingsControls(false);
					
						// Disable miscellaneous controls
						enableMiscellaneousControls(false);
						
						// Disable calibration controls
						enableCalibrationControls(false);
					}
				
					// Otherwise check if printer is in firmware mode
					else if(message == "Printer is in firmware mode") {
			
						// Set switch mode button label
						switchToModeButton->SetLabel("Switch to bootloader mode");
					
						// Check if controls can be enabled
						if(allowEnablingControls) {
					
							// Enable movement controls
							enableMovementControls(true);
						
							// Disable settings controls
							enableSettingsControls(false);
						
							// Enable miscellaneous controls
							enableMiscellaneousControls(true);
							
							// Enable calibration controls
							enableCalibrationControls(true);
						}
					}
				
					// Otherwise check if printer is in bootloader mode
					else if(message == "Printer is in bootloader mode") {
				
						// Set switch mode button label
						switchToModeButton->SetLabel("Switch to firmware mode");
					
						// Check if controls can be enabled
						if(allowEnablingControls) {
					
							// Disable movement controls
							enableMovementControls(false);
						
							// Enable settings controls
							enableSettingsControls(true);
						
							// Disable miscellaneous controls
							enableMiscellaneousControls(false);
							
							// Disable calibration controls
							enableCalibrationControls(false);
						}
					}
				
					// Otherwise check if the printer has been disconnected
					else if(message == "Printer has been disconnected") {
				
						// Check if controls can be enabled
						if(allowEnablingControls) {
				
							// Disable movement controls
							enableMovementControls(false);
						
							// Disable settings controls
							enableSettingsControls(false);
						
							// Disable miscellaneous controls
							enableMiscellaneousControls(false);
							
							// Disable calibration controls
							enableCalibrationControls(false);
						}
					}
				}
		
				// Append message to console's output
				consoleOutput->AppendText(static_cast<string>(consoleOutput->GetValue().IsEmpty() ? "" : "\n") + ">> " + message);
			}
		
			// Scroll to bottom
			consoleOutput->ShowPosition(consoleOutput->GetLastPosition());
		}
	}
}

void MyFrame::updateStatus(wxTimerEvent& event) {

	// Check if getting printer status was successful
	string status = printer.getStatus();
	if(!status.empty()) {
		
		// Check if printer is connected
		if(status == "Connected") {
	
			// Change connection button to disconnect
			if(connectionButton->GetLabel() != "Disconnect")
				connectionButton->SetLabel("Disconnect");

			// Show connected layout and status row
			setStatusRowVisible(true);
			setConnectedUiVisible(true);
		
			// Disable connection controls
			enableConnectionControls(false);
			
			// Enable connection button
			connectionButton->Enable(true);
		
			// Set status text color
			statusText->SetForegroundColour(wxColour(0, 255, 0));
		}
	
		// Otherwise
		else {
	
			// Check if printer was just disconnected
			if(status == "Disconnected" && (statusText->GetLabel() != status || connectionButton->GetLabel() == "Disconnect")) {

				// Restore compact disconnected layout
				setConnectedUiVisible(false);
				setStatusRowVisible(false);
				
				// Disable firmware controls
				enableFirmwareControls(false);
				
				// Disable movement controls
				enableMovementControls(false);
				
				// Disable settings controls
				enableSettingsControls(false);
				
				// Disable miscellaneous controls
				enableMiscellaneousControls(false);
				
				// Disable calibration controls
				enableCalibrationControls(false);
		
				// Change connection button to connect
				connectionButton->SetLabel("Connect");
		
				// Enable connection controls
				enableConnectionControls(true);
				refreshWindowLayout();
			
				// Log disconnection
				logToConsole("Printer has been disconnected");
			}
		
			// Set status text color
			statusText->SetForegroundColour(wxColour(255, 0, 0));
		}

		// Update status text
		statusText->SetLabel(status);
	}
}

wxArrayString MyFrame::getAvailableSerialPorts() {

	// Initialize serial ports
	wxArrayString serialPorts;
	serialPorts.Add("Auto");
	
	// Get available serial ports
	vector<string> availableSerialPorts = printer.getAvailableSerialPorts();
	for(uint8_t i = 0; i < availableSerialPorts.size(); i++)
		serialPorts.Add(availableSerialPorts[i].c_str());
	
	// Return serial ports
	return serialPorts;
}

void MyFrame::refreshSerialPorts(wxCommandEvent& event) {

	// Disable button that triggered event
	FindWindowById(event.GetId())->Enable(false);
	
	// Lock
	wxCriticalSectionLocker lock(criticalLock);

	// Append thread start callback to queue
	threadStartCallbackQueue.push([=]() -> void {
	
		// Disable connection controls
		enableConnectionControls(false);
	});
	
	// Append thread task to queue
	threadTaskQueue.push([=]() -> ThreadTaskResponse {
	
		// Delay
		sleepUs(300000);
	
		// Get available serial ports
		wxArrayString serialPorts = getAvailableSerialPorts();
		
		// Combine serial ports into a single response
		string response;
		for(size_t i = 0; i < serialPorts.GetCount(); i++) 
			response += serialPorts[i] + ' ';
		response.pop_back();
		
		// Return serial ports
		return {response, 0};
	});
	
	// Append thread complete callback to queue
	threadCompleteCallbackQueue.push([=](ThreadTaskResponse response) -> void {
	
		// Enable connection controls
		enableConnectionControls(true);
		
		// Get serial ports from response
		wxArrayString serialPorts;
		for(size_t i = 0; i < response.message.length(); i++) {
		
			size_t startingOffset = i;
			i = response.message.find(' ', i);
			serialPorts.Add(response.message.substr(startingOffset, i));
			
			if(i == string::npos)
				break;
		}
		
		// Save current choice
		wxString currentChoice = serialPortChoice->GetString(serialPortChoice->GetSelection());

		// Refresh choices
		serialPortChoice->Clear();
		serialPortChoice->Append(serialPorts);
	
		// Set choice to auto if previous choice doesn't exist anymore
		if(serialPortChoice->FindString(currentChoice) == wxNOT_FOUND)
			currentChoice = "Auto";
	
		// Select choice
		serialPortChoice->SetSelection(serialPortChoice->FindString(currentChoice));
	});
}

void MyFrame::updateDistanceMovementText() {

	// Set distance movement text to current distance
	stringstream stream;
	stream << fixed << setprecision(3) << static_cast<double>(distanceMovementSlider->GetValue()) / 1000;
	distanceMovementText->SetLabel("Distance: " + stream.str() + "mm");
}

void MyFrame::updateFeedRateMovementText() {

	// Set feed rate movement text to current feed rate
	feedRateMovementText->SetLabel("Feed rate: " + to_string(feedRateMovementSlider->GetValue()) + "mm/min");
}

void MyFrame::setConnectedUiVisible(bool visible) {

	// Show or hide all connected-only content
	ui.connectedContent->Show(visible);
}

void MyFrame::setStatusRowVisible(bool visible) {

	// Show or hide the status row in the connection section
	ui.statusRow->Show(visible);
}

void MyFrame::refreshWindowLayout() {

	// Recompute layout and resize the frame to the current content
	SetMinSize(wxSize(-1, -1));
	ui.rootPanel->Layout();
	Layout();
	GetSizer()->Fit(this);
	SetMinSize(GetSize());
}

void MyFrame::enableConnectionControls(bool enable) {

	// Enable or disable connection controls
	serialPortChoice->Enable(enable);
	refreshSerialPortsButton->Enable(enable);
	connectionButton->Enable(enable);
}

void MyFrame::enableFirmwareControls(bool enable) {

	// Enable or disable firmware controls
	installFirmwareFromFileButton->Enable(enable);
	installImeFirmwareButton->Enable(enable);
	installM3dFirmwareButton->Enable(enable);
	switchToModeButton->Enable(enable);
	sendCommandButton->Enable(enable);
}

void MyFrame::enableMovementControls(bool enable) {

	// Enable or disable movement controls
	backwardMovementButton->Enable(enable);
	forwardMovementButton->Enable(enable);
	rightMovementButton->Enable(enable);
	leftMovementButton->Enable(enable);
	upMovementButton->Enable(enable);
	downMovementButton->Enable(enable);
	homeMovementButton->Enable(enable);
	distanceMovementSlider->Enable(enable);
	feedRateMovementSlider->Enable(enable);
}

void MyFrame::enableSettingsControls(bool enable) {

	// Enable or disable settings controls
	printerSettingChoice->Enable(enable);
	savePrinterSettingButton->Enable(enable);
	
	// Check if enabling settings controls
	if(enable) {
	
		// Refresh EEPROM
		printer.collectPrinterInformation(false);
	
		// Set printer setting value
		setPrinterSettingValue();
	}
}

void MyFrame::enableMiscellaneousControls(bool enable) {

	// Enable or disable miscellaneous controls
	motorsOnButton->Enable(enable);
	motorsOffButton->Enable(enable);
	fanOnButton->Enable(enable);
	fanOffButton->Enable(enable);
}

void MyFrame::enableCalibrationControls(bool enable) {

	// Enable or disable calibration controls
	calibrateBedPositionButton->Enable(enable);
	calibrateBedOrientationButton->Enable(enable);
	saveZAsZeroButton->Enable(enable);
}

ThreadTaskResponse MyFrame::installFirmware(const string &firmwareLocation) {

	// Check if firmware ROM doesn't exists
	ifstream file(firmwareLocation, ios::binary);
	if(!file.good())
	
		// Return message
		return {"Firmware ROM doesn't exist", wxOK | wxICON_ERROR | wxCENTRE};

	// Otherwise
	else {
	
		// Check if firmware ROM name is valid
		uint8_t endOfNumbers = 0;
		if(firmwareLocation.find_last_of('.') != string::npos)
			endOfNumbers = firmwareLocation.find_last_of('.') - 1;
		else
			endOfNumbers = firmwareLocation.length() - 1;
	
		int8_t beginningOfNumbers = endOfNumbers;
		for(; beginningOfNumbers >= 0 && isdigit(firmwareLocation[beginningOfNumbers]); beginningOfNumbers--);

		if(beginningOfNumbers != endOfNumbers - 10)
		
			// Return message
			return {"Invalid firmware name", wxOK | wxICON_ERROR | wxCENTRE};

		// Otherwise
		else {

			// Check if installing printer's firmware failed
			if(!printer.installFirmware(firmwareLocation.c_str()))
			
				// Return message
				return {"Failed to update firmware", wxOK | wxICON_ERROR | wxCENTRE};

			// Otherwise
			else

				// Return message
				return {"Firmware successfully installed", wxOK | wxICON_INFORMATION | wxCENTRE};
		}
	}
}

void MyFrame::sendCommandManually(wxCommandEvent& event) {

	// Check if commands can be sent
	if(sendCommandButton->IsEnabled()) {
	
		// Check if command exists
		string command = static_cast<string>(commandInput->GetValue());
		if(!command.empty()) {
		
			// Clear command input
			commandInput->SetValue("");
			
			// Initialize thread start and complete callbacks
			function<void()> threadStartCallback = nullptr;
			function<void(ThreadTaskResponse response)> threadCompleteCallback = nullptr;
			
			// Check if switching modes
			if((command == "Q" && printer.getOperatingMode() == BOOTLOADER) || (command == "M115 S628" && printer.getOperatingMode() == FIRMWARE)) {
			
				// Set thread start callback
				threadStartCallback = [=]() -> void {

					// Disable connection controls
					enableConnectionControls(false);

					// Disable firmware controls
					enableFirmwareControls(false);
					
					// Disable movement controls
					enableMovementControls(false);
					
					// Disable miscellaneous controls
					enableMiscellaneousControls(false);
					
					// Disable calibration controls
					enableCalibrationControls(false);
					
					// Disable settings controls
					enableSettingsControls(false);
				};
			
				// Set thread complete callback
				threadCompleteCallback = [=](ThreadTaskResponse response) -> void {
			
					// Enable connection button
					connectionButton->Enable(true);

					// Check if connected to printer
					if(printer.isConnected())

						// Enable firmware controls
						enableFirmwareControls(true);

					// Otherwise
					else

						// Enable connection controls
						enableConnectionControls(true);
				};
			}
			
			// Send command
			sendCommand(command, threadStartCallback, threadCompleteCallback);
		}
	}
}

void MyFrame::savePrinterSetting(wxCommandEvent& event) {

	// Check if setting can be saved
	if(savePrinterSettingButton->IsEnabled()) {
	
		// Check if value exists
		string value = static_cast<string>(printerSettingInput->GetValue());
		if(!value.empty()) {
		
			// Disable save printer setting button
			savePrinterSettingButton->Enable(false);
			
			// Set offset and length
			uint16_t offset;
			uint8_t length;
			EEPROM_TYPES type;
			printer.getEepromOffsetLengthAndType(static_cast<string>(printerSettingChoice->GetString(printerSettingChoice->GetSelection())), offset, length, type);
			
			// Append thread start callback to queue
			threadStartCallbackQueue.push([=]() -> void {
		
				// Disable settings controls
				enableSettingsControls(false);
			});
		
			// Append thread task to queue
			threadTaskQueue.push([=]() -> ThreadTaskResponse {
			
				// Save value in EEPROM
				bool error = false;
				if(type == EEPROM_INT)
					error = !printer.eepromWriteInt(offset, length, stoi(value));
				else if(type == EEPROM_FLOAT)
					error = !printer.eepromWriteFloat(offset, length, stof(value));
				else if(type == EEPROM_STRING)
					error = !printer.eepromWriteString(offset, length, value);
				else if(type == EEPROM_BOOL)
					error = !printer.eepromWriteInt(offset, length, toLowerCase(value) == "false" ? 0 : 1);
		
				// Check if saving value in EEPROM was successful
				if(!error)
				
					// Return message
					return {"Setting successfully saved", wxOK | wxICON_INFORMATION | wxCENTRE};
				
				// Otherwise
				else
				
					// Return message
					return {"Failed to save setting", wxOK | wxICON_ERROR | wxCENTRE};
			});

			// Append thread complete callback to queue
			threadCompleteCallbackQueue.push([=](ThreadTaskResponse response) -> void {
			
				// Check if connected to printer
				if(printer.isConnected())
	
					// Enable settings controls
					enableSettingsControls(true);
				
				// Display message
				wxMessageBox(response.message, "M33 Manager", response.style);
			});
		}
	}
}

void MyFrame::setPrinterSettingValue() {

	// Set offset and length
	uint16_t offset;
	uint8_t length;
	EEPROM_TYPES type;
	printer.getEepromOffsetLengthAndType(static_cast<string>(printerSettingChoice->GetString(printerSettingChoice->GetSelection())), offset, length, type);
	
	// Get value from EEPROM
	string value;
	if(type == EEPROM_INT)
		value = to_string(printer.eepromGetInt(offset, length));
	else if(type == EEPROM_FLOAT)
		value = to_string(printer.eepromGetFloat(offset, length));
	else if(type == EEPROM_STRING)
		value = printer.eepromGetString(offset, length);
	else if(type == EEPROM_BOOL)
		value = printer.eepromGetInt(offset, length) == 0 ? "False" : "True";
	
	// Clean up floating point values
	if(type == EEPROM_FLOAT) {
		while(value.back() == '0')
			value.pop_back();
		if(value.back() == '.')
			value.pop_back();
	}
	
	// Set printer setting input
	printerSettingInput->SetValue(value);
}

void MyFrame::sendCommand(const string &command, function<void()> threadStartCallback, function<void(ThreadTaskResponse response)> threadCompleteCallback) {

	// Log command
	logToConsole("Send: " + command);

	// Lock
	wxCriticalSectionLocker lock(criticalLock);

	// Append thread start callback to queue
	threadStartCallbackQueue.push(threadStartCallback ? threadStartCallback : [=]() -> void {});
	
	// Append thread task to queue
	threadTaskQueue.push([=]() -> ThreadTaskResponse {
	
		// Set changed mode
		bool changedMode = (command == "Q" && printer.getOperatingMode() == BOOTLOADER) || (command == "M115 S628" && printer.getOperatingMode() == FIRMWARE);
		
		// Check if send command failed
		if(!printer.sendRequest(command))
		
			// Log error
			logToConsole("Sending command failed");
		
		// Otherwise check if not changing modes
		else if(!changedMode) {
		
			// Initialize response and waiting
			string response;
		
			// Wait until command receives a response
			do {
				
				// Get response
				do {
					response = printer.receiveResponse();
				} while(response.empty() && printer.isConnected());
		
				// Check if printer isn't connected
				if(!printer.isConnected())
		
					// Break
					break;
				
				// Check if printer is in bootloader mode
				if(printer.getOperatingMode() == BOOTLOADER) {
				
					// Convert response to hexadecimal
					stringstream hexResponse;
					for(size_t i = 0; i < response.length(); i++)
						hexResponse << "0x" << hex << setfill('0') << setw(2) << uppercase << (static_cast<uint8_t>(response[i]) & 0xFF) << ' ';

					response = hexResponse.str();
					if(!response.empty())
						response.pop_back();
				}
				
				// Log response
				if(response != "wait" && response != "ait")
					logToConsole("Receive: " + response);
				
				// Check if printer is in bootloader mode
				if(printer.getOperatingMode() == BOOTLOADER)
				
					// Break
					break;
			} while(response.substr(0, 2) != "ok" && response[0] != 'k' && response.substr(0, 2) != "rs" && response.substr(0, 4) != "skip" && response.substr(0, 5) != "Error");
		}

		// Return empty response
		return {"", 0};
	});
	
	// Append thread complete callback to queue
	threadCompleteCallbackQueue.push(threadCompleteCallback ? threadCompleteCallback : [=](ThreadTaskResponse response) -> void {});
}

void MyFrame::checkInvalidValues() {

	// Check if printer is connected
	if(printer.isConnected()) {

		// Lock
		wxCriticalSectionLocker lock(criticalLock);

		// Append thread start callback to queue
		threadStartCallbackQueue.push([=]() -> void {});

		// Append thread task to queue
		threadTaskQueue.push([=]() -> ThreadTaskResponse {
	
			// Check if printer is in firmware mode
			if(printer.getOperatingMode() == FIRMWARE)
	
				// Put printer into bootloader mode
				printer.switchToBootloaderMode();

			// Return empty response
			return {"", 0};
		});

		// Append thread complete callback to queue
		threadCompleteCallbackQueue.push([=](ThreadTaskResponse response) -> void {

			// Check if printer is still connected
			if(printer.isConnected()) {

				// Refresh EEPROM
				printer.collectPrinterInformation(false);
		
				// Set calibrate bed orientation dialog
				function<void()> calibrateBedOrientationDialog = [=]() -> void {

					// Check if printer's bed orientation is invalid
					if(!printer.hasValidBedOrientation()) {
	
						// Display bed orientation calibration dialog
						wxMessageDialog *dial = new wxMessageDialog(nullptr, "Bed orientation is invalid. Calibrate?", "M33 Manager", wxYES_NO | wxYES_DEFAULT | wxICON_QUESTION);
		
						// Check if calibrating bed orientation
						if(dial->ShowModal() == wxID_YES) {
		
							// Lock
							wxCriticalSectionLocker lock(criticalLock);

							// Append thread start callback to queue
							threadStartCallbackQueue.push([=]() -> void {
					
								// Stop status timer
								statusTimer->Stop();
					
								// Set fixing invalid values
								fixingInvalidValues = true;
				
								// Disable connection controls
								enableConnectionControls(false);

								// Disable firmware controls
								enableFirmwareControls(false);
	
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
							});

							// Append thread task to queue
							threadTaskQueue.push([=]() -> ThreadTaskResponse {
					
								// Check if printer is in bootloader mode
								if(printer.getOperatingMode() == BOOTLOADER)
	
									// Put printer into firmware mode
									printer.switchToFirmwareMode();

								// Return empty response
								return {"", 0};
							});

							// Append thread complete callback to queue
							threadCompleteCallbackQueue.push([=](ThreadTaskResponse response) -> void {
					
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
		
									// Disable movement controls
									enableMovementControls(false);
		
									// Disable settings controls
									enableSettingsControls(false);
		
									// Disable miscellaneous controls
									enableMiscellaneousControls(false);
									
									// Disable calibration controls
									enableCalibrationControls(false);
					
									// Calibrate bed orientation
									wxCommandEvent event(wxEVT_BUTTON, calibrateBedOrientationButton->GetId());
									calibrateBedOrientationButton->GetEventHandler()->ProcessEvent(event);
						
									// Append thread start callback to queue
									threadStartCallbackQueue.push([=]() -> void {});

									// Append thread task to queue
									threadTaskQueue.push([=]() -> ThreadTaskResponse {

										// Return empty response
										return {"", 0};
									});

									// Append thread complete callback to queue
									threadCompleteCallbackQueue.push([=](ThreadTaskResponse response) -> void {
						
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
					if(!printer.hasValidBedPosition()) {
	
						// Display bed position calibration dialog
						wxMessageDialog *dial = new wxMessageDialog(nullptr, "Bed position is invalid. Calibrate?", "M33 Manager", wxYES_NO | wxYES_DEFAULT | wxICON_QUESTION);
		
						// Check if calibrating bed position
						if(dial->ShowModal() == wxID_YES) {
				
							// Lock
							wxCriticalSectionLocker lock(criticalLock);

							// Append thread start callback to queue
							threadStartCallbackQueue.push([=]() -> void {
					
								// Stop status timer
								statusTimer->Stop();
					
								// Set fixing invalid values
								fixingInvalidValues = true;
				
								// Disable connection controls
								enableConnectionControls(false);

								// Disable firmware controls
								enableFirmwareControls(false);
	
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
							});

							// Append thread task to queue
							threadTaskQueue.push([=]() -> ThreadTaskResponse {
					
								// Check if printer is in bootloader mode
								if(printer.getOperatingMode() == BOOTLOADER)
	
									// Put printer into firmware mode
									printer.switchToFirmwareMode();

								// Return empty response
								return {"", 0};
							});

							// Append thread complete callback to queue
							threadCompleteCallbackQueue.push([=](ThreadTaskResponse response) -> void {
					
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
		
									// Disable movement controls
									enableMovementControls(false);
		
									// Disable settings controls
									enableSettingsControls(false);
		
									// Disable miscellaneous controls
									enableMiscellaneousControls(false);
									
									// Disable calibration controls
									enableCalibrationControls(false);
					
									// Calibrate bed position
									wxCommandEvent event(wxEVT_BUTTON, calibrateBedPositionButton->GetId());
									calibrateBedPositionButton->GetEventHandler()->ProcessEvent(event);
						
									// Lock
									wxCriticalSectionLocker lock(criticalLock);

									// Append thread start callback to queue
									threadStartCallbackQueue.push([=]() -> void {});

									// Append thread task to queue
									threadTaskQueue.push([=]() -> ThreadTaskResponse {

										// Return empty response
										return {"", 0};
									});

									// Append thread complete callback to queue
									threadCompleteCallbackQueue.push([=](ThreadTaskResponse response) -> void {
						
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
					if(!printer.hasValidFirmware()) {
	
						// Get iMe version
						string iMeVersion = static_cast<string>(TOSTRING(IME_ROM_VERSION_STRING)).substr(2);
						for(uint8_t i = 0; i < 3; i++)
							iMeVersion.insert(i * 2 + 2 + i, ".");
		
						// Display firmware installation dialog
						wxMessageDialog *dial = new wxMessageDialog(nullptr, "Firmware is corrupt. Install " + (printer.getFirmwareType() == M3D || printer.getFirmwareType() == M3D_MOD ? "M3D V" TOSTRING(M3D_ROM_VERSION_STRING) : "iMe V" + iMeVersion) + "?", "M33 Manager", wxYES_NO | wxYES_DEFAULT | wxICON_QUESTION);
		
						// Check if installing firmware
						if(dial->ShowModal() == wxID_YES) {
				
							// Set fixing invalid values
							fixingInvalidValues = true;
		
							// Check if installing M3D firmware
							if(printer.getFirmwareType() == M3D || printer.getFirmwareType() == M3D_MOD) {
			
								// Install M3D firmware
								wxCommandEvent event(wxEVT_BUTTON, installM3dFirmwareButton->GetId());
								installM3dFirmwareButton->GetEventHandler()->ProcessEvent(event);
							}
			
							// Otherwise
							else {
			
								// Install iMe firmware
								wxCommandEvent event(wxEVT_BUTTON, installImeFirmwareButton->GetId());
								installImeFirmwareButton->GetEventHandler()->ProcessEvent(event);
							}
					
							// Lock
							wxCriticalSectionLocker lock(criticalLock);

							// Append thread start callback to queue
							threadStartCallbackQueue.push([=]() -> void {});

							// Append thread task to queue
							threadTaskQueue.push([=]() -> ThreadTaskResponse {

								// Return empty response
								return {"", 0};
							});

							// Append thread complete callback to queue
							threadCompleteCallbackQueue.push([=](ThreadTaskResponse response) -> void {
					
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
					else if(printer.getFirmwareType() == UNKNOWN_FIRMWARE || ((printer.getFirmwareType() == M3D || printer.getFirmwareType() == M3D_MOD) && stoi(printer.getFirmwareRelease()) < M3D_ROM_VERSION_STRING) || (printer.getFirmwareType() == IME && printer.getFirmwareVersion() < IME_ROM_VERSION_STRING)) {
					
						// Set if firmware is incompatible
						bool incompatible = true;
						if((printer.getFirmwareType() == M3D || printer.getFirmwareType() == M3D_MOD) && stoi(printer.getFirmwareRelease()) >= 2015122112)
							incompatible = false;
						else if(printer.getFirmwareType() == IME && printer.getFirmwareVersion() >= 1900000118)
							incompatible = false;
						
						// Get iMe version
						string iMeVersion = static_cast<string>(TOSTRING(IME_ROM_VERSION_STRING)).substr(2);
						for(uint8_t i = 0; i < 3; i++)
							iMeVersion.insert(i * 2 + 2 + i, ".");
	
						// Display firmware installation dialog
						wxMessageDialog *dial = new wxMessageDialog(nullptr, static_cast<string>(incompatible ? "Firmware is incompatible" : "Newer firmware available") + ". Update to " + (printer.getFirmwareType() == M3D || printer.getFirmwareType() == M3D_MOD ? "M3D V" TOSTRING(M3D_ROM_VERSION_STRING) : "iMe V" + iMeVersion) + "?", "M33 Manager", wxYES_NO | wxYES_DEFAULT | wxICON_QUESTION);
	
						// Check if installing firmware
						if(dial->ShowModal() == wxID_YES) {
			
							// Set fixing invalid values
							fixingInvalidValues = true;
	
							// Check if installing M3D firmware
							if(printer.getFirmwareType() == M3D || printer.getFirmwareType() == M3D_MOD) {
		
								// Install M3D firmware
								wxCommandEvent event(wxEVT_BUTTON, installM3dFirmwareButton->GetId());
								installM3dFirmwareButton->GetEventHandler()->ProcessEvent(event);
							}
		
							// Otherwise
							else {
		
								// Install iMe firmware
								wxCommandEvent event(wxEVT_BUTTON, installImeFirmwareButton->GetId());
								installImeFirmwareButton->GetEventHandler()->ProcessEvent(event);
							}
				
							// Lock
							wxCriticalSectionLocker lock(criticalLock);

							// Append thread start callback to queue
							threadStartCallbackQueue.push([=]() -> void {});

							// Append thread task to queue
							threadTaskQueue.push([=]() -> ThreadTaskResponse {

								// Return empty response
								return {"", 0};
							});

							// Append thread complete callback to queue
							threadCompleteCallbackQueue.push([=](ThreadTaskResponse response) -> void {
				
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
						else if(incompatible)
						
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
				if(!printer.hasValidBedOrientation() || !printer.hasValidBedPosition() || !printer.hasValidFirmware())
		
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

// Check if using Windows
#ifdef WINDOWS

	WXLRESULT MyFrame::MSWWindowProc(WXUINT message, WXWPARAM wParam, WXLPARAM lParam) {
	
		// Check if device change
		if(message == WM_DEVICECHANGE)
		
			// Check if an interface device was removed
			if(wParam == DBT_DEVICEREMOVECOMPLETE && reinterpret_cast<PDEV_BROADCAST_HDR>(lParam)->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE) {
			
				// Create device info string
				stringstream deviceInfoStringStream;
				deviceInfoStringStream << "VID_" << setfill('0') << setw(4) << hex << uppercase << PRINTER_VENDOR_ID << "&PID_" << setfill('0') << setw(4) << hex << uppercase << PRINTER_PRODUCT_ID;
				string deviceInfoString = deviceInfoStringStream.str();
				
				// Check if device has the printer's PID and VID
				PDEV_BROADCAST_DEVICEINTERFACE deviceInterface = reinterpret_cast<PDEV_BROADCAST_DEVICEINTERFACE>(lParam);
				
				if(!_tcsnicmp(deviceInterface->dbcc_name, _T("\\\\?\\USB#" + deviceInfoString), ("\\\\?\\USB#" + deviceInfoString).length())) {
					
					// Get device ID
					wstring deviceId = &deviceInterface->dbcc_name[("\\\\?\\USB#" + deviceInfoString).length() + 1];
					deviceId = _T("USB\\" + deviceInfoString + "\\") + deviceId.substr(0, deviceId.find(_T("#")));
					
					// Check if getting all connected devices was successful
					HDEVINFO deviceInfo = SetupDiGetClassDevs(nullptr, nullptr, nullptr, DIGCF_ALLCLASSES);
					if(deviceInfo != INVALID_HANDLE_VALUE) {

						// Check if allocating memory was successful
						PSP_DEVINFO_DATA deviceInfoData = reinterpret_cast<PSP_DEVINFO_DATA>(HeapAlloc(GetProcessHeap(), 0, sizeof(SP_DEVINFO_DATA)));
						if(deviceInfoData) {

							// Set device info size
							deviceInfoData->cbSize = sizeof(SP_DEVINFO_DATA);

							// Go through all devices
							for(DWORD i = 0; SetupDiEnumDeviceInfo(deviceInfo, i, deviceInfoData); i++) {

								// Check if device is has the printer's PID and VID
								TCHAR buffer[MAX_PATH];
								if(SetupDiGetDeviceInstanceId(deviceInfo, deviceInfoData, buffer, sizeof(buffer), nullptr) && !_tcsnicmp(buffer, deviceId.c_str(), deviceId.length())) {
		
									// Check if getting device registry key was successful
									HKEY deviceRegistryKey = SetupDiOpenDevRegKey(deviceInfo, deviceInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_QUERY_VALUE);
									if(deviceRegistryKey) {

										// Check if getting port name was successful
										DWORD type = 0;
										DWORD size = sizeof(buffer);
										if(RegQueryValueEx(deviceRegistryKey, _T("PortName"), nullptr, &type, reinterpret_cast<LPBYTE>(buffer), &size) == ERROR_SUCCESS && type == REG_SZ) {
	
											// Check if port is the printer's current port
											string currentPort = printer.getCurrentSerialPort();
											if(!_tcsicmp(buffer, wstring(currentPort.begin(), currentPort.end()).c_str()))
											
												// Disconnect printer
												printer.disconnect();
										}
										
										// Close registry key
										RegCloseKey(deviceRegistryKey);
									}
								}
							}

							// Clear device info data
							HeapFree(GetProcessHeap(), 0, deviceInfoData);
						}
						
						// Clear device info
						SetupDiDestroyDeviceInfoList(deviceInfo);
					}
				}
			}
		
		// Return event	
		return wxFrame::MSWWindowProc(message, wParam, lParam);
	}
#endif
