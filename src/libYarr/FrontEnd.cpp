// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Abstract FE class
// # Comment: Combined multiple FE 
// ################################

#include "FrontEnd.h"

#include <iomanip>

bool FrontEnd::isActive() {
	return active;
}

bool FrontEnd::getActive() {
	return this->active;
}

void FrontEnd::setActive(bool arg_active) {
	active = arg_active;
}

void FrontEndCfg::createExampleConfig(const std::string& outputDir, const std::string& systemType) {
	if (systemType != "SingleChip") {
		throw std::runtime_error("Unknown system type: "+systemType);
	}

	json cfg;
	this->toFileJson(cfg);

	std::string outFilePath(outputDir+name+".json");

	std::ofstream outfile(outFilePath);
	outfile << std::setw(4) << cfg;
	outfile.close();
}
