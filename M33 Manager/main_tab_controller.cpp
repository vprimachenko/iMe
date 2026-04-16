#include "main_tab_controller.h"

void MainTabController::build(const UiLayout &ui, GuiHost &newHost) {
	host = &newHost;

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
	commandInput->Enable(false);
	commandInput->Bind(wxEVT_TEXT_ENTER, &MainTabController::sendCommandManually, this);

	sendCommandButton = new wxButton(ui.consoleSection, wxID_ANY, "Send");
	sendCommandButton->Enable(false);
	sendCommandButton->Bind(wxEVT_BUTTON, &MainTabController::sendCommandManually, this);

	wxBoxSizer *consoleSectionSizer = new wxBoxSizer(wxVERTICAL);
	consoleSectionSizer->Add(consoleOutput, 1, wxEXPAND | wxBOTTOM, 8);
	wxBoxSizer *consoleCommandSizer = new wxBoxSizer(wxHORIZONTAL);
	consoleCommandSizer->Add(commandInput, 1, wxEXPAND | wxRIGHT, 8);
	consoleCommandSizer->Add(sendCommandButton, 0, wxEXPAND);
	consoleSectionSizer->Add(consoleCommandSizer, 0, wxEXPAND);
	ui.consoleSizer->Add(consoleSectionSizer, 1, wxEXPAND);
}

void MainTabController::enableCommandControls(bool enable) {
	commandInput->Enable(enable);
	sendCommandButton->Enable(enable);
}

void MainTabController::appendConsoleMessage(const string &message) {
	string prefix = ">> ";
	if(message.rfind("Send:", 0) == 0)
		prefix = "<< ";
	else if(message.rfind("Receive:", 0) == 0)
		prefix = ">> ";

	consoleOutput->AppendText(static_cast<string>(consoleOutput->GetValue().IsEmpty() ? "" : "\n") + prefix + message);
	consoleOutput->ShowPosition(consoleOutput->GetLastPosition());
}

void MainTabController::removeLastConsoleLine() {
	size_t end = consoleOutput->GetValue().Length();
	size_t start = consoleOutput->GetValue().find_last_of('\n');
	consoleOutput->Remove(start != string::npos ? start : 0, end);
	consoleOutput->ShowPosition(consoleOutput->GetLastPosition());
}

void MainTabController::clearCommandInput() {
	commandInput->SetValue("");
}

bool MainTabController::isCommandSubmissionEnabled() const {
	return sendCommandButton->IsEnabled();
}

void MainTabController::sendCommandManually(wxCommandEvent &event) {
	if(sendCommandButton->IsEnabled()) {
		string command = static_cast<string>(commandInput->GetValue());
		if(!command.empty()) {
			clearCommandInput();
			host->runManualCommand(command);
		}
	}
}
