// Header files
#include <fstream>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <wx/mstream.h>
#include "gui.h"
#ifdef WINDOWS
	#include <setupapi.h>
	#include <dbt.h>
#endif


// Custom events
wxDEFINE_EVENT(wxEVT_THREAD_TASK_START, wxThreadEvent);
wxDEFINE_EVENT(wxEVT_THREAD_TASK_COMPLETE, wxThreadEvent);

namespace {
constexpr int DEFAULT_PAUSE_STANDBY_TEMPERATURE_C = 120;
constexpr int MIN_PAUSE_STANDBY_TEMPERATURE_C = 80;
constexpr int MAX_PAUSE_STANDBY_TEMPERATURE_C = 160;
constexpr int DEFAULT_MANUAL_NOZZLE_TEMPERATURE_C = 200;
constexpr int MIN_MANUAL_NOZZLE_TEMPERATURE_C = 150;
constexpr int MAX_MANUAL_NOZZLE_TEMPERATURE_C = 260;
}

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
	printPauseStandbyApplied = false;
	hasTrackedPrintNozzleTarget = false;
	lastPrintNozzleTargetC = 0;
	printJobStatus.state = PRINT_JOB_IDLE;
	printJobStatus.statusText = "No print job loaded";
	connectionStatusText = "Not connected";
	connectionStatusColour = wxColour(255, 0, 0);
	wxConfig config("M33 Manager");
	pauseStandbyTemperatureC = static_cast<int>(config.ReadLong("PauseStandbyTemperatureC", DEFAULT_PAUSE_STANDBY_TEMPERATURE_C));
	pauseStandbyTemperatureC = max(MIN_PAUSE_STANDBY_TEMPERATURE_C, min(MAX_PAUSE_STANDBY_TEMPERATURE_C, pauseStandbyTemperatureC));
	manualNozzleTemperatureC = static_cast<int>(config.ReadLong("ManualNozzleTemperatureC", DEFAULT_MANUAL_NOZZLE_TEMPERATURE_C));
	manualNozzleTemperatureC = max(MIN_MANUAL_NOZZLE_TEMPERATURE_C, min(MAX_MANUAL_NOZZLE_TEMPERATURE_C, manualNozzleTemperatureC));

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
	
	mainTabController.build(ui, *this);
	controlTabController.build(ui, *this);
	firmwareTabController.build(ui, *this);
	printTabController.build(ui, *this);

	wxStaticText *firmwareModeHint = new wxStaticText(ui.firmwareSection, wxID_ANY,
		"Firmware management actions require bootloader mode.\n"
		"Connect uses firmware mode automatically for normal printer control.");
	switchToModeButton = new wxButton(ui.firmwareSection, wxID_ANY, "Switch to bootloader mode");
	switchToModeButton->Enable(false);
	switchToModeButton->Bind(wxEVT_BUTTON, &MyFrame::onSwitchToModeButton, this);
	ui.firmwareSizer->Insert(0, firmwareModeHint, 0, wxEXPAND | wxBOTTOM, 8);
	ui.firmwareSizer->Insert(1, switchToModeButton, 0, wxALIGN_LEFT | wxBOTTOM, 12);

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
	statusText = new wxStaticText(ui.footerSection, wxID_ANY, connectionStatusText);
	statusText->SetForegroundColour(connectionStatusColour);
	ui.footerSizer->Add(statusText, 0, wxALIGN_CENTER_VERTICAL | wxBOTTOM, 2);
	ui.footerSizer->AddStretchSpacer(1);
	ui.footerSizer->Add(versionText, 0, wxALIGN_RIGHT);

	setConnectedUiVisible(false);
	refreshFooterStatus();
	Layout();
	
	// Thread task events
	Bind(wxEVT_THREAD_TASK_START, &MyFrame::threadTaskStart, this);
	Bind(wxEVT_THREAD_TASK_COMPLETE, &MyFrame::threadTaskComplete, this);
	
	// Check if creating worker thread failed
	if(!startWorkerThread())
		Close();

	// Create log timer after the full UI is built so timer callbacks
	// never run against partially constructed controls.
	logTimer = new wxTimer(this, wxID_ANY);
	Bind(wxEVT_TIMER, &MyFrame::updateLog, this, logTimer->GetId());
	logTimer->Start(100);
	
	// Create status timer after the rest of the UI is ready.
	statusTimer = new wxTimer(this, wxID_ANY);
	Bind(wxEVT_TIMER, &MyFrame::updateStatus, this, statusTimer->GetId());
	statusTimer->Start(100);
	
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

MyFrame::~MyFrame() {
	shutdownFrameResources();
}

void MyFrame::enqueueBackgroundTask(
	function<void()> startCallback,
	function<ThreadTaskResponse()> task,
	function<void(ThreadTaskResponse)> completeCallback
) {
	wxCriticalSectionLocker lock(criticalLock);
	pendingTaskQueue.push({
		startCallback ? startCallback : []() -> void {},
		task,
		completeCallback ? completeCallback : [](ThreadTaskResponse response) -> void {}
	});
}

