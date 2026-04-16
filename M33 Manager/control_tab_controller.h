#ifndef CONTROL_TAB_CONTROLLER_H
#define CONTROL_TAB_CONTROLLER_H

#include <wx/button.h>
#include <wx/choice.h>
#include <wx/gbsizer.h>
#include <wx/slider.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <vector>
#include "gui_host.h"
#include "ui_layout.h"

class ControlTabController {
	public:
		void build(const UiLayout &ui, GuiHost &host);
		void enableMovementControls(bool enable);
			void enableSettingsControls(bool enable);
			void enableMiscellaneousControls(bool enable);
			void enableCalibrationControls(bool enable);
			void enableStandbyControls(bool enable);
			void setPrinterSettingValue();

	private:
		void savePrinterSetting(wxCommandEvent &event);
		void updateStandbyTemperatureLabel();
		void updateManualNozzleTemperatureLabel();

		GuiHost *host = nullptr;
		wxButton *backwardMovementButton = nullptr;
		wxButton *forwardMovementButton = nullptr;
		wxButton *rightMovementButton = nullptr;
		wxButton *leftMovementButton = nullptr;
			wxButton *upMovementButton = nullptr;
			wxButton *downMovementButton = nullptr;
			wxButton *homeMovementButton = nullptr;
			wxStaticText *movementFeedRateText = nullptr;
			std::vector<wxButton *> movementStepButtons;
			wxChoice *printerSettingChoice = nullptr;
			wxTextCtrl *printerSettingInput = nullptr;
			wxButton *savePrinterSettingButton = nullptr;
			wxButton *motorsOnButton = nullptr;
			wxButton *motorsOffButton = nullptr;
			wxButton *fanOnButton = nullptr;
			wxButton *fanOffButton = nullptr;
			wxStaticText *manualNozzleTemperatureText = nullptr;
			wxSlider *manualNozzleTemperatureSlider = nullptr;
			wxButton *heatNozzleButton = nullptr;
			wxButton *coolDownNozzleButton = nullptr;
			wxStaticText *pauseStandbyTemperatureText = nullptr;
			wxSlider *pauseStandbyTemperatureSlider = nullptr;
			wxButton *calibrateBedPositionButton = nullptr;
			wxButton *calibrateBedOrientationButton = nullptr;
			wxButton *saveCurrentPositionAsHomeButton = nullptr;
			wxButton *saveZAsZeroButton = nullptr;
	};

#endif
