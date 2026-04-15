#include <iomanip>
#include <sstream>
#include <wx/msgdlg.h>
#include "control_tab_controller.h"

using namespace std;

namespace {
constexpr int SAFE_MANUAL_FEED_RATE_XY = 600;
constexpr int SAFE_MANUAL_FEED_RATE_Z = 30;
}

void ControlTabController::build(const UiLayout &ui, GuiHost &newHost) {
	host = &newHost;

	backwardMovementButton = new wxButton(ui.movementSection, wxID_ANY, "↑");
	backwardMovementButton->Enable(false);

	leftMovementButton = new wxButton(ui.movementSection, wxID_ANY, "←");
	leftMovementButton->Enable(false);

	homeMovementButton = new wxButton(ui.movementSection, wxID_ANY, "Auto-center X/Y");
	homeMovementButton->SetMinSize(wxSize(130, -1));
	homeMovementButton->Enable(false);
	homeMovementButton->Bind(wxEVT_BUTTON, [=](wxCommandEvent &event) {
		host->runHome();
	});

	rightMovementButton = new wxButton(ui.movementSection, wxID_ANY, "→");
	rightMovementButton->Enable(false);

	forwardMovementButton = new wxButton(ui.movementSection, wxID_ANY, "↓");
	forwardMovementButton->Enable(false);

	upMovementButton = new wxButton(ui.movementSection, wxID_ANY, "Z+");
	upMovementButton->Enable(false);

	downMovementButton = new wxButton(ui.movementSection, wxID_ANY, "Z-");
	downMovementButton->Enable(false);

	auto bindMoveButton = [this](wxButton *button, const function<void()> &moveAction) {
		movementStepButtons.push_back(button);
		button->Bind(wxEVT_BUTTON, [moveAction](wxCommandEvent &event) {
			moveAction();
		});
	};

	auto createStepButton = [=](const string &label) {
		wxButton *button = new wxButton(ui.movementSection, wxID_ANY, label);
		button->SetMinSize(wxSize(54, -1));
		button->Enable(false);
		return button;
	};

	movementFeedRateText = new wxStaticText(
		ui.movementSection,
		wxID_ANY,
		"Manual move speeds: X/Y 600 mm/min, Z 30 mm/min",
		wxDefaultPosition,
		wxDefaultSize,
		wxALIGN_CENTRE_HORIZONTAL | wxST_NO_AUTORESIZE
	);
	movementFeedRateText->SetMinSize(wxSize(260, -1));

	wxButton *yLargeButton = createStepButton("10");
	wxButton *yMediumButton = createStepButton("1");
	wxButton *ySmallButton = createStepButton("0.1");
	wxButton *xNegativeLargeButton = createStepButton("10");
	wxButton *xNegativeMediumButton = createStepButton("1");
	wxButton *xNegativeSmallButton = createStepButton("0.1");
	wxButton *xPositiveSmallButton = createStepButton("0.1");
	wxButton *xPositiveMediumButton = createStepButton("1");
	wxButton *xPositiveLargeButton = createStepButton("10");
	wxButton *yNegativeSmallButton = createStepButton("0.1");
	wxButton *yNegativeMediumButton = createStepButton("1");
	wxButton *yNegativeLargeButton = createStepButton("10");
	wxButton *zLargeButton = createStepButton("10");
	wxButton *zMediumButton = createStepButton("1");
	wxButton *zSmallButton = createStepButton("0.1");
	wxButton *zNegativeSmallButton = createStepButton("0.1");
	wxButton *zNegativeMediumButton = createStepButton("1");
	wxButton *zNegativeLargeButton = createStepButton("10");

	bindMoveButton(yLargeButton, [=]() { host->runMoveY(true, 10.0, SAFE_MANUAL_FEED_RATE_XY); });
	bindMoveButton(yMediumButton, [=]() { host->runMoveY(true, 1.0, SAFE_MANUAL_FEED_RATE_XY); });
	bindMoveButton(ySmallButton, [=]() { host->runMoveY(true, 0.1, SAFE_MANUAL_FEED_RATE_XY); });
	bindMoveButton(xNegativeLargeButton, [=]() { host->runMoveX(false, 10.0, SAFE_MANUAL_FEED_RATE_XY); });
	bindMoveButton(xNegativeMediumButton, [=]() { host->runMoveX(false, 1.0, SAFE_MANUAL_FEED_RATE_XY); });
	bindMoveButton(xNegativeSmallButton, [=]() { host->runMoveX(false, 0.1, SAFE_MANUAL_FEED_RATE_XY); });
	bindMoveButton(xPositiveSmallButton, [=]() { host->runMoveX(true, 0.1, SAFE_MANUAL_FEED_RATE_XY); });
	bindMoveButton(xPositiveMediumButton, [=]() { host->runMoveX(true, 1.0, SAFE_MANUAL_FEED_RATE_XY); });
	bindMoveButton(xPositiveLargeButton, [=]() { host->runMoveX(true, 10.0, SAFE_MANUAL_FEED_RATE_XY); });
	bindMoveButton(yNegativeSmallButton, [=]() { host->runMoveY(false, 0.1, SAFE_MANUAL_FEED_RATE_XY); });
	bindMoveButton(yNegativeMediumButton, [=]() { host->runMoveY(false, 1.0, SAFE_MANUAL_FEED_RATE_XY); });
	bindMoveButton(yNegativeLargeButton, [=]() { host->runMoveY(false, 10.0, SAFE_MANUAL_FEED_RATE_XY); });
	bindMoveButton(zLargeButton, [=]() { host->runMoveZ(true, 10.0, SAFE_MANUAL_FEED_RATE_Z); });
	bindMoveButton(zMediumButton, [=]() { host->runMoveZ(true, 1.0, SAFE_MANUAL_FEED_RATE_Z); });
	bindMoveButton(zSmallButton, [=]() { host->runMoveZ(true, 0.1, SAFE_MANUAL_FEED_RATE_Z); });
	bindMoveButton(zNegativeSmallButton, [=]() { host->runMoveZ(false, 0.1, SAFE_MANUAL_FEED_RATE_Z); });
	bindMoveButton(zNegativeMediumButton, [=]() { host->runMoveZ(false, 1.0, SAFE_MANUAL_FEED_RATE_Z); });
	bindMoveButton(zNegativeLargeButton, [=]() { host->runMoveZ(false, 10.0, SAFE_MANUAL_FEED_RATE_Z); });

	wxGridBagSizer *movementGridSizer = new wxGridBagSizer(8, 8);
	backwardMovementButton->SetMinSize(wxSize(56, -1));
	leftMovementButton->SetMinSize(wxSize(56, -1));
	rightMovementButton->SetMinSize(wxSize(56, -1));
	forwardMovementButton->SetMinSize(wxSize(56, -1));
	upMovementButton->SetMinSize(wxSize(56, -1));
	downMovementButton->SetMinSize(wxSize(56, -1));

	movementGridSizer->Add(yLargeButton, wxGBPosition(0, 1));
	movementGridSizer->Add(yMediumButton, wxGBPosition(0, 2));
	movementGridSizer->Add(ySmallButton, wxGBPosition(0, 3));
	movementGridSizer->Add(backwardMovementButton, wxGBPosition(1, 2));
	movementGridSizer->Add(xNegativeLargeButton, wxGBPosition(2, 0));
	movementGridSizer->Add(xNegativeMediumButton, wxGBPosition(2, 1));
	movementGridSizer->Add(leftMovementButton, wxGBPosition(2, 2));
	movementGridSizer->Add(homeMovementButton, wxGBPosition(2, 3), wxGBSpan(1, 2), wxEXPAND);
	movementGridSizer->Add(rightMovementButton, wxGBPosition(2, 5));
	movementGridSizer->Add(xPositiveSmallButton, wxGBPosition(2, 6));
	movementGridSizer->Add(xPositiveMediumButton, wxGBPosition(2, 7));
	movementGridSizer->Add(xPositiveLargeButton, wxGBPosition(2, 8));
	movementGridSizer->Add(forwardMovementButton, wxGBPosition(3, 2));
	movementGridSizer->Add(yNegativeSmallButton, wxGBPosition(4, 1));
	movementGridSizer->Add(yNegativeMediumButton, wxGBPosition(4, 2));
	movementGridSizer->Add(yNegativeLargeButton, wxGBPosition(4, 3));
	movementGridSizer->Add(upMovementButton, wxGBPosition(0, 10));
	movementGridSizer->Add(zLargeButton, wxGBPosition(1, 10));
	movementGridSizer->Add(zMediumButton, wxGBPosition(2, 10));
	movementGridSizer->Add(zSmallButton, wxGBPosition(3, 10));
	movementGridSizer->Add(zNegativeSmallButton, wxGBPosition(5, 10));
	movementGridSizer->Add(zNegativeMediumButton, wxGBPosition(6, 10));
	movementGridSizer->Add(zNegativeLargeButton, wxGBPosition(7, 10));
	movementGridSizer->Add(downMovementButton, wxGBPosition(8, 10));
	movementGridSizer->AddGrowableCol(4);
	movementGridSizer->AddGrowableRow(2);

	wxBoxSizer *movementSectionSizer = new wxBoxSizer(wxVERTICAL);
	movementSectionSizer->Add(movementGridSizer, 0, wxALIGN_CENTER_HORIZONTAL | wxBOTTOM, 8);
	movementSectionSizer->Add(movementFeedRateText, 0, wxALIGN_CENTER_HORIZONTAL);
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
	for(wxButton *button : movementStepButtons)
		button->Enable(enable);
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