void MyFrame::shutdownFrameResources() {
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

	stopWorkerThread();
	printer.disconnect();

	wxCriticalSectionLocker lock(criticalLock);
	while(!pendingTaskQueue.empty())
		pendingTaskQueue.pop();
	while(!logQueue.empty())
		logQueue.pop();
	while(!printJobUpdateQueue.empty())
		printJobUpdateQueue.pop();
}

void MyFrame::workerMain() {
	// Loop until stopped
	while(!workerStopRequested.load()) {
		
		// Initialize task
		PendingGuiTask pendingTask;
		bool hasPendingTask = false;
		
		{
			// Lock
			wxCriticalSectionLocker lock(criticalLock);
			
			if(!pendingTaskQueue.empty()) {
				pendingTask = pendingTaskQueue.front();
				pendingTaskQueue.pop();
				hasPendingTask = true;
			}
		}
		
		if(hasPendingTask && pendingTask.task) {
		
			if(!workerStopRequested.load() && !closing) {
				wxThreadEvent *startEvent = new wxThreadEvent(wxEVT_THREAD_TASK_START);
				startEvent->SetPayload(ThreadTaskStartPayload{pendingTask.startCallback});
				wxQueueEvent(GetEventHandler(), startEvent);
			}
			
			ThreadTaskResponse response = pendingTask.task();
		
			if(!workerStopRequested.load() && !closing) {
				wxThreadEvent *completeEvent = new wxThreadEvent(wxEVT_THREAD_TASK_COMPLETE);
				completeEvent->SetPayload(ThreadTaskCompletePayload{response, pendingTask.completeCallback});
				wxQueueEvent(GetEventHandler(), completeEvent);
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
	ThreadTaskStartPayload payload = event.GetPayload<ThreadTaskStartPayload>();
	if(payload.callback)
		payload.callback();
}

void MyFrame::threadTaskComplete(wxThreadEvent& event) {
	if(closing)
		return;
	ThreadTaskCompletePayload payload = event.GetPayload<ThreadTaskCompletePayload>();
	if(payload.callback)
		payload.callback(payload.response);
}

void MyFrame::close(wxCloseEvent& event) {
	if(closing) {
		event.Veto(false);
		return;
	}

	closing = true;
	shutdownFrameResources();

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

		enqueueBackgroundTask([=]() -> void {
			// Stop status timer
			statusTimer->Stop();
		
			// Disable connection controls
			enableConnectionControls(false);

			connectionStatusText = "Connecting";
			connectionStatusColour = wxColour(255, 180, 0);
			refreshFooterStatus();
			refreshWindowLayout();
			
			// Clear allow enabling controls
			allowEnablingControls = false;
		},
		[=]() -> ThreadTaskResponse {
			// Connect to printer
			printer.connect(currentChoice != "Auto" ? currentChoice : "");

			// Normal interactive use expects firmware mode after connecting.
			if(printer.isConnected())
				workflows.ensureMode(FIRMWARE);
			
			// Return empty response
			return {"", 0};
		},
		[=](ThreadTaskResponse response) -> void {
			// Set allow enabling controls
			allowEnablingControls = true;
	
			// Enable connection button
			connectionButton->Enable(true);

			// Check if connected to printer
			if(printer.isConnected()) {

				// Show connected-only content
				setConnectedUiVisible(true);

				// Change connection button to disconnect	
				connectionButton->SetLabel("Disconnect");
				restoreControlsForCurrentState();
				refreshWindowLayout();
				
				// Log printer mode
				logToConsole(static_cast<string>("Printer is in ") + (printer.getOperatingMode() == BOOTLOADER ? "bootloader" : "firmware") + " mode");
				
				// Start status timer
				statusTimer->Start(100);
			}
			
			// Otherwise
			else {
			
				// Restore compact disconnected layout
				setConnectedUiVisible(false);

				// Enable connection controls
				enableConnectionControls(true);
				updatePrintUiAvailability();
				refreshWindowLayout();
				
				// Start status timer
				statusTimer->Start(100);
			}
		});
	}
	
	// Otherwise
	else {
		
		enqueueBackgroundTask([=]() -> void {
		
			// Stop status timer
			statusTimer->Stop();
		},
		[=]() -> ThreadTaskResponse {
		
			// Disconnect printer
			printer.disconnect();
			
			// Return empty response
			return {"", 0};
		},
		[=](ThreadTaskResponse response) -> void {
	
			// Restore compact disconnected layout
			setConnectedUiVisible(false);

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
			enableStandbyControls(true);
		
			// Change connection button to connect
			connectionButton->SetLabel("Connect");
		
			// Enable connection controls
			enableConnectionControls(true);
			updatePrintUiAvailability();
			refreshWindowLayout();
		
			// Start status timer
			statusTimer->Start(100);
		});
	}
}

void MyFrame::logToConsole(const string &message) {

	// Lock
	wxCriticalSectionLocker lock(criticalLock);

	// Mirror GUI console output to the launching terminal for easier debugging.
	cout << "[" << getTerminalLogLabel(message) << "] " << message << endl;
	cout.flush();

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

			// Show connected layout
			setConnectedUiVisible(true);
			enableCommandControls(true);
		
			// Disable connection controls
			enableConnectionControls(false);
			
			// Enable connection button
			connectionButton->Enable(true);
			updatePrintUiAvailability();
		
			connectionStatusText = status;
			connectionStatusColour = wxColour(0, 255, 0);
		}
	
		// Otherwise
		else {
	
			// Check if printer was just disconnected
			if(status == "Disconnected" && (statusText->GetLabel() != status || connectionButton->GetLabel() == "Disconnect")) {

				// Restore compact disconnected layout
				setConnectedUiVisible(false);
				
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
				enableStandbyControls(true);
		
				// Change connection button to connect
				connectionButton->SetLabel("Connect");
		
				// Enable connection controls
				enableConnectionControls(true);
				updatePrintUiAvailability();
				refreshWindowLayout();
			
				// Log disconnection
				logToConsole("Printer has been disconnected");
			}
		
			connectionStatusText = status;
			connectionStatusColour = wxColour(255, 0, 0);
		}

		refreshFooterStatus();
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
	
	enqueueBackgroundTask([=]() -> void {
		enableConnectionControls(false);
	},
	[=]() -> ThreadTaskResponse {
	
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
	},
	[=](ThreadTaskResponse response) -> void {
	
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
	(void)visible;
}

void MyFrame::refreshWindowLayout() {

	// Recompute layout and resize the frame to the current content
	SetMinSize(wxSize(-1, -1));
	ui.rootPanel->Layout();
	Layout();
	GetSizer()->Fit(this);
	SetMinSize(GetSize());
}

void MyFrame::refreshFooterStatus() {
	wxString footerStatus = connectionStatusText;
	wxColour footerColour = connectionStatusColour;

	if(printJobStatus.state == PRINT_JOB_LOADED) {
		footerStatus = printJobStatus.statusText.empty() ? "Ready to print" : printJobStatus.statusText;
		footerColour = wxColour(120, 120, 120);
	}
	else if(printJobStatus.state == PRINT_JOB_RUNNING || printJobStatus.state == PRINT_JOB_PAUSED || printJobStatus.state == PRINT_JOB_STOPPING) {
		footerStatus = printJobStatus.statusText.empty() ? "Printing..." : printJobStatus.statusText;
		footerColour = printJobStatus.state == PRINT_JOB_PAUSED ? wxColour(255, 180, 0) : wxColour(80, 170, 255);
	}
	else if(printJobStatus.state == PRINT_JOB_COMPLETED) {
		footerStatus = "Print completed";
		footerColour = wxColour(0, 200, 0);
	}
	else if(printJobStatus.state == PRINT_JOB_FAILED) {
		footerStatus = printJobStatus.errorText.empty() ? "Print failed" : printJobStatus.errorText;
		footerColour = wxColour(255, 0, 0);
	}

	statusText->SetLabel(footerStatus);
	statusText->SetForegroundColour(footerColour);
	ui.footerSection->Layout();
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

int MyFrame::getPauseStandbyTemperatureC() const {
	return pauseStandbyTemperatureC;
}

void MyFrame::setPauseStandbyTemperatureC(int temperatureC) {
	pauseStandbyTemperatureC = max(MIN_PAUSE_STANDBY_TEMPERATURE_C, min(MAX_PAUSE_STANDBY_TEMPERATURE_C, temperatureC));
	wxConfig config("M33 Manager");
	config.Write("PauseStandbyTemperatureC", pauseStandbyTemperatureC);
	config.Flush();
}

int MyFrame::getManualNozzleTemperatureC() const {
	return manualNozzleTemperatureC;
}

void MyFrame::setManualNozzleTemperatureC(int temperatureC) {
	manualNozzleTemperatureC = max(MIN_MANUAL_NOZZLE_TEMPERATURE_C, min(MAX_MANUAL_NOZZLE_TEMPERATURE_C, temperatureC));
	wxConfig config("M33 Manager");
	config.Write("ManualNozzleTemperatureC", manualNozzleTemperatureC);
	config.Flush();
}

void MyFrame::enableCalibrationControls(bool enable) {

	if(enable && isPrintJobBlockingUi())
		return;

	// Enable or disable calibration controls
	controlTabController.enableCalibrationControls(enable);
}

void MyFrame::enableStandbyControls(bool enable) {
	if(enable && isPrintJobBlockingUi())
		return;

	controlTabController.enableStandbyControls(enable);
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
