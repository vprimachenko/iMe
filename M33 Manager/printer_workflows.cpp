#include <fstream>
#include <iomanip>
#include <sstream>
#ifndef WINDOWS
	#include <unistd.h>
#endif
#include "common.h"
#include "printer_workflows.h"

PrinterWorkflows::PrinterWorkflows(Printer &newPrinter) :
	printer(newPrinter)
{
}

ThreadTaskResponse PrinterWorkflows::moveX(bool positive, double distanceMm, int feedRate, const function<void(const string &message)> &logFunction) {
	return executeCommandSequence({"G91", "G0 X" + string(positive ? "" : "-") + to_string(distanceMm) + " F" + to_string(printer.convertFeedRate(feedRate))}, logFunction);
}

ThreadTaskResponse PrinterWorkflows::moveY(bool positive, double distanceMm, int feedRate, const function<void(const string &message)> &logFunction) {
	return executeCommandSequence({"G91", "G0 Y" + string(positive ? "" : "-") + to_string(distanceMm) + " F" + to_string(printer.convertFeedRate(feedRate))}, logFunction);
}

ThreadTaskResponse PrinterWorkflows::moveZ(bool positive, double distanceMm, int feedRate, const function<void(const string &message)> &logFunction) {
	return executeCommandSequence({"G91", "G0 Z" + string(positive ? "" : "-") + to_string(distanceMm) + " F" + to_string(printer.convertFeedRate(feedRate))}, logFunction);
}

ThreadTaskResponse PrinterWorkflows::home(const function<void(const string &message)> &logFunction) {
	return executeCommandSequence({"G28"}, logFunction);
}

ThreadTaskResponse PrinterWorkflows::motorsOn(const function<void(const string &message)> &logFunction) {
	return executeCommandSequence({"M17"}, logFunction);
}

ThreadTaskResponse PrinterWorkflows::motorsOff(const function<void(const string &message)> &logFunction) {
	return executeCommandSequence({"M18"}, logFunction);
}

ThreadTaskResponse PrinterWorkflows::fanOn(const function<void(const string &message)> &logFunction) {
	return executeCommandSequence({"M106 S255"}, logFunction);
}

ThreadTaskResponse PrinterWorkflows::fanOff(const function<void(const string &message)> &logFunction) {
	return executeCommandSequence({"M107"}, logFunction);
}

ThreadTaskResponse PrinterWorkflows::calibrateBedPosition(const function<void(const string &message)> &logFunction) {
	return executeCommandSequence({"G91", "G0 Z3 F90", "G90", "M109 S150", "M104 S0", "M107", "G30"}, logFunction);
}

ThreadTaskResponse PrinterWorkflows::calibrateBedOrientation(const function<void(const string &message)> &logFunction) {
	return executeCommandSequence({"G90", "G0 Z3 F90", "M109 S150", "M104 S0", "M107", "G32"}, logFunction);
}

ThreadTaskResponse PrinterWorkflows::saveZAsZero(const function<void(const string &message)> &logFunction) {
	vector<string> sequence;
	if(printer.getFirmwareType() == M3D || printer.getFirmwareType() == M3D_MOD) {
		sequence.push_back("G91");
		sequence.push_back("G0 Z0.1 F90");
	}
	sequence.push_back("G33");
	return executeCommandSequence(sequence, logFunction);
}

vector<string> PrinterWorkflows::getPrinterSettingNames() const {
	return printer.getEepromSettingsNames();
}

string PrinterWorkflows::loadPrinterSettingValue(const string &settingName) {
	uint16_t offset;
	uint8_t length;
	EEPROM_TYPES type;
	printer.getEepromOffsetLengthAndType(settingName, offset, length, type);

	string value;
	if(type == EEPROM_INT)
		value = to_string(printer.eepromGetInt(offset, length));
	else if(type == EEPROM_FLOAT)
		value = to_string(printer.eepromGetFloat(offset, length));
	else if(type == EEPROM_STRING)
		value = printer.eepromGetString(offset, length);
	else if(type == EEPROM_BOOL)
		value = printer.eepromGetInt(offset, length) == 0 ? "False" : "True";

	if(type == EEPROM_FLOAT) {
		while(!value.empty() && value.back() == '0')
			value.pop_back();
		if(!value.empty() && value.back() == '.')
			value.pop_back();
	}

	return value;
}

