#ifndef GUI_HOST_H
#define GUI_HOST_H

#include <cstddef>
#include <functional>
#include <string>
#include <vector>
#include "gcode.h"
#include "printer.h"

using namespace std;

struct ThreadTaskResponse {
	string message;
	int style;
};

struct PrintPreviewSegment {
	float startX = 0.0f;
	float startY = 0.0f;
	float startZ = 0.0f;
	float endX = 0.0f;
	float endY = 0.0f;
	float endZ = 0.0f;
	size_t commandIndex = 0;
	bool extruding = false;
};

enum PrintJobState {
	PRINT_JOB_IDLE,
	PRINT_JOB_LOADED,
	PRINT_JOB_RUNNING,
	PRINT_JOB_PAUSED,
	PRINT_JOB_STOPPING,
	PRINT_JOB_COMPLETED,
	PRINT_JOB_FAILED
};

struct PreparedPrintJob {
	string filePath;
	vector<string> printableCommands;
	vector<Gcode> parsedCommands;
	vector<string> statusMessages;
	vector<PrintPreviewSegment> previewSegments;
	float minX = 0.0f;
	float minY = 0.0f;
	float minZ = 0.0f;
	float maxX = 0.0f;
	float maxY = 0.0f;
	float maxZ = 0.0f;
	size_t totalLineCount = 0;
	size_t acceptedCommandCount = 0;
	string summaryText;
	string errorText;
	size_t errorLineNumber = 0;
	bool valid = false;
};

struct PrintJobStatus {
	PrintJobState state = PRINT_JOB_IDLE;
	string filePath;
	size_t totalCommands = 0;
	size_t currentCommandIndex = 0;
	string summaryText;
	string statusText;
	string errorText;
};

class GuiHost {
	public:
		virtual ~GuiHost() {}
		virtual int getPauseStandbyTemperatureC() const = 0;
		virtual void setPauseStandbyTemperatureC(int temperatureC) = 0;
		virtual int getManualNozzleTemperatureC() const = 0;
		virtual void setManualNozzleTemperatureC(int temperatureC) = 0;
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
		virtual void runHeatNozzle() = 0;
		virtual void runCoolDownNozzle() = 0;
			virtual void runCalibrateBedPosition() = 0;
			virtual void runCalibrateBedOrientation() = 0;
			virtual void runSaveCurrentPositionAsHome() = 0;
			virtual void runSaveZAsZero() = 0;
		virtual void savePrinterSetting(const string &settingName, const string &value) = 0;
		virtual void installImeFirmware() = 0;
		virtual void installM3dFirmware() = 0;
		virtual void installFirmwareFromFile() = 0;
		virtual void installDrivers() = 0;
		virtual void loadPrintFile() = 0;
		virtual void startPrintJob() = 0;
		virtual void pausePrintJob() = 0;
		virtual void resumePrintJob() = 0;
		virtual void stopPrintJob() = 0;
		virtual PrintJobStatus getPrintJobStatus() const = 0;
		virtual const PreparedPrintJob &getPreparedPrintJob() const = 0;
};

#endif
