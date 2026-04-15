#ifndef FIRMWARE_TAB_CONTROLLER_H
#define FIRMWARE_TAB_CONTROLLER_H

#include <wx/button.h>
#include "gui_host.h"
#include "ui_layout.h"

class FirmwareTabController {
	public:
		void build(const UiLayout &ui, GuiHost &host);
		void enableFirmwareControls(bool enable);
		void setInstallDriversEnabled(bool enable);

	private:
		GuiHost *host = nullptr;
		wxButton *installFirmwareFromFileButton = nullptr;
		wxButton *installImeFirmwareButton = nullptr;
		wxButton *installM3dFirmwareButton = nullptr;
		#ifndef MACOS
			wxButton *installDriversButton = nullptr;
		#endif
};

#endif
