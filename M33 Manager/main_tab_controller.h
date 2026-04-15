#ifndef MAIN_TAB_CONTROLLER_H
#define MAIN_TAB_CONTROLLER_H

#include <string>
#include <wx/button.h>
#include <wx/textctrl.h>
#include "gui_host.h"
#include "ui_layout.h"

using namespace std;

class MainTabController {
	public:
		void build(const UiLayout &ui, GuiHost &host);
		void enableCommandControls(bool enable);
		void appendConsoleMessage(const string &message);
		void removeLastConsoleLine();
		void clearCommandInput();
		bool isCommandSubmissionEnabled() const;

	private:
		void sendCommandManually(wxCommandEvent &event);

		GuiHost *host = nullptr;
		wxTextCtrl *commandInput = nullptr;
		wxTextCtrl *consoleOutput = nullptr;
		wxButton *sendCommandButton = nullptr;
};

#endif
