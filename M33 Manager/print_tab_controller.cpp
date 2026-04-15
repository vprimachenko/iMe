#include <algorithm>
#include <wx/sizer.h>
#include "print_tab_controller.h"

void PrintTabController::build(const UiLayout &ui, GuiHost &newHost) {
	host = &newHost;

	filePathText = new wxTextCtrl(ui.printSection, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
	filePathText->SetHint("No G-code file selected");

	browseButton = new wxButton(ui.printSection, wxID_ANY, "Browse...");
	browseButton->Bind(wxEVT_BUTTON, [=](wxCommandEvent &event) {
		host->loadPrintFile();
	});

	summaryText = new wxStaticText(ui.printSection, wxID_ANY, "Choose a G-code file to prepare it for printing.");
	statusText = new wxStaticText(ui.printSection, wxID_ANY, "No print job loaded");

	preview = new SpinningCubeView(ui.printSection);

	progressGauge = new wxGauge(ui.printSection, wxID_ANY, 1);
	progressText = new wxStaticText(ui.printSection, wxID_ANY, "0 / 0");

	startButton = new wxButton(ui.printSection, wxID_ANY, "Start");
	startButton->Enable(false);
	startButton->Bind(wxEVT_BUTTON, [=](wxCommandEvent &event) {
		host->startPrintJob();
	});

	pauseResumeButton = new wxButton(ui.printSection, wxID_ANY, "Pause");
	pauseResumeButton->Hide();
	pauseResumeButton->Bind(wxEVT_BUTTON, [=](wxCommandEvent &event) {
		PrintJobStatus status = host->getPrintJobStatus();
		if(status.state == PRINT_JOB_PAUSED)
			host->resumePrintJob();
		else
			host->pausePrintJob();
	});

	stopButton = new wxButton(ui.printSection, wxID_ANY, "Stop");
	stopButton->Hide();
	stopButton->Bind(wxEVT_BUTTON, [=](wxCommandEvent &event) {
		host->stopPrintJob();
	});

	wxBoxSizer *topRowSizer = new wxBoxSizer(wxHORIZONTAL);
	topRowSizer->Add(filePathText, 1, wxEXPAND | wxRIGHT, 8);
	topRowSizer->Add(browseButton, 0, wxEXPAND);

	wxBoxSizer *progressSizer = new wxBoxSizer(wxHORIZONTAL);
	progressSizer->Add(progressGauge, 1, wxEXPAND | wxRIGHT, 8);
	progressSizer->Add(progressText, 0, wxALIGN_CENTER_VERTICAL);

	wxBoxSizer *controlSizer = new wxBoxSizer(wxHORIZONTAL);
	controlSizer->Add(startButton, 0, wxRIGHT, 8);
	controlSizer->Add(pauseResumeButton, 0, wxRIGHT, 8);
	controlSizer->Add(stopButton, 0);

	ui.printSizer->Add(topRowSizer, 0, wxEXPAND | wxBOTTOM, 8);
	ui.printSizer->Add(summaryText, 0, wxEXPAND | wxBOTTOM, 4);
	ui.printSizer->Add(statusText, 0, wxEXPAND | wxBOTTOM, 8);
	ui.printSizer->Add(preview, 1, wxEXPAND | wxBOTTOM, 8);
	ui.printSizer->Add(progressSizer, 0, wxEXPAND | wxBOTTOM, 8);
	ui.printSizer->Add(controlSizer, 0, wxALIGN_LEFT);

	applyPrintJobStatus(host->getPrintJobStatus());
}

void PrintTabController::applyPrintJobStatus(const PrintJobStatus &status) {
	preview->setPreparedPrintJob(host->getPreparedPrintJob(), status.currentCommandIndex);

	filePathText->SetValue(status.filePath.empty() ? "" : status.filePath);
	if(status.filePath.empty())
		filePathText->SetHint("No G-code file selected");

	summaryText->SetLabel(status.summaryText.empty() ? "Choose a G-code file to prepare it for printing." : status.summaryText);

	wxString statusLabel = status.statusText.empty() ? "No print job loaded" : status.statusText;
	if(!status.errorText.empty())
		statusLabel = status.errorText;
	if(!interactiveEnabled && !interactiveDisabledMessage.empty() && status.state != PRINT_JOB_RUNNING && status.state != PRINT_JOB_PAUSED && status.state != PRINT_JOB_STOPPING)
		statusLabel = interactiveDisabledMessage;
	statusText->SetLabel(statusLabel);

	const int range = static_cast<int>(std::max<size_t>(status.totalCommands, 1));
	const int value = static_cast<int>(std::min(status.currentCommandIndex, status.totalCommands));
	progressGauge->SetRange(range);
	progressGauge->SetValue(value);
	progressText->SetLabel(wxString::Format("%zu / %zu", status.currentCommandIndex, status.totalCommands));

	const bool hasLoadedJob = !status.filePath.empty() && status.totalCommands > 0;
	const bool printing = status.state == PRINT_JOB_RUNNING;
	const bool paused = status.state == PRINT_JOB_PAUSED;
	const bool stopping = status.state == PRINT_JOB_STOPPING;

	browseButton->Enable(interactiveEnabled && !printing && !paused && !stopping);

	const bool allowStart = hasLoadedJob && (status.state == PRINT_JOB_LOADED || status.state == PRINT_JOB_COMPLETED || status.state == PRINT_JOB_FAILED);
	startButton->Show(allowStart);
	startButton->Enable(interactiveEnabled && allowStart);

	pauseResumeButton->Show(printing || paused);
	pauseResumeButton->SetLabel(paused ? "Resume" : "Pause");
	pauseResumeButton->Enable(interactiveEnabled && !stopping);

	stopButton->Show(printing || paused || stopping);
	stopButton->Enable(interactiveEnabled && !stopping);

	if(filePathText && filePathText->GetParent())
		filePathText->GetParent()->Layout();
}

void PrintTabController::setInteractiveEnabled(bool enabled, const string &disabledMessage) {
	interactiveEnabled = enabled;
	interactiveDisabledMessage = disabledMessage;
	applyPrintJobStatus(host->getPrintJobStatus());
}
