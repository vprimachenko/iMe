#ifndef UI_LAYOUT_H
#define UI_LAYOUT_H

#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/frame.h>
#include <wx/notebook.h>
#include <wx/statbox.h>

struct UiLayout {
	wxPanel *rootPanel = nullptr;
	wxBoxSizer *rootSizer = nullptr;

	wxPanel *connectionSection = nullptr;
	wxBoxSizer *connectionSizer = nullptr;
	wxBoxSizer *connectionContentSizer = nullptr;
	wxPanel *statusRow = nullptr;
	wxBoxSizer *statusRowSizer = nullptr;

	wxPanel *connectedContent = nullptr;
	wxBoxSizer *connectedContentSizer = nullptr;
	wxNotebook *connectedNotebook = nullptr;
	wxPanel *mainTab = nullptr;
	wxBoxSizer *mainTabSizer = nullptr;
	wxPanel *controlTab = nullptr;
	wxBoxSizer *controlTabSizer = nullptr;
	wxPanel *firmwareTab = nullptr;
	wxBoxSizer *firmwareTabSizer = nullptr;
	wxPanel *printTab = nullptr;
	wxBoxSizer *printTabSizer = nullptr;

	wxPanel *firmwareSection = nullptr;
	wxBoxSizer *firmwareSizer = nullptr;
	wxPanel *consoleSection = nullptr;
	wxBoxSizer *consoleSizer = nullptr;
	wxPanel *movementSection = nullptr;
	wxStaticBoxSizer *movementSizer = nullptr;
	wxPanel *settingsSection = nullptr;
	wxStaticBoxSizer *settingsSizer = nullptr;
	wxPanel *miscellaneousSection = nullptr;
	wxStaticBoxSizer *miscellaneousSizer = nullptr;
	wxPanel *calibrationSection = nullptr;
	wxStaticBoxSizer *calibrationSizer = nullptr;
	wxPanel *printSection = nullptr;
	wxBoxSizer *printSizer = nullptr;

	wxPanel *footerSection = nullptr;
	wxBoxSizer *footerSizer = nullptr;
};

UiLayout buildMainLayout(wxFrame *frame);

#endif
