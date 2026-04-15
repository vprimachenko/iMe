#include <wx/statbox.h>
#include "ui_layout.h"

namespace {

wxPanel *createSectionPanel(wxWindow *parent, const wxString &title, wxStaticBoxSizer *&sectionSizer) {
	wxPanel *panel = new wxPanel(parent, wxID_ANY);
	sectionSizer = new wxStaticBoxSizer(wxVERTICAL, panel, title);
	panel->SetSizer(sectionSizer);
	return panel;
}

wxPanel *createPlainSectionPanel(wxWindow *parent, wxBoxSizer *&sectionSizer) {
	wxPanel *panel = new wxPanel(parent, wxID_ANY);
	sectionSizer = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(sectionSizer);
	return panel;
}

}

UiLayout buildMainLayout(wxFrame *frame) {
	UiLayout layout;

	layout.rootPanel = new wxPanel(frame, wxID_ANY);
	layout.rootSizer = new wxBoxSizer(wxVERTICAL);
	layout.rootPanel->SetSizer(layout.rootSizer);

	layout.connectionSection = createSectionPanel(layout.rootPanel, "Connection", layout.connectionSizer);
	layout.connectionContentSizer = new wxBoxSizer(wxVERTICAL);
	layout.connectionSizer->Add(layout.connectionContentSizer, 1, wxEXPAND | wxALL, 8);

	layout.statusRow = new wxPanel(layout.connectionSection, wxID_ANY);
	layout.statusRowSizer = new wxBoxSizer(wxHORIZONTAL);
	layout.statusRow->SetSizer(layout.statusRowSizer);

	layout.connectedContent = new wxPanel(layout.rootPanel, wxID_ANY);
	layout.connectedContentSizer = new wxBoxSizer(wxVERTICAL);
	layout.connectedContent->SetSizer(layout.connectedContentSizer);
	layout.connectedNotebook = new wxNotebook(layout.connectedContent, wxID_ANY);
	layout.connectedContentSizer->Add(layout.connectedNotebook, 1, wxEXPAND);

	layout.mainTab = new wxPanel(layout.connectedNotebook, wxID_ANY);
	layout.mainTabSizer = new wxBoxSizer(wxVERTICAL);
	layout.mainTab->SetSizer(layout.mainTabSizer);

	layout.controlTab = new wxPanel(layout.connectedNotebook, wxID_ANY);
	layout.controlTabSizer = new wxBoxSizer(wxVERTICAL);
	layout.controlTab->SetSizer(layout.controlTabSizer);

	layout.firmwareTab = new wxPanel(layout.connectedNotebook, wxID_ANY);
	layout.firmwareTabSizer = new wxBoxSizer(wxVERTICAL);
	layout.firmwareTab->SetSizer(layout.firmwareTabSizer);

	layout.printTab = new wxPanel(layout.connectedNotebook, wxID_ANY);
	layout.printTabSizer = new wxBoxSizer(wxVERTICAL);
	layout.printTab->SetSizer(layout.printTabSizer);

	layout.connectedNotebook->AddPage(layout.printTab, "Print");
	layout.connectedNotebook->AddPage(layout.controlTab, "Control");
	layout.connectedNotebook->AddPage(layout.mainTab, "Command");
	layout.connectedNotebook->AddPage(layout.firmwareTab, "Manage");

	layout.consoleSection = createPlainSectionPanel(layout.mainTab, layout.consoleSizer);
	layout.mainTabSizer->Add(layout.consoleSection, 1, wxEXPAND | wxALL, 8);

	layout.movementSection = createSectionPanel(layout.controlTab, "Movement", layout.movementSizer);
	layout.settingsSection = createSectionPanel(layout.controlTab, "Settings", layout.settingsSizer);
	layout.miscellaneousSection = createSectionPanel(layout.controlTab, "Miscellaneous", layout.miscellaneousSizer);
	layout.calibrationSection = createSectionPanel(layout.controlTab, "Calibration", layout.calibrationSizer);
	layout.controlTabSizer->Add(layout.movementSection, 0, wxEXPAND | wxALL, 8);
	wxBoxSizer *controlLowerRowSizer = new wxBoxSizer(wxHORIZONTAL);
	controlLowerRowSizer->Add(layout.settingsSection, 1, wxEXPAND | wxRIGHT, 8);
	controlLowerRowSizer->Add(layout.miscellaneousSection, 1, wxEXPAND | wxRIGHT, 8);
	controlLowerRowSizer->Add(layout.calibrationSection, 1, wxEXPAND);
	layout.controlTabSizer->Add(controlLowerRowSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

	layout.firmwareSection = createPlainSectionPanel(layout.firmwareTab, layout.firmwareSizer);
	layout.firmwareTabSizer->Add(layout.firmwareSection, 0, wxEXPAND | wxALL, 8);

	layout.printSection = createPlainSectionPanel(layout.printTab, layout.printSizer);
	layout.printTabSizer->Add(layout.printSection, 1, wxEXPAND | wxALL, 8);

	layout.footerSection = new wxPanel(layout.rootPanel, wxID_ANY);
	layout.footerSizer = new wxBoxSizer(wxHORIZONTAL);
	layout.footerSection->SetSizer(layout.footerSizer);

	layout.rootSizer->Add(layout.connectionSection, 0, wxEXPAND | wxALL, 8);
	layout.rootSizer->Add(layout.connectedContent, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);
	layout.rootSizer->Add(layout.footerSection, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

	wxBoxSizer *frameSizer = new wxBoxSizer(wxVERTICAL);
	frameSizer->Add(layout.rootPanel, 1, wxEXPAND);
	frame->SetSizer(frameSizer);

	return layout;
}
