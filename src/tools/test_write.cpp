#include <iostream>
#include <fstream>
#include <getopt.h>

#include "logging.h"
#include "LoggingConfig.h"

#include "AllHwControllers.h"
#include "ScanHelper.h"

namespace {
    auto logger = logging::make_log("test_write");
}

struct Config {
    std::string controllerConfig;
    std::string dataFile;
    std::vector<uint32_t> write_channels;
};

void printHelp() {
    std::cerr << "Usage: test_write -c <controller_config> -w <write_channel>\n";
    std::cerr << "Options:\n";
    std::cerr << "  -h, --help    Show this help message\n";
    std::cerr << "  -c, --controller-config <file>      Path to the controller configuration file\n";
    std::cerr << "  -d, --data-file <file>              Path to file to read data to hardware\n";
    std::cerr << "  -w, --write-channel <channel> <channel>     Channel numbers to write\n";
}

Config parseOptions(int argc, char* argv[]) {
    json loggerConfig;
    loggerConfig["pattern"] = "[%T:%e]%^[%=8l][%=15n][%t]:%$ %v";
    loggerConfig["log_config"][0]["name"] = "all";
    loggerConfig["log_config"][0]["level"] = "info";
    loggerConfig["outputDir"] = "";
    logging::setupLoggers(loggerConfig);

    Config config;

    const struct option long_options[] =
      {
        {"help", no_argument, nullptr, 'h'},
        {nullptr, 0, nullptr, 0},
      };

    int opt;
    while ((opt = getopt_long(argc, argv, "hc:d:w:", long_options, nullptr)) != -1) {
        switch (opt) {
            case 'h':
                printHelp();
                break;
            case 'c':
                config.controllerConfig = optarg;
                break;
            case 'd':
                config.dataFile = optarg;
                break;
            case 'w':
                optind -= 1;
                for (; optind < argc && *argv[optind] != '-'; optind += 1) {
                  try {
                    // Try parsing as number and throw if not
                    config.write_channels.push_back(std::stoi(optarg));
                  } catch(std::exception &e) {
                    break;
                  }
                }
                break;
            default:
                spdlog::critical("Error while parsing command line parameters!");
                printHelp();
                exit(EXIT_FAILURE);
        }
    }
    return config;
}

int main(int argc, char* argv[]) {

    Config c = parseOptions(argc, argv);

    if(c.dataFile.empty()) {
        logger->error("No data file given to load data from");
        return 1;
    }

    std::fstream data_file(c.dataFile, std::ios::in | std::ios::binary);
    if (!data_file.is_open()) {
        logger->error("Failed to open input data file: {}", c.dataFile);
        return 1;
    }

    std::unique_ptr<HwController> hwCtrl = nullptr;
    if(c.controllerConfig.empty()) {
      logger->error("No controller specified");
      return 1;
    } else {
      try {
        logger->info("Using controller from {}", c.controllerConfig);
        json ctrlCfg = ScanHelper::openJsonFile(c.controllerConfig);
        auto controllerType = ctrlCfg["ctrlCfg"]["type"];
        hwCtrl = StdDict::getHwController(controllerType);
        hwCtrl->loadConfig(ctrlCfg["ctrlCfg"]["cfg"]);
      } catch (std::runtime_error &e) {
        logger->error("Opening controller config: {}", e.what());
        return 1;
      }
    }

    if (hwCtrl == nullptr) {
        std::cerr << "Failed to create hardware controller for: " << c.controllerConfig << "\n";
        return 1;
    }

    TxCore &txCore = *hwCtrl;

    txCore.setCmdEnable(c.write_channels);

    static const size_t BUFFER_SIZE = 1000; 
    std::array<uint8_t, BUFFER_SIZE> buffer;
    while(1) {
        if(data_file.eof()) {
            logger->info("End of file reached, resetting file pointer");
            data_file.clear();
            data_file.seekg(0, std::ios::beg);
        }
        logger->info("At file offset {}", data_file.tellg());

        struct {
                uint32_t adr;
                uint32_t size;
        } header;

        data_file.read((char *)&header, sizeof(header));
        logger->info("Read header: adr = {:08x}, size = {}", header.adr, header.size);

        if(header.size > BUFFER_SIZE) {
            logger->error("Data size {} is too large from header", header.size);
            exit(1);
        }
        data_file.read((char *)buffer.data(), header.size);

        if(std::find(c.write_channels.begin(), c.write_channels.end(), header.adr) == c.write_channels.end()) {
            logger->error("Channel {:08x} not in configured channels", header.adr);
            exit(1);
        }
        txCore.setCmdEnable(header.adr);

        uint32_t *data = (uint32_t*)buffer.data();
        for(size_t i = 0; i < header.size; ++i) {
            txCore.writeFifo(data[i]);
        }
        txCore.releaseFifo();
    }
}
