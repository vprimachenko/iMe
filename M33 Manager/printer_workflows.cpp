#include <fstream>
#include <iomanip>
#include <sstream>
#include <cctype>
#include <cmath>
#ifndef WINDOWS
	#include <unistd.h>
#endif
#include "../eeprom.h"
#include "common.h"
#include "printer_workflows.h"

PrinterWorkflows::PrinterWorkflows(Printer &newPrinter) :
	printer(newPrinter)
{
}

namespace {

string trimWhitespace(const string &value) {
	size_t start = value.find_first_not_of(" \t\r\n");
	if(start == string::npos)
		return "";
	size_t end = value.find_last_not_of(" \t\r\n");
	return value.substr(start, end - start + 1);
}

string stripGcodeComment(const string &line) {
	size_t commentOffset = line.find(';');
	size_t checksumOffset = line.find('*');
	size_t endOffset = string::npos;
	if(commentOffset != string::npos && checksumOffset != string::npos)
		endOffset = min(commentOffset, checksumOffset);
	else if(commentOffset != string::npos)
		endOffset = commentOffset;
	else
		endOffset = checksumOffset;

	string strippedLine = endOffset == string::npos ? line : line.substr(0, endOffset);
	return trimWhitespace(strippedLine);
}

string extractStatusDirective(const string &line) {
	size_t commentOffset = line.find(';');
	if(commentOffset == string::npos)
		return "";

	string commentText = trimWhitespace(line.substr(commentOffset + 1));
	string commentTextLower = toLowerCase(commentText);

	const string prefixes[] = {
		"@m33 status:",
		"@m33 status ",
		"m33 status:",
		"m33 status "
	};

	for(const string &prefix : prefixes) {
		if(commentTextLower.rfind(prefix, 0) == 0) {
			string statusText = trimWhitespace(commentText.substr(prefix.size()));
			if(!statusText.empty())
				return statusText;
		}
	}

	return "";
}

void updatePreviewBounds(PreparedPrintJob &job, float x, float y, float z, bool &hasBounds) {
	if(!hasBounds) {
		job.minX = job.maxX = x;
		job.minY = job.maxY = y;
		job.minZ = job.maxZ = z;
		hasBounds = true;
		return;
	}

	job.minX = min(job.minX, x);
	job.minY = min(job.minY, y);
	job.minZ = min(job.minZ, z);
	job.maxX = max(job.maxX, x);
	job.maxY = max(job.maxY, y);
	job.maxZ = max(job.maxZ, z);
}

float parseFloatOrDefault(const Gcode &gcode, char parameter, float fallback) {
	return gcode.hasValue(parameter) ? stof(gcode.getValue(parameter)) : fallback;
}

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

ThreadTaskResponse PrinterWorkflows::setManualNozzleTemperature(int temperatureC, const function<void(const string &message)> &logFunction) {
	return executeCommandSequence({"M104 S" + to_string(temperatureC)}, logFunction);
}

ThreadTaskResponse PrinterWorkflows::setNozzleStandbyTemperature(int temperatureC, const function<void(const string &message)> &logFunction) {
	return executeCommandSequence({"M104 S" + to_string(temperatureC)}, logFunction);
}

ThreadTaskResponse PrinterWorkflows::reheatNozzleForResume(int targetTemperatureC, const function<void(const string &message)> &logFunction) {
	return executeCommandSequence({"M109 S" + to_string(targetTemperatureC)}, logFunction);
}

ThreadTaskResponse PrinterWorkflows::coolDownNozzle(const function<void(const string &message)> &logFunction) {
	return executeCommandSequence({"M104 S0"}, logFunction);
}

ThreadTaskResponse PrinterWorkflows::calibrateBedPosition(const function<void(const string &message)> &logFunction) {
	return executeCommandSequence({"G91", "G0 Z3 F90", "G90", "M109 S150", "M104 S0", "M107", "G30"}, logFunction);
}

ThreadTaskResponse PrinterWorkflows::calibrateBedOrientation(const function<void(const string &message)> &logFunction) {
	return executeCommandSequence({"G90", "G0 Z3 F90", "M109 S150", "M104 S0", "M107", "G32"}, logFunction);
}

ThreadTaskResponse PrinterWorkflows::saveCurrentPositionAsHome(const function<void(const string &message)> &logFunction) {
	ThreadTaskResponse response = executeCommandSequence({"G92 X0 Y0"}, logFunction);
	if(response.style & wxICON_ERROR)
		return response;

	if(!printer.eepromWriteFloat(EEPROM_LAST_RECORDED_X_VALUE_OFFSET, EEPROM_LAST_RECORDED_X_VALUE_LENGTH, 0.0f) ||
		!printer.eepromWriteFloat(EEPROM_LAST_RECORDED_Y_VALUE_OFFSET, EEPROM_LAST_RECORDED_Y_VALUE_LENGTH, 0.0f) ||
		!printer.eepromWriteInt(EEPROM_SAVED_X_STATE_OFFSET, EEPROM_SAVED_X_STATE_LENGTH, 1) ||
		!printer.eepromWriteInt(EEPROM_SAVED_Y_STATE_OFFSET, EEPROM_SAVED_Y_STATE_LENGTH, 1))
		return {"Failed to save the current X/Y position as home", wxOK | wxICON_ERROR | wxCENTRE};

	if(!printer.collectPrinterInformation(false))
		return {"Saved the current X/Y position as home, but failed to refresh printer information", wxOK | wxICON_WARNING | wxCENTRE};

	return {"Current manual X/Y position saved as home. The printer will now treat this location as the homing corner.", wxOK | wxICON_INFORMATION | wxCENTRE};
}

ThreadTaskResponse PrinterWorkflows::saveZAsZero(const function<void(const string &message)> &logFunction) {
	ThreadTaskResponse response = executeCommandSequence({"G33"}, logFunction);
	if(response.style & wxICON_ERROR)
		return response;

	return {"Current manual Z position saved as Z0. The printer will now treat this height as the bed reference.", wxOK | wxICON_INFORMATION | wxCENTRE};
}

PreparedPrintJob PrinterWorkflows::preparePrintJob(const string &filePath) {
	PreparedPrintJob job;
	job.filePath = filePath;

	ifstream file(filePath);
	if(!file.good()) {
		job.errorText = "Failed to open G-code file";
		return job;
	}

	string line;
	size_t lineNumber = 0;
	float currentX = 0.0f;
	float currentY = 0.0f;
	float currentZ = 0.0f;
	float currentE = 0.0f;
	bool absolutePositioning = true;
	bool absoluteExtrusion = true;
	bool hasBounds = false;
	size_t skippedBedCommands = 0;
	size_t skippedNoOpCommands = 0;
	size_t normalizedCommands = 0;
	size_t customStatusDirectives = 0;
	string pendingStatusMessage;
	while(getline(file, line)) {
		lineNumber++;
		job.totalLineCount++;

		string statusDirective = extractStatusDirective(line);
		if(!statusDirective.empty()) {
			pendingStatusMessage = statusDirective;
			customStatusDirectives++;
		}

		string sanitizedLine = stripGcodeComment(line);
		if(sanitizedLine.empty())
			continue;

		Gcode gcode;
		if(!gcode.parseLine(line)) {
			job.errorLineNumber = lineNumber;
			job.errorText = "Failed to parse G-code on line " + to_string(lineNumber);
			return job;
		}

		if(gcode.isHostCommand())
			continue;

		string printableCommand = trimWhitespace(gcode.getOriginalCommand());
		if(printableCommand.empty())
			continue;

		if(gcode.hasValue('G')) {
			const string gValue = gcode.getValue('G');
			if(gValue == "20") {
				job.errorLineNumber = lineNumber;
				job.errorText = "Inch-based G-code (G20) is not supported for printing";
				return job;
			}
			if(gValue == "21") {
				skippedNoOpCommands++;
				continue;
			}
			if(gValue == "28" && (gcode.hasParameter('X') || gcode.hasParameter('Y') || gcode.hasParameter('Z'))) {
				printableCommand = "G28";
				normalizedCommands++;
			}
		}

		if(gcode.hasValue('M')) {
			const string mValue = gcode.getValue('M');
			if(mValue == "140" || mValue == "190") {
				skippedBedCommands++;
				continue;
			}
		}

		if(gcode.hasValue('T') && !gcode.hasValue('G') && !gcode.hasValue('M')) {
			if(gcode.getValue('T') == "0") {
				skippedNoOpCommands++;
				continue;
			}

			job.errorLineNumber = lineNumber;
			job.errorText = "Unsupported tool selection on line " + to_string(lineNumber);
			return job;
		}

		job.printableCommands.push_back(printableCommand);
		job.parsedCommands.push_back(gcode);
		job.statusMessages.push_back(pendingStatusMessage);
		pendingStatusMessage.clear();
		job.acceptedCommandCount++;

		if(gcode.hasValue('G')) {
			string gValue = gcode.getValue('G');
			if(gValue == "90")
				absolutePositioning = true;
			else if(gValue == "91")
				absolutePositioning = false;
			else if(gValue == "92") {
				if(gcode.hasValue('X'))
					currentX = stof(gcode.getValue('X'));
				if(gcode.hasValue('Y'))
					currentY = stof(gcode.getValue('Y'));
				if(gcode.hasValue('Z'))
					currentZ = stof(gcode.getValue('Z'));
				if(gcode.hasValue('E'))
					currentE = stof(gcode.getValue('E'));
			}
			else if(gValue == "0" || gValue == "1") {
				float nextX = currentX;
				float nextY = currentY;
				float nextZ = currentZ;
				float nextE = currentE;

				if(gcode.hasValue('X')) {
					float x = stof(gcode.getValue('X'));
					nextX = absolutePositioning ? x : currentX + x;
				}
				if(gcode.hasValue('Y')) {
					float y = stof(gcode.getValue('Y'));
					nextY = absolutePositioning ? y : currentY + y;
				}
				if(gcode.hasValue('Z')) {
					float z = stof(gcode.getValue('Z'));
					nextZ = absolutePositioning ? z : currentZ + z;
				}
				if(gcode.hasValue('E')) {
					float e = stof(gcode.getValue('E'));
					nextE = absoluteExtrusion ? e : currentE + e;
				}

				const bool moved = nextX != currentX || nextY != currentY || nextZ != currentZ;
				if(moved) {
					PrintPreviewSegment segment;
					segment.startX = currentX;
					segment.startY = currentY;
					segment.startZ = currentZ;
					segment.endX = nextX;
					segment.endY = nextY;
					segment.endZ = nextZ;
					segment.commandIndex = job.acceptedCommandCount - 1;
					segment.extruding = nextE > currentE;
					job.previewSegments.push_back(segment);
					updatePreviewBounds(job, currentX, currentY, currentZ, hasBounds);
					updatePreviewBounds(job, nextX, nextY, nextZ, hasBounds);
				}

				currentX = nextX;
				currentY = nextY;
				currentZ = nextZ;
				currentE = nextE;
			}
			else if(gValue == "2" || gValue == "3") {
				float endX = currentX;
				float endY = currentY;
				float endZ = currentZ;
				float endE = currentE;

				if(gcode.hasValue('X')) {
					float x = stof(gcode.getValue('X'));
					endX = absolutePositioning ? x : currentX + x;
				}
				if(gcode.hasValue('Y')) {
					float y = stof(gcode.getValue('Y'));
					endY = absolutePositioning ? y : currentY + y;
				}
				if(gcode.hasValue('Z')) {
					float z = stof(gcode.getValue('Z'));
					endZ = absolutePositioning ? z : currentZ + z;
				}
				if(gcode.hasValue('E')) {
					float e = stof(gcode.getValue('E'));
					endE = absoluteExtrusion ? e : currentE + e;
				}

				const bool clockwise = gValue == "2";
				const bool extruding = endE > currentE;
				const bool hasIJ = gcode.hasValue('I') || gcode.hasValue('J');
				if(hasIJ) {
					const float centerX = currentX + parseFloatOrDefault(gcode, 'I', 0.0f);
					const float centerY = currentY + parseFloatOrDefault(gcode, 'J', 0.0f);
					const float radius = sqrtf((currentX - centerX) * (currentX - centerX) + (currentY - centerY) * (currentY - centerY));
					float startAngle = atan2f(currentY - centerY, currentX - centerX);
					float endAngle = atan2f(endY - centerY, endX - centerX);
					float angleDelta = endAngle - startAngle;
					if(clockwise && angleDelta >= 0.0f)
						angleDelta -= 2.0f * static_cast<float>(M_PI);
					else if(!clockwise && angleDelta <= 0.0f)
						angleDelta += 2.0f * static_cast<float>(M_PI);

					const int steps = max(12, static_cast<int>(fabsf(angleDelta) / (static_cast<float>(M_PI) / 18.0f)));
					float previousX = currentX;
					float previousY = currentY;
					float previousZ = currentZ;
					for(int step = 1; step <= steps; step++) {
						const float t = static_cast<float>(step) / steps;
						const float angle = startAngle + angleDelta * t;
						const float arcX = centerX + cosf(angle) * radius;
						const float arcY = centerY + sinf(angle) * radius;
						const float arcZ = currentZ + (endZ - currentZ) * t;
						PrintPreviewSegment segment;
						segment.startX = previousX;
						segment.startY = previousY;
						segment.startZ = previousZ;
						segment.endX = arcX;
						segment.endY = arcY;
						segment.endZ = arcZ;
						segment.commandIndex = job.acceptedCommandCount - 1;
						segment.extruding = extruding;
						job.previewSegments.push_back(segment);
						updatePreviewBounds(job, previousX, previousY, previousZ, hasBounds);
						updatePreviewBounds(job, arcX, arcY, arcZ, hasBounds);
						previousX = arcX;
						previousY = arcY;
						previousZ = arcZ;
					}
				}
				else {
					PrintPreviewSegment segment;
					segment.startX = currentX;
					segment.startY = currentY;
					segment.startZ = currentZ;
					segment.endX = endX;
					segment.endY = endY;
					segment.endZ = endZ;
					segment.commandIndex = job.acceptedCommandCount - 1;
					segment.extruding = extruding;
					job.previewSegments.push_back(segment);
					updatePreviewBounds(job, currentX, currentY, currentZ, hasBounds);
					updatePreviewBounds(job, endX, endY, endZ, hasBounds);
				}

				currentX = endX;
				currentY = endY;
				currentZ = endZ;
				currentE = endE;
			}
		}

		if(gcode.hasValue('M')) {
			string mValue = gcode.getValue('M');
			if(mValue == "82")
				absoluteExtrusion = true;
			else if(mValue == "83")
				absoluteExtrusion = false;
		}
	}

	if(job.acceptedCommandCount == 0) {
		job.errorText = "No printable G-code commands found in file";
		return job;
	}

	job.valid = true;
	job.summaryText = "Loaded " + to_string(job.acceptedCommandCount) + " printable commands from " + to_string(job.totalLineCount) + " lines";
	if(skippedBedCommands || skippedNoOpCommands || normalizedCommands || customStatusDirectives) {
		job.summaryText += " (";
		bool wroteSegment = false;
		if(skippedBedCommands) {
			job.summaryText += "skipped " + to_string(skippedBedCommands) + " bed command";
			if(skippedBedCommands != 1)
				job.summaryText += "s";
			wroteSegment = true;
		}
		if(skippedNoOpCommands) {
			if(wroteSegment)
				job.summaryText += ", ";
			job.summaryText += "skipped " + to_string(skippedNoOpCommands) + " no-op command";
			if(skippedNoOpCommands != 1)
				job.summaryText += "s";
			wroteSegment = true;
		}
		if(normalizedCommands) {
			if(wroteSegment)
				job.summaryText += ", ";
			job.summaryText += "normalized " + to_string(normalizedCommands) + " homing command";
			if(normalizedCommands != 1)
				job.summaryText += "s";
			wroteSegment = true;
		}
		if(customStatusDirectives) {
			if(wroteSegment)
				job.summaryText += ", ";
			job.summaryText += "found " + to_string(customStatusDirectives) + " custom status directive";
			if(customStatusDirectives != 1)
				job.summaryText += "s";
		}
		job.summaryText += ")";
	}
	return job;
}

ThreadTaskResponse PrinterWorkflows::executePreparedPrintCommand(const string &command, const function<void(const string &message)> &logFunction) {
	return executeCommand(command, logFunction);
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

		if(exitCode)
			return {"Failed to install drivers", wxOK | wxICON_ERROR | wxCENTRE};

		return {"Driver installation was started successfully. Windows may still show a trust prompt before the driver is fully installed. You might need to reconnect the printer to the computer afterward.", wxOK | wxICON_INFORMATION | wxCENTRE};
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
