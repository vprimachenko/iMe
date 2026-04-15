#ifndef GUI_HOST_H
#define GUI_HOST_H

#include <functional>
#include <string>
#include <vector>
#include "printer.h"

using namespace std;

struct ThreadTaskResponse {
	string message;
	int style;
};

class GuiHost {
	public:
		virtual ~GuiHost() {}
		virtual vector<string> getPrinterSettingNames() = 0;
		virtual string loadPrinterSettingValue(const string &settingName) = 0;
		virtual void runManualCommand(const string &command) = 0;
		virtual void runMoveX(bool positive, double distanceMm, int feedRate) = 0;
		virtual void runMoveY(bool positive, double distanceMm, int feedRate) = 0;
		virtual void runMoveZ(bool positive, double distanceMm, int feedRate) = 0;
		virtual void runHome() = 0;
		virtual void runMotorsOn() = 0;
		virtual void runMotorsOff() = 0;
		virtual void runFanOn() = 0;
		virtual void runFanOff() = 0;
		virtual void runCalibrateBedPosition() = 0;
		virtual void runCalibrateBedOrientation() = 0;
		virtual void runSaveZAsZero() = 0;
		virtual void savePrinterSetting(const string &settingName, const string &value) = 0;
		virtual void installImeFirmware() = 0;
		virtual void installM3dFirmware() = 0;
		virtual void installFirmwareFromFile() = 0;
		virtual void installDrivers() = 0;
};

#endif
