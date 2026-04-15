#include "common.h"
#include "firmware_tab_controller.h"

void FirmwareTabController::build(const UiLayout &ui, GuiHost &newHost) {
	host = &newHost;

	string iMeVersion = static_cast<string>(TOSTRING(IME_ROM_VERSION_STRING)).substr(2);
	for(uint8_t i = 0; i < 3; i++)
		iMeVersion.insert(i * 2 + 2 + i, ".");

	installImeFirmwareButton = new wxButton(ui.firmwareSection, wxID_ANY, "Install iMe V" + iMeVersion);
	installImeFirmwareButton->Enable(false);
	installImeFirmwareButton->Bind(wxEVT_BUTTON, [=](wxCommandEvent &event) {
		host->installImeFirmware();
	});

	installM3dFirmwareButton = new wxButton(ui.firmwareSection, wxID_ANY, "Install M3D V" TOSTRING(M3D_ROM_VERSION_STRING));
	installM3dFirmwareButton->Enable(false);
	installM3dFirmwareButton->Bind(wxEVT_BUTTON, [=](wxCommandEvent &event) {
		host->installM3dFirmware();
	});

	installFirmwareFromFileButton = new wxButton(ui.firmwareSection, wxID_ANY, "Install firmware from file");
	installFirmwareFromFileButton->Enable(false);
	installFirmwareFromFileButton->Bind(wxEVT_BUTTON, [=](wxCommandEvent &event) {
		host->installFirmwareFromFile();
	});

	wxBoxSizer *firmwareRowsSizer = new wxBoxSizer(wxVERTICAL);
	firmwareRowsSizer->Add(installImeFirmwareButton, 0, wxEXPAND | wxBOTTOM, 8);
	firmwareRowsSizer->Add(installM3dFirmwareButton, 0, wxEXPAND | wxBOTTOM, 8);
	firmwareRowsSizer->Add(installFirmwareFromFileButton, 0, wxEXPAND);
	#ifndef MACOS
		installDriversButton = new wxButton(ui.firmwareSection, wxID_ANY, "Install drivers");
		installDriversButton->Bind(wxEVT_BUTTON, [=](wxCommandEvent &event) {
			host->installDrivers();
		});
		firmwareRowsSizer->AddSpacer(12);
		firmwareRowsSizer->Add(installDriversButton, 0, wxEXPAND);
	#endif
	ui.firmwareSizer->Add(firmwareRowsSizer, 0, wxEXPAND);
}

void FirmwareTabController::enableFirmwareControls(bool enable) {
	installFirmwareFromFileButton->Enable(enable);
	installImeFirmwareButton->Enable(enable);
	installM3dFirmwareButton->Enable(enable);
}

void FirmwareTabController::setInstallDriversEnabled(bool enable) {
	#ifndef MACOS
		installDriversButton->Enable(enable);
	#endif
}
