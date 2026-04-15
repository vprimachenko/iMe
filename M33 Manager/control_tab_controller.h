#ifndef CONTROL_TAB_CONTROLLER_H
#define CONTROL_TAB_CONTROLLER_H

#include <wx/button.h>
#include <wx/choice.h>
#include <wx/gbsizer.h>
#include <wx/slider.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include "gui_host.h"
#include "ui_layout.h"

class ControlTabController {
	public:
		void build(const UiLayout &ui, GuiHost &host);
		void enableMovementControls(bool enable);
		void enableSettingsControls(bool enable);
		void enableMiscellaneousControls(bool enable);
		void enableCalibrationControls(bool enable);
		void updateDistanceMovementText();
		void updateFeedRateMovementText();
		void setPrinterSettingValue();

	private:
		void savePrinterSetting(wxCommandEvent &event);

		GuiHost *host = nullptr;
		wxButton *backwardMovementButton = nullptr;
		wxButton *forwardMovementButton = nullptr;
		wxButton *rightMovementButton = nullptr;
		wxButton *leftMovementButton = nullptr;
		wxButton *upMovementButton = nullptr;
		wxButton *downMovementButton = nullptr;
		wxButton *homeMovementButton = nullptr;
		wxStaticText *distanceMovementText = nullptr;
		wxStaticText *feedRateMovementText = nullptr;
		wxSlider *distanceMovementSlider = nullptr;
		wxSlider *feedRateMovementSlider = nullptr;
		wxChoice *printerSettingChoice = nullptr;
		wxTextCtrl *printerSettingInput = nullptr;
		wxButton *savePrinterSettingButton = nullptr;
		wxButton *motorsOnButton = nullptr;
		wxButton *motorsOffButton = nullptr;
		wxButton *fanOnButton = nullptr;
		wxButton *fanOffButton = nullptr;
			wxButton *calibrateBedPositionButton = nullptr;
			wxButton *calibrateBedOrientationButton = nullptr;
			wxButton *saveCurrentPositionAsHomeButton = nullptr;
			wxButton *saveZAsZeroButton = nullptr;
};

#endif