ThreadTaskResponse PrinterWorkflows::savePrinterSetting(const string &settingName, const string &value) {
	uint16_t offset;
	uint8_t length;
	EEPROM_TYPES type;
	printer.getEepromOffsetLengthAndType(settingName, offset, length, type);

	bool error = false;
	if(type == EEPROM_INT)
		error = !printer.eepromWriteInt(offset, length, stoi(value));
	else if(type == EEPROM_FLOAT)
		error = !printer.eepromWriteFloat(offset, length, stof(value));
	else if(type == EEPROM_STRING)
		error = !printer.eepromWriteString(offset, length, value);
	else if(type == EEPROM_BOOL)
		error = !printer.eepromWriteInt(offset, length, toLowerCase(value) == "false" ? 0 : 1);

	if(!error)
		return {"Setting successfully saved", wxOK | wxICON_INFORMATION | wxCENTRE};
	return {"Failed to save setting", wxOK | wxICON_ERROR | wxCENTRE};
}

ManualCommandPlan PrinterWorkflows::prepareManualCommand(const string &command) const {
	ManualCommandPlan plan;
	plan.requiresModeTransitionHandling =
		(command == "Q" && printer.getOperatingMode() == BOOTLOADER) ||
		(command == "M115 S628" && printer.getOperatingMode() == FIRMWARE);
	return plan;
}

ThreadTaskResponse PrinterWorkflows::executeManualCommand(const string &command, const function<void(const string &message)> &logFunction) {
	return executeCommand(command, logFunction);
}

ThreadTaskResponse PrinterWorkflows::installImeFirmware() {
	string firmwareLocation = getTemporaryLocation() + "iMe " TOSTRING(IME_ROM_VERSION_STRING) ".hex";
	ofstream fout(firmwareLocation, ios::binary);
	if(fout.fail())
		return {"Failed to unpack iMe firmware", wxOK | wxICON_ERROR | wxCENTRE};

	for(uint64_t i = 0; i < IME_HEX_SIZE; i++)
		fout.put(IME_HEX_DATA[i]);
	fout.close();

	return installFirmwareFile(firmwareLocation);
}

ThreadTaskResponse PrinterWorkflows::installM3dFirmware() {
	string firmwareLocation = getTemporaryLocation() + "M3D " TOSTRING(M3D_ROM_VERSION_STRING) ".hex";
	ofstream fout(firmwareLocation, ios::binary);
	if(fout.fail())
		return {"Failed to unpack M3D firmware", wxOK | wxICON_ERROR | wxCENTRE};

	for(uint64_t i = 0; i < M3D_HEX_SIZE; i++)
		fout.put(M3D_HEX_DATA[i]);
	fout.close();

	return installFirmwareFile(firmwareLocation);
}

ThreadTaskResponse PrinterWorkflows::installFirmwareFromFile(const string &firmwareLocation) {
	return installFirmwareFile(firmwareLocation);
}

ThreadTaskResponse PrinterWorkflows::switchToMode(operatingModes newOperatingMode) {
	if(newOperatingMode == FIRMWARE)
		printer.switchToFirmwareMode();
	else
		printer.switchToBootloaderMode();

	if(!printer.isConnected())
		return {static_cast<string>("Failed to switch printer into ") + (newOperatingMode == FIRMWARE ? "firmware" : "bootloader") + " mode", wxOK | wxICON_ERROR | wxCENTRE};

	return {static_cast<string>("Printer has been successfully switched into ") + (newOperatingMode == FIRMWARE ? "firmware" : "bootloader") + " mode and is connected at " + printer.getCurrentSerialPort() + " running at a baud rate of " TOSTRING(PRINTER_BAUD_RATE), wxOK | wxICON_INFORMATION | wxCENTRE};
}

