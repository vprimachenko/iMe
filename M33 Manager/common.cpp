// Header files
#include <algorithm>
#include "common.h"


// Supporting function implementation
string getTemporaryLocation() {

	// Return temp location
	char* path = getenv("TEMP");
	if(!path)
		 path = getenv("TMP");
	if(!path)
		path = getenv("TMPDIR");
	
	string tempPath = path ? path : P_tmpdir;
	
	// Make sure path ends with a path seperator
	char pathSeperator =
	#ifdef WINDOWS
		'\\'
	#else
		'/'
	#endif
	;
	
	if(tempPath.back() != pathSeperator)
		tempPath.push_back(pathSeperator);
	
	// Return temp path
	return tempPath;
}

string toLowerCase(const string &text) {

	// Initialize return value
	string returnValue = text;
	
	// Convert return value to lower case
	transform(returnValue.begin(), returnValue.end(), returnValue.begin(), ::tolower);
	
	// Return return value
	return returnValue;
}

string getTerminalLogLabel(const string &message) {
	if(message == "Remove last line")
		return "console";

	if(message.rfind("Send:", 0) == 0 || message.rfind("Receive:", 0) == 0)
		return "io";

	if(message.rfind("Autodetecting serial port", 0) == 0 ||
		message.rfind("Printer found at ", 0) == 0 ||
		message.rfind("Connecting to ", 0) == 0 ||
		message.rfind("Connected to ", 0) == 0 ||
		message.rfind("Reconnecting to ", 0) == 0 ||
		message.rfind("Reconnected to printer", 0) == 0 ||
		message.rfind("Switching printer into ", 0) == 0 ||
		message.rfind("Printer is in ", 0) == 0 ||
		message.rfind("Printer has been disconnected", 0) == 0)
		return "conn";

	if(message.rfind("Loaded ", 0) == 0 ||
		message.rfind("Ready to print", 0) == 0 ||
		message.rfind("Printing", 0) == 0 ||
		message.rfind("Paused", 0) == 0 ||
		message.rfind("Stopping", 0) == 0 ||
		message.rfind("Print completed", 0) == 0 ||
		message.rfind("Print stopped", 0) == 0)
		return "job";

	if(message.rfind("Using ", 0) == 0 ||
		message.rfind("Firmware is ", 0) == 0 ||
		message.rfind("Bed position is ", 0) == 0 ||
		message.rfind("Bed orientation is ", 0) == 0 ||
		message.rfind("Done checking printer's invalid values", 0) == 0)
		return "diag";

	if(message.find("failed") != string::npos ||
		message.find("Failed") != string::npos ||
		message.find("doesn't exist") != string::npos ||
		message.find("Invalid ") != string::npos ||
		message.find("disconnected") != string::npos ||
		message.find("Disconnected") != string::npos ||
		message.find("unsupported") != string::npos ||
		message.find("Unsupported") != string::npos)
		return "error";

	return "core";
}
