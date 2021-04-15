
// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Simple tool to create config
// # Date: April 2020
// ################################

#include <iostream>
#include <string>
#include <fstream>
#include <iomanip>

#include "storage.hpp"
#include "logging.h"
#include "LoggingConfig.h"

#include "AllChips.h"
#include "FrontEnd.h"
#include "StarCfg.h"

auto logger = logging::make_log("cfgCreator");

void createSingleChip(const std::string&, const std::string&, const std::string&);
void createStarLSStave(const std::string&, const std::string&, const std::string&);
void createStarPetal(const std::string&, const std::string&, const std::string&);

struct StarModule {
    std::string name; unsigned hccID; unsigned numABCs; unsigned tx; unsigned rx;
};

using configFunc = void(*)(const std::string&, const std::string&, const std::string&);

static const std::unordered_map<std::string, configFunc> createConfig {
    {"SingleChip", &createSingleChip},
    {"StripLSStave", &createStarLSStave},
    {"StripPetal", &createStarPetal}
};

void printHelp() {
    std::cout << "-t <string> : chip type" << std::endl;
    std::cout << "-s <string> : system type" << std::endl;
    std::cout << "-o <string> : output directory" << std::endl;
    std::cout << "-n <string> : Chip Name/Serial number" << std::endl;
}

int main (int argc, char *argv[]) {
    // Setup logger with some defaults
    std::string defaultLogPattern = "[%T:%e]%^[%=8l][%=15n]:%$ %v";
    spdlog::set_pattern(defaultLogPattern);
    json j; // empty
    j["pattern"] = defaultLogPattern;
    j["log_config"][0]["name"] = "all";
    j["log_config"][0]["level"] = "info";
    logging::setupLoggers(j);
 
    logger->info("Starting config creator!");

    logger->info("Parsing command line parameters ...");
    int c;
    std::string chipType = "RD53A";
    std::string systemType = "SingleChip";
    std::string outputDir = "configs/";
    std::string chipName = "JohnDoe";
    while ((c = getopt(argc, argv, "ht:s:o:n:")) != -1) {
        int count = 0;
        switch (c) {
            case 'h':
                printHelp();
                return 0;
                break;
            case 't':
                chipType = optarg;
                break;
            case 's':
                systemType = optarg;
                break;
            case 'o':
                outputDir = optarg;
                if (outputDir.back() != '/')
                    outputDir = outputDir + "/";
                break;
            case 'n':
                chipName = optarg;
                break;
            default:
                spdlog::critical("No command line parameters given!");
                return -1;
        }
    }

    logger->info("Chip Type   : {}", chipType);
    logger->info("System Type : {}", systemType);
    logger->info("Chip Name   : {}", chipName);
    logger->info("Ouput directory  : {}", outputDir);

    try {
        (createConfig.at(systemType))(chipType, chipName, outputDir);
    } catch (std::out_of_range &oor) {
        logger->error("Unknown system type {}", systemType);
        std::string knowntypes;
        for (const auto& f : createConfig) knowntypes += f.first+" ";
        logger->info("Known system types are: {}", knowntypes);
        return -1;
    } catch (std::runtime_error &e) {
        logger->error("Fail to create configuration files: {}", e.what());
        return -2;
    }

    logger->info("... done! Success, bye!");
    return 0;
}

void createSingleChip(const std::string& chipType, const std::string& chipName, const std::string& outputDir) {
    // Create FE object
    std::unique_ptr<FrontEnd> fe = StdDict::getFrontEnd(chipType);
    FrontEndCfg *feCfg = dynamic_cast<FrontEndCfg*>(fe.get());

    feCfg->setName(chipName);

    json cfg;
    feCfg->toFileJson(cfg);

    std::string outFilePath(outputDir+chipName+".json");
    logger->info("Writing default config to file {} ... ", outFilePath);

    std::ofstream outfile(outFilePath);
    outfile << std::setw(4) << cfg;
    outfile.close();
}