ThreadTaskResponse PrinterWorkflows::installDrivers() {
	#ifdef WINDOWS
		ofstream fout(getTemporaryLocation() + "M3D_v2.cat", ios::binary);
		if(fout.fail())
			return {"Failed to unpack drivers", wxOK | wxICON_ERROR | wxCENTRE};
		for(uint64_t i = 0; i < m3DV2CatSize; i++)
			fout.put(m3DV2CatData[i]);
		fout.close();

		fout.open(getTemporaryLocation() + "M3D_v2.inf", ios::binary);
		if(fout.fail())
			return {"Failed to unpack drivers", wxOK | wxICON_ERROR | wxCENTRE};
		for(uint64_t i = 0; i < m3DV2InfSize; i++)
			fout.put(m3DV2InfData[i]);
		fout.close();

		TCHAR buffer[MAX_PATH];
		GetWindowsDirectory(buffer, MAX_PATH);
		wstring path = buffer;

		string executablePath;
		ifstream file(path + "\\sysnative\\pnputil.exe", ios::binary);
		executablePath = file.good() ? "sysnative" : "System32";
		file.close();

		STARTUPINFO startupInfo;
		SecureZeroMemory(&startupInfo, sizeof(startupInfo));
		startupInfo.cb = sizeof(startupInfo);

		PROCESS_INFORMATION processInfo;
		SecureZeroMemory(&processInfo, sizeof(processInfo));

		TCHAR command[MAX_PATH];
		_tcscpy(command, (path + "\\" + executablePath + "\\pnputil.exe -i -a \"" + getTemporaryLocation() + "M3D_v2.inf\"").c_str());

		if(!CreateProcess(nullptr, command, nullptr, nullptr, false, CREATE_NO_WINDOW, nullptr, nullptr, &startupInfo, &processInfo))
			return {"Failed to install drivers", wxOK | wxICON_ERROR | wxCENTRE};

		WaitForSingleObject(processInfo.hProcess, INFINITE);

		DWORD exitCode;
		GetExitCodeProcess(processInfo.hProcess, &exitCode);
		CloseHandle(processInfo.hProcess);
		CloseHandle(processInfo.hThread);

		if(!exitCode)
			return {"Failed to install drivers", wxOK | wxICON_ERROR | wxCENTRE};

		return {"Drivers successfully installed. You might need to reconnect the printer to the computer for the drivers to take effect.", wxOK | wxICON_INFORMATION | wxCENTRE};
	#endif

	#ifdef LINUX
		if(getuid())
			return {"Elevated privileges required", wxOK | wxICON_ERROR | wxCENTRE};

		ofstream fout("/etc/udev/rules.d/90-micro-3d-local.rules", ios::binary);
		if(fout.fail())
			return {"Failed to unpack udev rule", wxOK | wxICON_ERROR | wxCENTRE};
		for(uint64_t i = 0; i < _90Micro3dLocalRulesSize; i++)
			fout.put(_90Micro3dLocalRulesData[i]);
		fout.close();

		fout.open("/etc/udev/rules.d/91-micro-3d-heatbed-local.rules", ios::binary);
		if(fout.fail())
			return {"Failed to unpack udev rule", wxOK | wxICON_ERROR | wxCENTRE};
		for(uint64_t i = 0; i < _91Micro3dHeatbedLocalRulesSize; i++)
			fout.put(_91Micro3dHeatbedLocalRulesData[i]);
		fout.close();

		fout.open("/etc/udev/rules.d/92-m3d-pro-local.rules", ios::binary);
		if(fout.fail())
			return {"Failed to unpack udev rule", wxOK | wxICON_ERROR | wxCENTRE};
		for(uint64_t i = 0; i < _92M3dProLocalRulesSize; i++)
			fout.put(_92M3dProLocalRulesData[i]);
		fout.close();

		fout.open("/etc/udev/rules.d/93-micro+-local.rules", ios::binary);
		if(fout.fail())
			return {"Failed to unpack udev rule", wxOK | wxICON_ERROR | wxCENTRE};
		for(uint64_t i = 0; i < _93Micro_localRulesSize; i++)
			fout.put(_93Micro_localRulesData[i]);
		fout.close();

		if(system("udevadm control --reload-rules") || system("udevadm trigger"))
			return {"Failed to apply udev rule", wxOK | wxICON_ERROR | wxCENTRE};

		return {"Drivers successfully installed. You might need to reconnect the printer to the computer for the drivers to take effect.", wxOK | wxICON_INFORMATION | wxCENTRE};
	#endif

	return {"", 0};
}

bool PrinterWorkflows::ensureMode(operatingModes targetMode) {
	if(printer.getOperatingMode() == targetMode)
		return printer.isConnected();

	if(targetMode == FIRMWARE)
		printer.switchToFirmwareMode();
	else
		printer.switchToBootloaderMode();

	return printer.isConnected() && printer.getOperatingMode() == targetMode;
}

