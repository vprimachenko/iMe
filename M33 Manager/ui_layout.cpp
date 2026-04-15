#include <wx/statbox.h>
#include "ui_layout.h"

namespace {

wxPanel *createSectionPanel(wxWindow *parent, const wxString &title, wxStaticBoxSizer *&sectionSizer) {
	wxPanel *panel = new wxPanel(parent, wxID_ANY);
	sectionSizer = new wxStaticBoxSizer(wxVERTICAL, panel, title);
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
	layout.connectedContentSizer = new wxBoxSizer(wxHORIZONTAL);
	layout.connectedContent->SetSizer(layout.connectedContentSizer);
	layout.connectedLeftColumnSizer = new wxBoxSizer(wxVERTICAL);
	layout.connectedRightColumnSizer = new wxBoxSizer(wxVERTICAL);
	layout.connectedContentSizer->Add(layout.connectedLeftColumnSizer, 1, wxEXPAND | wxRIGHT, 8);
	layout.connectedContentSizer->Add(layout.connectedRightColumnSizer, 0, wxEXPAND);

	layout.firmwareSection = createSectionPanel(layout.connectedContent, "Firmware", layout.firmwareSizer);
	layout.consoleSection = createSectionPanel(layout.connectedContent, "Console", layout.consoleSizer);
	layout.movementSection = createSectionPanel(layout.connectedContent, "Movement", layout.movementSizer);
	layout.settingsSection = createSectionPanel(layout.connectedContent, "Settings", layout.settingsSizer);
	layout.miscellaneousSection = createSectionPanel(layout.connectedContent, "Miscellaneous", layout.miscellaneousSizer);
	layout.calibrationSection = createSectionPanel(layout.connectedContent, "Calibration", layout.calibrationSizer);

	layout.connectedLeftColumnSizer->Add(layout.firmwareSection, 0, wxEXPAND | wxBOTTOM, 8);
	layout.connectedLeftColumnSizer->Add(layout.consoleSection, 1, wxEXPAND);
	layout.connectedRightColumnSizer->Add(layout.movementSection, 0, wxEXPAND | wxBOTTOM, 8);
	layout.connectedRightColumnSizer->Add(layout.settingsSection, 0, wxEXPAND | wxBOTTOM, 8);
	layout.connectedRightColumnSizer->Add(layout.miscellaneousSection, 0, wxEXPAND | wxBOTTOM, 8);
	layout.connectedRightColumnSizer->Add(layout.calibrationSection, 0, wxEXPAND);

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
