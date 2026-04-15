#include <iomanip>
#include <sstream>
#include <wx/msgdlg.h>
#include "control_tab_controller.h"

using namespace std;

void ControlTabController::build(const UiLayout &ui, GuiHost &newHost) {
	host = &newHost;

	backwardMovementButton = new wxButton(ui.movementSection, wxID_ANY, "↑");
	backwardMovementButton->Enable(false);
	backwardMovementButton->Bind(wxEVT_BUTTON, [=](wxCommandEvent &event) {
		host->runMoveY(true, static_cast<double>(distanceMovementSlider->GetValue()) / 1000, feedRateMovementSlider->GetValue());
	});

	leftMovementButton = new wxButton(ui.movementSection, wxID_ANY, "←");
	leftMovementButton->Enable(false);
	leftMovementButton->Bind(wxEVT_BUTTON, [=](wxCommandEvent &event) {
		host->runMoveX(false, static_cast<double>(distanceMovementSlider->GetValue()) / 1000, feedRateMovementSlider->GetValue());
	});

	homeMovementButton = new wxButton(ui.movementSection, wxID_ANY, "Auto-center X/Y");
	homeMovementButton->SetMinSize(wxSize(130, -1));
	homeMovementButton->Enable(false);
	homeMovementButton->Bind(wxEVT_BUTTON, [=](wxCommandEvent &event) {
		host->runHome();
	});

	rightMovementButton = new wxButton(ui.movementSection, wxID_ANY, "→");
	rightMovementButton->Enable(false);
	rightMovementButton->Bind(wxEVT_BUTTON, [=](wxCommandEvent &event) {
		host->runMoveX(true, static_cast<double>(distanceMovementSlider->GetValue()) / 1000, feedRateMovementSlider->GetValue());
	});

	forwardMovementButton = new wxButton(ui.movementSection, wxID_ANY, "↓");
	forwardMovementButton->Enable(false);
	forwardMovementButton->Bind(wxEVT_BUTTON, [=](wxCommandEvent &event) {
		host->runMoveY(false, static_cast<double>(distanceMovementSlider->GetValue()) / 1000, feedRateMovementSlider->GetValue());
	});

	upMovementButton = new wxButton(ui.movementSection, wxID_ANY, "Z+");
	upMovementButton->Enable(false);
	upMovementButton->Bind(wxEVT_BUTTON, [=](wxCommandEvent &event) {
		host->runMoveZ(true, static_cast<double>(distanceMovementSlider->GetValue()) / 1000, feedRateMovementSlider->GetValue());
	});

	downMovementButton = new wxButton(ui.movementSection, wxID_ANY, "Z-");
	downMovementButton->Enable(false);
	downMovementButton->Bind(wxEVT_BUTTON, [=](wxCommandEvent &event) {
		host->runMoveZ(false, static_cast<double>(distanceMovementSlider->GetValue()) / 1000, feedRateMovementSlider->GetValue());
	});

	distanceMovementText = new wxStaticText(ui.movementSection, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL | wxST_NO_AUTORESIZE);
	distanceMovementText->SetMinSize(wxSize(180, -1));

	distanceMovementSlider = new wxSlider(ui.movementSection, wxID_ANY, 10 * 1000, 0.001 * 1000, 100 * 1000);
	distanceMovementSlider->SetMinSize(wxSize(180, -1));
	distanceMovementSlider->Enable(false);
	distanceMovementSlider->Bind(wxEVT_COMMAND_SLIDER_UPDATED, [=](wxCommandEvent &event) {
		updateDistanceMovementText();
	});
	updateDistanceMovementText();

	feedRateMovementText = new wxStaticText(ui.movementSection, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL | wxST_NO_AUTORESIZE);
	feedRateMovementText->SetMinSize(wxSize(180, -1));

	feedRateMovementSlider = new wxSlider(ui.movementSection, wxID_ANY, DEFAULT_X_SPEED, 1, 4800);
	feedRateMovementSlider->SetMinSize(wxSize(180, -1));
	feedRateMovementSlider->Enable(false);
	feedRateMovementSlider->Bind(wxEVT_COMMAND_SLIDER_UPDATED, [=](wxCommandEvent &event) {
		updateFeedRateMovementText();
	});
	updateFeedRateMovementText();

	wxGridBagSizer *movementGridSizer = new wxGridBagSizer(8, 8);
	movementGridSizer->Add(backwardMovementButton, wxGBPosition(0, 1));
	movementGridSizer->Add(leftMovementButton, wxGBPosition(1, 0));
	movementGridSizer->Add(homeMovementButton, wxGBPosition(1, 1));
	movementGridSizer->Add(rightMovementButton, wxGBPosition(1, 2));
	movementGridSizer->Add(forwardMovementButton, wxGBPosition(2, 1));
	movementGridSizer->Add(upMovementButton, wxGBPosition(0, 3));
	movementGridSizer->Add(downMovementButton, wxGBPosition(1, 3));
	movementGridSizer->AddGrowableCol(0);
	movementGridSizer->AddGrowableCol(1);
	movementGridSizer->AddGrowableCol(2);
	movementGridSizer->AddGrowableCol(3);
	wxBoxSizer *movementSectionSizer = new wxBoxSizer(wxVERTICAL);
	movementSectionSizer->Add(movementGridSizer, 0, wxALIGN_CENTER_HORIZONTAL | wxBOTTOM, 8);
	movementSectionSizer->Add(distanceMovementText, 0, wxEXPAND | wxBOTTOM, 4);
	movementSectionSizer->Add(distanceMovementSlider, 0, wxEXPAND | wxBOTTOM, 8);
	movementSectionSizer->Add(feedRateMovementText, 0, wxEXPAND | wxBOTTOM, 4);
	movementSectionSizer->Add(feedRateMovementSlider, 0, wxEXPAND);
	ui.movementSizer->Add(movementSectionSizer, 1, wxEXPAND | wxALL, 8);

	wxArrayString printerSettings;
	vector<string> temp = host->getPrinterSettingNames();
	for(uint8_t i = 0; i < temp.size(); i++)
		printerSettings.Add(temp[i]);

	printerSettingChoice = new wxChoice(ui.settingsSection, wxID_ANY, wxDefaultPosition, wxDefaultSize, printerSettings);
	printerSettingChoice->SetSelection(printerSettingChoice->FindString(printerSettings[0]));
	printerSettingChoice->Enable(false);
	printerSettingChoice->Bind(wxEVT_CHOICE, [=](wxCommandEvent &event) {
		setPrinterSettingValue();
	});

	printerSettingInput = new wxTextCtrl(ui.settingsSection, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
	printerSettingInput->SetHint("Value");
	printerSettingInput->Bind(wxEVT_TEXT_ENTER, &ControlTabController::savePrinterSetting, this);

	savePrinterSettingButton = new wxButton(ui.settingsSection, wxID_ANY, "Save");
	savePrinterSettingButton->Enable(false);
	savePrinterSettingButton->Bind(wxEVT_BUTTON, &ControlTabController::savePrinterSetting, this);

	wxBoxSizer *settingsSectionSizer = new wxBoxSizer(wxVERTICAL);
	settingsSectionSizer->Add(new wxStaticText(ui.settingsSection, wxID_ANY, "Printer Setting"), 0, wxBOTTOM, 4);
	settingsSectionSizer->Add(printerSettingChoice, 0, wxEXPAND | wxBOTTOM, 8);
	wxBoxSizer *settingsInputSizer = new wxBoxSizer(wxHORIZONTAL);
	settingsInputSizer->Add(printerSettingInput, 1, wxEXPAND | wxRIGHT, 8);
	settingsInputSizer->Add(savePrinterSettingButton, 0, wxEXPAND);
	settingsSectionSizer->Add(settingsInputSizer, 0, wxEXPAND);
	ui.settingsSizer->Add(settingsSectionSizer, 1, wxEXPAND | wxALL, 8);

	motorsOnButton = new wxButton(ui.miscellaneousSection, wxID_ANY, "Motors on");
	motorsOnButton->Enable(false);
	motorsOnButton->Bind(wxEVT_BUTTON, [=](wxCommandEvent &event) {
		host->runMotorsOn();
	});

	motorsOffButton = new wxButton(ui.miscellaneousSection, wxID_ANY, "Motors off");
	motorsOffButton->Enable(false);
	motorsOffButton->Bind(wxEVT_BUTTON, [=](wxCommandEvent &event) {
		host->runMotorsOff();
	});

	fanOnButton = new wxButton(ui.miscellaneousSection, wxID_ANY, "Fan on");
	fanOnButton->Enable(false);
	fanOnButton->Bind(wxEVT_BUTTON, [=](wxCommandEvent &event) {
		host->runFanOn();
	});

	fanOffButton = new wxButton(ui.miscellaneousSection, wxID_ANY, "Fan off");
	fanOffButton->Enable(false);
	fanOffButton->Bind(wxEVT_BUTTON, [=](wxCommandEvent &event) {
		host->runFanOff();
	});

	wxBoxSizer *miscellaneousSectionSizer = new wxBoxSizer(wxVERTICAL);
	miscellaneousSectionSizer->Add(motorsOnButton, 0, wxEXPAND | wxBOTTOM, 8);
	miscellaneousSectionSizer->Add(motorsOffButton, 0, wxEXPAND | wxBOTTOM, 8);
	miscellaneousSectionSizer->Add(fanOnButton, 0, wxEXPAND | wxBOTTOM, 8);
	miscellaneousSectionSizer->Add(fanOffButton, 0, wxEXPAND);
	ui.miscellaneousSizer->Add(miscellaneousSectionSizer, 1, wxEXPAND | wxALL, 8);

	calibrateBedPositionButton = new wxButton(ui.calibrationSection, wxID_ANY, "Calibrate bed position");
	calibrateBedPositionButton->Enable(false);
	calibrateBedPositionButton->Bind(wxEVT_BUTTON, [=](wxCommandEvent &event) {
		host->runCalibrateBedPosition();
	});

	calibrateBedOrientationButton = new wxButton(ui.calibrationSection, wxID_ANY, "Calibrate bed orientation");
	calibrateBedOrientationButton->Enable(false);
	calibrateBedOrientationButton->Bind(wxEVT_BUTTON, [=](wxCommandEvent &event) {
		host->runCalibrateBedOrientation();
	});

	saveCurrentPositionAsHomeButton = new wxButton(ui.calibrationSection, wxID_ANY, "Set current X/Y as home");
	saveCurrentPositionAsHomeButton->Enable(false);
	saveCurrentPositionAsHomeButton->Bind(wxEVT_BUTTON, [=](wxCommandEvent &event) {
		wxMessageDialog confirmDialog(
			ui.calibrationSection,
			"Use the movement controls to position the print head at the desired manual home corner, then save that current X/Y position as home.\n\nThis does not perform an automatic homing move. Continue?",
			"M33 Manager",
			wxYES_NO | wxYES_DEFAULT | wxICON_QUESTION
		);
		if(confirmDialog.ShowModal() == wxID_YES)
			host->runSaveCurrentPositionAsHome();
	});

	saveZAsZeroButton = new wxButton(ui.calibrationSection, wxID_ANY, "Set current height as Z0");
	saveZAsZeroButton->Enable(false);
	saveZAsZeroButton->Bind(wxEVT_BUTTON, [=](wxCommandEvent &event) {
		wxMessageDialog confirmDialog(
			ui.calibrationSection,
			"Use the movement controls to position the nozzle at the desired first-layer height, then save that current height as Z0.\n\nThis does not probe the bed. Continue?",
			"M33 Manager",
			wxYES_NO | wxYES_DEFAULT | wxICON_QUESTION
		);
		if(confirmDialog.ShowModal() == wxID_YES)
			host->runSaveZAsZero();
	});

	wxBoxSizer *calibrationSectionSizer = new wxBoxSizer(wxVERTICAL);
	calibrationSectionSizer->Add(calibrateBedPositionButton, 0, wxEXPAND | wxBOTTOM, 8);
	calibrationSectionSizer->Add(calibrateBedOrientationButton, 0, wxEXPAND | wxBOTTOM, 8);
	calibrationSectionSizer->Add(saveCurrentPositionAsHomeButton, 0, wxEXPAND | wxBOTTOM, 8);
	calibrationSectionSizer->Add(saveZAsZeroButton, 0, wxEXPAND);
	ui.calibrationSizer->Add(calibrationSectionSizer, 1, wxEXPAND | wxALL, 8);
}

void ControlTabController::enableMovementControls(bool enable) {
	backwardMovementButton->Enable(enable);
	forwardMovementButton->Enable(enable);
	rightMovementButton->Enable(enable);
	leftMovementButton->Enable(enable);
	upMovementButton->Enable(enable);
	downMovementButton->Enable(enable);
	homeMovementButton->Enable(enable);
	distanceMovementSlider->Enable(enable);
	feedRateMovementSlider->Enable(enable);
}

void ControlTabController::enableSettingsControls(bool enable) {
	printerSettingChoice->Enable(enable);
	savePrinterSettingButton->Enable(enable);

	if(enable) {
		setPrinterSettingValue();
	}
}

void ControlTabController::enableMiscellaneousControls(bool enable) {
	motorsOnButton->Enable(enable);
	motorsOffButton->Enable(enable);
	fanOnButton->Enable(enable);
	fanOffButton->Enable(enable);
}

void ControlTabController::enableCalibrationControls(bool enable) {
	calibrateBedPositionButton->Enable(enable);
	calibrateBedOrientationButton->Enable(enable);
	saveCurrentPositionAsHomeButton->Enable(enable);
	saveZAsZeroButton->Enable(enable);
}

void ControlTabController::updateDistanceMovementText() {
	stringstream stream;
	stream << fixed << setprecision(3) << static_cast<double>(distanceMovementSlider->GetValue()) / 1000;
	distanceMovementText->SetLabel("Distance: " + stream.str() + "mm");
}

void ControlTabController::updateFeedRateMovementText() {
	feedRateMovementText->SetLabel("Feed rate: " + to_string(feedRateMovementSlider->GetValue()) + "mm/min");
}

void ControlTabController::setPrinterSettingValue() {
	printerSettingInput->SetValue(host->loadPrinterSettingValue(static_cast<string>(printerSettingChoice->GetString(printerSettingChoice->GetSelection()))));
}

void ControlTabController::savePrinterSetting(wxCommandEvent &event) {
	if(savePrinterSettingButton->IsEnabled()) {
		string value = static_cast<string>(printerSettingInput->GetValue());
		if(!value.empty())
			host->savePrinterSetting(static_cast<string>(printerSettingChoice->GetString(printerSettingChoice->GetSelection())), value);
	}
}