InvalidValuesReport PrinterWorkflows::inspectInvalidValues() {
	InvalidValuesReport report;
	printer.collectPrinterInformation(false);
	report.hasInvalidFirmware = !printer.hasValidFirmware();
	report.hasInvalidBedPosition = !printer.hasValidBedPosition();
	report.hasInvalidBedOrientation = !printer.hasValidBedOrientation();

	if(printer.getFirmwareType() == M3D || printer.getFirmwareType() == M3D_MOD)
		report.firmwareAction = FIRMWARE_REMEDIATION_INSTALL_M3D;
	else
		report.firmwareAction = FIRMWARE_REMEDIATION_INSTALL_IME;

	if(!report.hasInvalidFirmware) {
		report.firmwareUpdateAvailable =
			printer.getFirmwareType() == UNKNOWN_FIRMWARE ||
			((printer.getFirmwareType() == M3D || printer.getFirmwareType() == M3D_MOD) && stoi(printer.getFirmwareRelease()) < M3D_ROM_VERSION_STRING) ||
			(printer.getFirmwareType() == IME && printer.getFirmwareVersion() < IME_ROM_VERSION_STRING);

		if((printer.getFirmwareType() == M3D || printer.getFirmwareType() == M3D_MOD) && stoi(printer.getFirmwareRelease()) >= 2015122112)
			report.firmwareIsIncompatible = false;
		else if(printer.getFirmwareType() == IME && printer.getFirmwareVersion() >= 1900000118)
			report.firmwareIsIncompatible = false;
		else if(report.firmwareUpdateAvailable)
			report.firmwareIsIncompatible = true;
	}

	return report;
}

string PrinterWorkflows::getFirmwareDisplayName(FirmwareRemediationAction action) const {
	if(action == FIRMWARE_REMEDIATION_INSTALL_M3D)
		return "M3D V" TOSTRING(M3D_ROM_VERSION_STRING);

	string iMeVersion = static_cast<string>(TOSTRING(IME_ROM_VERSION_STRING)).substr(2);
	for(uint8_t i = 0; i < 3; i++)
		iMeVersion.insert(i * 2 + 2 + i, ".");
	return "iMe V" + iMeVersion;
}

ThreadTaskResponse PrinterWorkflows::executeCommand(const string &command, const function<void(const string &message)> &logFunction) {
	if(logFunction)
		logFunction("Send: " + command);

	bool changedMode = (command == "Q" && printer.getOperatingMode() == BOOTLOADER) || (command == "M115 S628" && printer.getOperatingMode() == FIRMWARE);
	if(!printer.sendRequest(command)) {
		if(logFunction)
			logFunction("Sending command failed");
		return {"", 0};
	}

	if(changedMode)
		return {"", 0};

	string response;
	do {
		do {
			response = printer.receiveResponse();
		} while(response.empty() && printer.isConnected());

		if(!printer.isConnected())
			break;

		string responseForLog = formatResponseForLogging(response);
		if(responseForLog != "wait" && responseForLog != "ait" && logFunction)
			logFunction("Receive: " + responseForLog);

		if(printer.getOperatingMode() == BOOTLOADER)
			break;
	} while(response.rfind("ok", 0) != 0 && response[0] != 'k' && response.rfind("rs", 0) != 0 && response.rfind("skip", 0) != 0 && response.rfind("Error", 0) != 0);

	return {"", 0};
}

ThreadTaskResponse PrinterWorkflows::executeCommandSequence(const vector<string> &commands, const function<void(const string &message)> &logFunction) {
	ThreadTaskResponse response = {"", 0};
	for(size_t i = 0; i < commands.size(); i++)
		response = executeCommand(commands[i], logFunction);
	return response;
}

string PrinterWorkflows::formatResponseForLogging(const string &response) const {
	if(printer.getOperatingMode() != BOOTLOADER)
		return response;

	stringstream hexResponse;
	for(size_t i = 0; i < response.length(); i++)
		hexResponse << "0x" << hex << setfill('0') << setw(2) << uppercase << (static_cast<uint8_t>(response[i]) & 0xFF) << ' ';

	string formattedResponse = hexResponse.str();
	if(!formattedResponse.empty())
		formattedResponse.pop_back();
	return formattedResponse;
}

ThreadTaskResponse PrinterWorkflows::installFirmwareFile(const string &firmwareLocation) {
	ifstream file(firmwareLocation, ios::binary);
	if(!file.good())
		return {"Firmware ROM doesn't exist", wxOK | wxICON_ERROR | wxCENTRE};

	uint8_t endOfNumbers = 0;
	if(firmwareLocation.find_last_of('.') != string::npos)
		endOfNumbers = firmwareLocation.find_last_of('.') - 1;
	else
		endOfNumbers = firmwareLocation.length() - 1;

	int8_t beginningOfNumbers = endOfNumbers;
	for(; beginningOfNumbers >= 0 && isdigit(firmwareLocation[beginningOfNumbers]); beginningOfNumbers--);

	if(beginningOfNumbers != endOfNumbers - 10)
		return {"Invalid firmware name", wxOK | wxICON_ERROR | wxCENTRE};

	if(!printer.installFirmware(firmwareLocation.c_str()))
		return {"Failed to update firmware", wxOK | wxICON_ERROR | wxCENTRE};

	return {"Firmware successfully installed", wxOK | wxICON_INFORMATION | wxCENTRE};
}
