#include <string>
#include <iostream>
#include "logging.h"
#include "LoggingConfig.h"

#include "Histo2d.h"
#include "Histo1d.h"
#include "Histo3d.h"
#include "GraphErrors.h"

int main(int argc, char*argv[]) {
	// Setup logger with some defaults
	std::string defaultLogPattern = "[%T:%e]%^[%=8l][%=15n]:%$ %v";
	spdlog::set_pattern(defaultLogPattern);
	json j; // empty
	j["pattern"] = defaultLogPattern;
	j["log_config"][0]["name"] = "all";
	j["log_config"][0]["level"] = "info";
	logging::setupLoggers(j);

	if (argc < 2 || argc > 2) {
		std::cout << "Usage: " << argv[0] << " <filename>" << std::endl;
		return -1;
	}
	Histo1d h1("Temp1", 1, 0.0, 1.0);
	Histo2d h2("Temp2", 1, 0.0, 1.0, 1, 0.0, 1.0);
	Histo3d h3("Temp3", 1, 0.0, 1.0, 1, 0.0, 1.0, 1, 0.0, 1.0);
	GraphErrors ge("TempErr");

	if(h1.fromFile(std::string(argv[1]))){
		h1.plot("replot", "./");
	} else if (h2.fromFile(std::string(argv[1]))) {
		h2.plot("replot", "./");
	} else if(h3.fromFile(std::string(argv[1]))){
		h3.plot("replot", "./");
	} else if(ge.fromFile(std::string(argv[1]))){
		ge.plot("replot", "./");
	} else {
		std::cout << "ABORTING: Could not read as either 1D or 2D histogram for replotting" << std::endl;
	}
	return 0;
}