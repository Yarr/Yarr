// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Abstract FE class
// # Comment: Combined multiple FE 
// ################################

#include "FrontEnd.h"

bool FrontEnd::isActive() {
	return active;
}

bool FrontEnd::getActive() {
	return this->active;
}

void FrontEnd::setActive(bool arg_active) {
	active = arg_active;
}

std::tuple<json, std::vector<json>> FrontEndCfg::getPreset(const std::string& systemType) {
	// Return a json object for connectivity configuration and a vector json objects for chip configurations

	std::tuple<json, std::vector<json>> preset;
	auto& [connectivity, chips] = preset;

	if (systemType != "SingleChip") {
		throw std::runtime_error("Unknown system type: "+systemType);
	}

	// Add a front end config
	json cfg;
	this->toFileJson(cfg);
	chips.push_back(std::move(cfg));

	// connectivity configuration
	connectivity["chipType"] = "";

	connectivity["chips"][0]["config"] = name+".json";
	connectivity["chips"][0]["tx"] = 0;
	connectivity["chips"][0]["rx"] = 0;
	connectivity["chips"][0]["locked"] = 1;
	connectivity["chips"][0]["enable"] = 1;

	return preset;
}
