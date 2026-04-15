#ifndef SPINNING_CUBE_VIEW_H
#define SPINNING_CUBE_VIEW_H

#include "gui_host.h"
#include <wx/panel.h>

class SpinningCubeView : public wxPanel {
	public:
		explicit SpinningCubeView(wxWindow *parent);
		void setPreparedPrintJob(const PreparedPrintJob &job, size_t completedCommandCount);

	private:
		wxWindow *content = nullptr;
};

#endif
