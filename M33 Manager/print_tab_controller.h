#ifndef PRINT_TAB_CONTROLLER_H
#define PRINT_TAB_CONTROLLER_H

#include <wx/button.h>
#include <wx/gauge.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include "gui_host.h"
#include "spinning_cube_view.h"
#include "ui_layout.h"

class PrintTabController {
	public:
		void build(const UiLayout &ui, GuiHost &host);
		void applyPrintJobStatus(const PrintJobStatus &status);

	private:
		GuiHost *host = nullptr;
		wxTextCtrl *filePathText = nullptr;
		wxStaticText *summaryText = nullptr;
		wxStaticText *statusText = nullptr;
		wxGauge *progressGauge = nullptr;
		wxStaticText *progressText = nullptr;
		wxButton *browseButton = nullptr;
		wxButton *startButton = nullptr;
		wxButton *pauseResumeButton = nullptr;
		wxButton *stopButton = nullptr;
		SpinningCubeView *preview = nullptr;
};

#endif