void createStarObject(const std::array<StarModule, 14>& modules,
                      const std::string& chipName,
                      const std::string& outputDir,
                      const std::string& configName) {
    // Connectivity config
    json systemCfg;
    systemCfg["chipType"] = "Star";

    for (int i=0; i<modules.size(); i++) {
        std::string mName(chipName+"_"+modules[i].name);
        systemCfg["chips"][i]["config"] = outputDir+mName+".json";
        systemCfg["chips"][i]["tx"] = modules[i].tx;
        systemCfg["chips"][i]["rx"] = modules[i].rx;
        systemCfg["chips"][i]["locked"] = 1;

        // Chip config
        // Create FE object
        auto fe = StdDict::getFrontEnd("Star");
        StarCfg* feCfg = dynamic_cast<StarCfg*>(fe.get());

        feCfg->setName(mName);
        feCfg->setHCCChipId(modules[i].hccID);

        // Add ABCStars
        feCfg->clearABCchipIDs();
        for (int iABC=0; iABC<modules[i].numABCs; iABC++) {
            feCfg->addABCchipID(iABC);
        }

        // Write chip config to file
        json chipCfg;
        feCfg->toFileJson(chipCfg);

        std::string chipFilePath(systemCfg["chips"][i]["config"]);
        logger->info("Write chip config to file {} ... ", chipFilePath);

        std::ofstream chipfile(chipFilePath);
        chipfile << std::setw(4) << chipCfg;
        chipfile.close();
    }

    // Write connectivity config to file
    std::string outFilePath(outputDir+configName);
    logger->info("Write connectivity config to file {} ... ", outFilePath);

    std::ofstream outfile(outFilePath);
    outfile << std::setw(4) << systemCfg;
    outfile.close();
}

void createStarLSStave(const std::string& chipType, const std::string& chipName, const std::string& outputDir) {
    if (chipType != "Star") {
        logger->error("Chip type has to be Star to create a long strip stave configuration.");
        throw std::runtime_error("Chip type error");
    }

    // 14 HCCStars on one side of a long strip stave
    const std::array<StarModule, 14> lsstave {{
        {.name="LS0",  .hccID=0,  .numABCs=10, .tx=100, .rx=13 },
        {.name="LS1",  .hccID=1,  .numABCs=10, .tx=100, .rx=12 },
        {.name="LS2",  .hccID=2,  .numABCs=10, .tx=100, .rx=11 },
        {.name="LS3",  .hccID=3,  .numABCs=10, .tx=100, .rx=10 },
        {.name="LS4",  .hccID=4,  .numABCs=10, .tx=101, .rx=9  },
        {.name="LS5",  .hccID=5,  .numABCs=10, .tx=101, .rx=8  },
        {.name="LS6",  .hccID=6,  .numABCs=10, .tx=101, .rx=7  },
        {.name="LS7",  .hccID=7,  .numABCs=10, .tx=101, .rx=6  },
        {.name="LS8",  .hccID=8,  .numABCs=10, .tx=102, .rx=5  },
        {.name="LS9",  .hccID=9,  .numABCs=10, .tx=102, .rx=4  },
        {.name="LS10", .hccID=10, .numABCs=10, .tx=102, .rx=3  },
        {.name="LS11", .hccID=11, .numABCs=10, .tx=102, .rx=2  },
        {.name="LS12", .hccID=12, .numABCs=10, .tx=102, .rx=1  },
        {.name="LS13", .hccID=13, .numABCs=10, .tx=103, .rx=0  }
    }};

    createStarObject(lsstave, chipName, outputDir, "example_lsstave_setup.json");
}

void createStarPetal(const std::string& chipType, const std::string& chipName, const std::string& outputDir) {
    if (chipType != "Star") {
        logger->error("Chip type has to be Star to create a strip petal configuration.");
        throw std::runtime_error("Chip type error");
    }

    // 14 HCCStars on one side of a petal
    const std::array<StarModule, 14> petal {{
        {.name="R0_H0", .hccID=0,  .numABCs=8,  .tx=102, .rx=16 },
        {.name="R0_H1", .hccID=1,  .numABCs=9,  .tx=102, .rx=24 },
        {.name="R1_H0", .hccID=2,  .numABCs=10, .tx=102, .rx=20 },
        {.name="R1_H1", .hccID=3,  .numABCs=11, .tx=102, .rx=26 },
        {.name="R2_H0", .hccID=4,  .numABCs=6,  .tx=101, .rx=22 },
        {.name="R2_H1", .hccID=5,  .numABCs=6,  .tx=101, .rx=12 },
        {.name="R3_H0", .hccID=6,  .numABCs=7,  .tx=101, .rx=14 },
        {.name="R3_H1", .hccID=7,  .numABCs=7,  .tx=101, .rx=18 },
        {.name="R3_H2", .hccID=8,  .numABCs=7,  .tx=101, .rx=0  },
        {.name="R3_H3", .hccID=9,  .numABCs=7,  .tx=101, .rx=2  },
        {.name="R4_H0", .hccID=10, .numABCs=8,  .tx=103, .rx=4  },
        {.name="R4_H1", .hccID=11, .numABCs=8,  .tx=103, .rx=8  },
        {.name="R5_H0", .hccID=12, .numABCs=9,  .tx=103, .rx=6  },
        {.name="R5_H1", .hccID=13, .numABCs=9,  .tx=103, .rx=10 }
    }};

    createStarObject(petal, chipName, outputDir, "example_petal_setup.json");
}
