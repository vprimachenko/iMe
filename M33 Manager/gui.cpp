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
	wxFrame(nullptr, wxID_ANY, title, pos, size, style),
	workflows(printer)
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
	closing = false;
	workerStopRequested = false;

	// Initialize print job state
	printPauseRequested = false;
	printStopRequested = false;
	printJobStatus.state = PRINT_JOB_IDLE;
	printJobStatus.statusText = "No print job loaded";

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
	ui.statusRowSizer->Add(statusText, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
	ui.connectionContentSizer->Add(ui.statusRow, 0, wxEXPAND | wxTOP, 8);
	
	// Create log timer
	logTimer = new wxTimer(this, wxID_ANY);
	Bind(wxEVT_TIMER, &MyFrame::updateLog, this, logTimer->GetId());
	logTimer->Start(100);
	
	// Create status timer
	statusTimer = new wxTimer(this, wxID_ANY);
	Bind(wxEVT_TIMER, &MyFrame::updateStatus, this, statusTimer->GetId());
	statusTimer->Start(100);

	switchToModeButton = new wxButton(ui.statusRow, wxID_ANY, "Switch to bootloader mode");
	switchToModeButton->Enable(false);
	switchToModeButton->Bind(wxEVT_BUTTON, [=](wxCommandEvent &event) {
		switchToMode();
	});
	ui.statusRowSizer->Add(switchToModeButton, 0, wxALIGN_CENTER_VERTICAL);

	mainTabController.build(ui, *this);
	controlTabController.build(ui, *this);
	firmwareTabController.build(ui, *this);
	printTabController.build(ui, *this);

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
	
	// Check if creating worker thread failed
	if(!startWorkerThread())
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

MyFrame::~MyFrame() = default;

void MyFrame::workerMain() {

	// Loop until stopped
	while(!workerStopRequested.load()) {
		
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
			if(!workerStopRequested.load() && !closing)
				wxQueueEvent(GetEventHandler(), new wxThreadEvent(wxEVT_THREAD_TASK_START));
			
			// Perform thread task
			ThreadTaskResponse response = threadTask();
		
			// Trigger thread complete event
			if(!workerStopRequested.load() && !closing) {
				wxThreadEvent *event = new wxThreadEvent(wxEVT_THREAD_TASK_COMPLETE);
				event->SetPayload(response);
				wxQueueEvent(GetEventHandler(), event);
			}
		}
		
		// Delay
		sleepUs(10000);
	}
}

bool MyFrame::startWorkerThread() {
	try {
		workerStopRequested = false;
		workerThread = std::thread(&MyFrame::workerMain, this);
		return true;
	}
	catch(...) {
		return false;
	}
}

void MyFrame::stopWorkerThread() {
	workerStopRequested = true;
	if(workerThread.joinable())
		workerThread.join();
}

void MyFrame::threadTaskStart(wxThreadEvent& event) {
	if(closing)
		return;

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
	if(closing)
		return;

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
	if(closing) {
		event.Veto(false);
		return;
	}

	closing = true;

	// Stop timers and unbind timer handlers before destruction.
	if(logTimer) {
		logTimer->Stop();
		Unbind(wxEVT_TIMER, &MyFrame::updateLog, this, logTimer->GetId());
		delete logTimer;
		logTimer = nullptr;
	}

	if(statusTimer) {
		statusTimer->Stop();
		Unbind(wxEVT_TIMER, &MyFrame::updateStatus, this, statusTimer->GetId());
		delete statusTimer;
		statusTimer = nullptr;
	}

	Unbind(wxEVT_THREAD_TASK_START, &MyFrame::threadTaskStart, this);
	Unbind(wxEVT_THREAD_TASK_COMPLETE, &MyFrame::threadTaskComplete, this);

	// Disconnect printer
	printer.disconnect();
	
	stopWorkerThread();

	DeletePendingEvents();
	
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
				enableCommandControls(true);
				
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
			enableCommandControls(false);
			
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
				mainTabController.removeLastConsoleLine();
			}
		
			// Otherwise
			else {
			
				// Check if not fixing invalid values
				if(!fixingInvalidValues) {
			
					// Check if printer is switching modes
					if(message == "Switching printer into bootloader mode" || message == "Switching printer into firmware mode") {
				
						// Disable movement controls
						enableMovementControls(false);
						enableCommandControls(false);
					
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
							enableCommandControls(true);
						
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
							enableCommandControls(true);
						
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
							enableCommandControls(false);
						
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
				mainTabController.appendConsoleMessage(message);
			}
		}
	}

	// Process print job updates
	while(true) {
		PrintJobStatus status;
		bool hasStatus = false;

		{
			wxCriticalSectionLocker lock(criticalLock);
			if(!printJobUpdateQueue.empty()) {
				status = printJobUpdateQueue.front();
				printJobUpdateQueue.pop();
				hasStatus = true;
			}
		}

		if(!hasStatus)
			break;

		applyPrintJobStatus(status);
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
			enableCommandControls(true);
		
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
				enableCommandControls(false);
				
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

	if(enable && isPrintJobBlockingUi())
		return;

	// Enable or disable connection controls
	serialPortChoice->Enable(enable);
	refreshSerialPortsButton->Enable(enable);
	connectionButton->Enable(enable);
}

void MyFrame::enableCommandControls(bool enable) {

	if(enable && isPrintJobBlockingUi())
		return;

	// Enable or disable manual command controls
	mainTabController.enableCommandControls(enable);
}

void MyFrame::enableFirmwareControls(bool enable) {

	if(enable && isPrintJobBlockingUi())
		return;

	// Enable or disable firmware controls
	firmwareTabController.enableFirmwareControls(enable);
	switchToModeButton->Enable(enable);
}

void MyFrame::enableMovementControls(bool enable) {

	if(enable && isPrintJobBlockingUi())
		return;

	// Enable or disable movement controls
	controlTabController.enableMovementControls(enable);
}

void MyFrame::enableSettingsControls(bool enable) {

	if(enable && isPrintJobBlockingUi())
		return;

	// Enable or disable settings controls
	controlTabController.enableSettingsControls(enable);
}

void MyFrame::enableMiscellaneousControls(bool enable) {

	if(enable && isPrintJobBlockingUi())
		return;

	// Enable or disable miscellaneous controls
	controlTabController.enableMiscellaneousControls(enable);
}

void MyFrame::enableCalibrationControls(bool enable) {

	if(enable && isPrintJobBlockingUi())
		return;

	// Enable or disable calibration controls
	controlTabController.enableCalibrationControls(enable);
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
