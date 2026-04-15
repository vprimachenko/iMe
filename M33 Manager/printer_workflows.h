#ifndef PRINTER_WORKFLOWS_H
#define PRINTER_WORKFLOWS_H

#include <functional>
#include <string>
#include <vector>
#include "printer.h"
#include "gui_host.h"

using namespace std;

enum FirmwareRemediationAction {
	FIRMWARE_REMEDIATION_NONE,
	FIRMWARE_REMEDIATION_INSTALL_IME,
	FIRMWARE_REMEDIATION_INSTALL_M3D
};

struct ManualCommandPlan {
	bool requiresModeTransitionHandling = false;
};

struct InvalidValuesReport {
	bool hasInvalidFirmware = false;
	bool hasInvalidBedPosition = false;
	bool hasInvalidBedOrientation = false;
	bool firmwareIsIncompatible = false;
	bool firmwareUpdateAvailable = false;
	FirmwareRemediationAction firmwareAction = FIRMWARE_REMEDIATION_NONE;
};

class PrinterWorkflows {
	public:
		explicit PrinterWorkflows(Printer &printer);

		ThreadTaskResponse moveX(bool positive, double distanceMm, int feedRate, const function<void(const string &message)> &logFunction);
		ThreadTaskResponse moveY(bool positive, double distanceMm, int feedRate, const function<void(const string &message)> &logFunction);
		ThreadTaskResponse moveZ(bool positive, double distanceMm, int feedRate, const function<void(const string &message)> &logFunction);
		ThreadTaskResponse home(const function<void(const string &message)> &logFunction);
		ThreadTaskResponse motorsOn(const function<void(const string &message)> &logFunction);
		ThreadTaskResponse motorsOff(const function<void(const string &message)> &logFunction);
		ThreadTaskResponse fanOn(const function<void(const string &message)> &logFunction);
		ThreadTaskResponse fanOff(const function<void(const string &message)> &logFunction);
			ThreadTaskResponse calibrateBedPosition(const function<void(const string &message)> &logFunction);
			ThreadTaskResponse calibrateBedOrientation(const function<void(const string &message)> &logFunction);
			ThreadTaskResponse saveCurrentPositionAsHome(const function<void(const string &message)> &logFunction);
			ThreadTaskResponse saveZAsZero(const function<void(const string &message)> &logFunction);
		PreparedPrintJob preparePrintJob(const string &filePath);
		ThreadTaskResponse executePreparedPrintCommand(const string &command, const function<void(const string &message)> &logFunction);

		vector<string> getPrinterSettingNames() const;
		string loadPrinterSettingValue(const string &settingName);
		ThreadTaskResponse savePrinterSetting(const string &settingName, const string &value);

		ManualCommandPlan prepareManualCommand(const string &command) const;
		ThreadTaskResponse executeManualCommand(const string &command, const function<void(const string &message)> &logFunction);

		ThreadTaskResponse installImeFirmware();
		ThreadTaskResponse installM3dFirmware();
		ThreadTaskResponse installFirmwareFromFile(const string &firmwareLocation);
		ThreadTaskResponse switchToMode(operatingModes newOperatingMode);
		ThreadTaskResponse installDrivers();
		bool ensureMode(operatingModes targetMode);

		InvalidValuesReport inspectInvalidValues();
		string getFirmwareDisplayName(FirmwareRemediationAction action) const;

	private:
		ThreadTaskResponse executeCommand(const string &command, const function<void(const string &message)> &logFunction);
		ThreadTaskResponse executeCommandSequence(const vector<string> &commands, const function<void(const string &message)> &logFunction);
		ThreadTaskResponse installFirmwareFile(const string &firmwareLocation);
		string formatResponseForLogging(const string &response) const;
		Printer &printer;
};

#endif
