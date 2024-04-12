// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Abstract FE class
// # Comment: Combined multiple FE 
// ################################

#include "FrontEnd.h"

bool FrontEnd::isActive() const {
	return active;
}

bool FrontEnd::getActive() const {
	return this->active;
}

void FrontEnd::setActive(bool arg_active) {
	active = arg_active;
}

std::tuple<json, std::vector<json>> FrontEndCfg::getPreset(const std::string& systemType) {
	throw std::runtime_error("No presets defined");
}

void FrontEnd::startClipboardMonitors(std::string arg_feName, unsigned arg_monitorCycleTime) {
	clipRawData.startMonitor(arg_feName + "_raw_data", arg_monitorCycleTime);
	clipData.startMonitor(arg_feName + "_event_data", arg_monitorCycleTime);
	clipHisto.startMonitor(arg_feName + "_histogrammer", arg_monitorCycleTime);
}

void FrontEnd::joinClipboardMonitors() {
	clipRawData.joinMonitor();
	clipData.joinMonitor();
	clipHisto.joinMonitor();
}
