#ifndef UI_LAYOUT_H
#define UI_LAYOUT_H

#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/frame.h>
#include <wx/statbox.h>

struct UiLayout {
	wxPanel *rootPanel = nullptr;
	wxBoxSizer *rootSizer = nullptr;

	wxPanel *connectionSection = nullptr;
	wxStaticBoxSizer *connectionSizer = nullptr;
	wxBoxSizer *connectionContentSizer = nullptr;
	wxPanel *statusRow = nullptr;
	wxBoxSizer *statusRowSizer = nullptr;

	wxPanel *connectedContent = nullptr;
	wxBoxSizer *connectedContentSizer = nullptr;
	wxBoxSizer *connectedLeftColumnSizer = nullptr;
	wxBoxSizer *connectedRightColumnSizer = nullptr;

	wxPanel *firmwareSection = nullptr;
	wxStaticBoxSizer *firmwareSizer = nullptr;
	wxPanel *consoleSection = nullptr;
	wxStaticBoxSizer *consoleSizer = nullptr;
	wxPanel *movementSection = nullptr;
	wxStaticBoxSizer *movementSizer = nullptr;
	wxPanel *settingsSection = nullptr;
	wxStaticBoxSizer *settingsSizer = nullptr;
	wxPanel *miscellaneousSection = nullptr;
	wxStaticBoxSizer *miscellaneousSizer = nullptr;
	wxPanel *calibrationSection = nullptr;
	wxStaticBoxSizer *calibrationSizer = nullptr;

	wxPanel *footerSection = nullptr;
	wxBoxSizer *footerSizer = nullptr;
};

UiLayout buildMainLayout(wxFrame *frame);

#endif
