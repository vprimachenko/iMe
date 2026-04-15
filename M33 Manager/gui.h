// Header guard
#ifndef GUI_H
#define GUI_H


// Header files
#include <string>
#include <functional>
#include <queue>
#include <thread>
#include <atomic>
#include <wx/sysopt.h>
#include <wx/glcanvas.h>
#include <wx/gbsizer.h>
#include "common.h"
#include "gui_host.h"
#include "ui_layout.h"
#include "printer_workflows.h"
#include "main_tab_controller.h"
#include "control_tab_controller.h"
#include "firmware_tab_controller.h"
#include "print_tab_controller.h"

// My app class
class MyApp: public wxApp {

	// Public
	public:

		/*
		Name: On init
		Purpose: Run when program starts
		*/
		virtual bool OnInit();
};

// My frame class
class MyFrame: public wxFrame, public GuiHost {

	// Public
	public:
	
		/*
		Name: Constructor
		Purpose: Initializes frame
		*/
		MyFrame(const wxString& title, const wxPoint& pos, const wxSize& size, long style);
		~MyFrame() override;
		
		/*
		Name: Thread task start
		Purpose: Event that's called when background thread starts a task
		*/
		void threadTaskStart(wxThreadEvent& event);
		
		/*
		Name: Thread task complete
		Purpose: Event that's called when background thread completes a task
		*/
		void threadTaskComplete(wxThreadEvent& event);
		
		/*
		Name: Close
		Purpose: Event that's called when frame closes
		*/
		void close(wxCloseEvent& event);
		
		/*
		Name: Change printer connection
		Purpose: Connects to or disconnects from the printer
		*/
		void changePrinterConnection(wxCommandEvent& event);
		
		/*
		Name: Switch to mode
		Purpose: Switches printer into firmware or bootloader mode
		*/
		void onSwitchToModeButton(wxCommandEvent& event);
		void switchToMode();
		
		/*
		Name: Install iMe firmware
		Purpose: Installs iMe firmware as the printer's firmware
		*/
		void installImeFirmware() override;
		
		/*
		Name: Install M3D firmware
		Purpose: Installs M3D firmware as the printer's firmware
		*/
		void installM3dFirmware() override;
		
		/*
		Name: Install drivers
		Purpose: Installs device drivers for the printer
		*/
		void installDrivers() override;
		
		/*
		Name: Log to console
		Purpose: Queues message to be displayed in the console
		*/
		void logToConsole(const string &message);
		
		/*
		Name: Update log
		Purpose: Appends queued messages to console
		*/
		void updateLog(wxTimerEvent& event);
		
		/*
		Name: Update status
		Purpose: Updates status text
		*/
		void updateStatus(wxTimerEvent& event);
		
		/*
		Name: Install firmware from file
		Purpose: Installs a file as the printer's firmware
		*/
		void installFirmwareFromFile() override;
		
		/*
		Name: Get available serial ports
		Purpose: Returns all available serial ports
		*/
		wxArrayString getAvailableSerialPorts();
		
		/*
		Name: Refresh serial ports
		Purpose: Refreshes list of serial ports
		*/
		void refreshSerialPorts(wxCommandEvent& event);
		
		/*
		Name: Send command manually
		Purpose: Sends a command manually to the printer
		*/
		vector<string> getPrinterSettingNames() override;
		string loadPrinterSettingValue(const string &settingName) override;
		void runManualCommand(const string &command) override;
		void runMoveX(bool positive, double distanceMm, int feedRate) override;
		void runMoveY(bool positive, double distanceMm, int feedRate) override;
		void runMoveZ(bool positive, double distanceMm, int feedRate) override;
		void runHome() override;
		void runMotorsOn() override;
		void runMotorsOff() override;
		void runFanOn() override;
		void runFanOff() override;
			void runCalibrateBedPosition() override;
			void runCalibrateBedOrientation() override;
			void runSaveCurrentPositionAsHome() override;
			void runSaveZAsZero() override;
		void loadPrintFile() override;
		void startPrintJob() override;
		void pausePrintJob() override;
		void resumePrintJob() override;
		void stopPrintJob() override;
		PrintJobStatus getPrintJobStatus() const override;
		const PreparedPrintJob &getPreparedPrintJob() const override;
		
		/*
		Name: Update distance movement text
		Purpose: Changes distance movement's text to the current distance set by the slider
		*/
		void enableConnectionControls(bool enable);
		void enableCommandControls(bool enable);
		void enableFirmwareControls(bool enable);
		void enableMovementControls(bool enable);
		void enableSettingsControls(bool enable);
		void enableMiscellaneousControls(bool enable);
		void enableCalibrationControls(bool enable);
		void setConnectedUiVisible(bool visible);
		void setStatusRowVisible(bool visible);
		void refreshWindowLayout();
		void applyPrintJobStatus(const PrintJobStatus &status);
		void restoreControlsForCurrentState();
		bool isPrintJobBlockingUi() const;
		void updatePrintUiAvailability();
		
		void savePrinterSetting(const string &settingName, const string &value) override;
		
		/*
		Name: Check invalid values
		Purpose: Checks in the printer has invalid values and displays dialogs to fix them
		*/
		void checkInvalidValues();
		
		// Check if using Windows
		#ifdef WINDOWS
		
			/*
			Name: MSWWindowProc
			Purpose: Windows event manager
			*/
			WXLRESULT MSWWindowProc(WXUINT message, WXWPARAM wParam, WXLPARAM lParam);
		#endif
	
	// Private
	private:
		void shutdownFrameResources();
		void workerMain();
		bool startWorkerThread();
		void stopWorkerThread();

		UiLayout ui;
		PrinterWorkflows workflows;
		MainTabController mainTabController;
		ControlTabController controlTabController;
		FirmwareTabController firmwareTabController;
		PrintTabController printTabController;
		
		// Controls
		wxChoice *serialPortChoice;
		wxButton *refreshSerialPortsButton;
		wxButton *connectionButton;
		wxStaticText *versionText;
		wxStaticText *statusText;
		wxButton *switchToModeButton;
		wxTimer *logTimer;
		wxTimer *statusTimer;
		
		// Critical lock
		wxCriticalSection criticalLock;
		
		// Thread start, task, and complete queues
		queue<function<void()>> threadStartCallbackQueue;
		queue<function<ThreadTaskResponse()>> threadTaskQueue;
		queue<function<void(ThreadTaskResponse response)>> threadCompleteCallbackQueue;
		
		// Log queue
		queue<string> logQueue;
		queue<PrintJobStatus> printJobUpdateQueue;
		std::thread workerThread;
		std::atomic<bool> workerStopRequested;
		
		// Printer
		Printer printer;
		PreparedPrintJob preparedPrintJob;
		PrintJobStatus printJobStatus;
		
		// Allow enabling controls
		bool allowEnablingControls;
		
		// Fixing invalid values
		bool fixingInvalidValues;

		// Print job controls
		bool printPauseRequested;
		bool printStopRequested;
		bool closing;
};


#endif
