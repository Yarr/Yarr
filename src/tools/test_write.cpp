#include <iostream>
#include <fstream>
#include <getopt.h>

#include "logging.h"

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
    std::cerr << "  -c, --controller-config <file>              Path to the controller configuration file\n";
    std::cerr << "  -w, --write-channel <channel> <channel>     Channel numbers to write\n";
}

Config parseOptions(int argc, char* argv[]) {
    Config config;

    const struct option long_options[] =
      {
        {"help", no_argument, nullptr, 'h'},
        {nullptr, 0, nullptr, 0},
      };

    int opt;
    while ((opt = getopt(argc, argv, "hc:w:")) != -1) {
        switch (opt) {
            case 'h':
                printHelp();
                break;
            case 'c':
                config.controllerConfig = optarg;
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

    std::fstream data_file(c.controllerConfig, std::ios::in | std::ios::binary);
    if (!data_file.is_open()) {
        std::cerr << "Failed to open input file: " << c.controllerConfig << "\n";
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

    std::array<uint8_t, 1000> buffer;
    while(1) {
        if(data_file.eof()) {
            logger->info("End of file reached, resetting file pointer");
            data_file.seekg(0, std::ios::beg);
        }

        struct {
                uint32_t adr;
                uint32_t size;
        } header;

        data_file.read((char *)&header, sizeof(header));
        data_file.read((char *)buffer.data(), header.size);

        uint32_t *data = (uint32_t*)buffer.data();
        for(size_t i = 0; i < header.size; ++i) {
            txCore.writeFifo(data[i]);
        }
        txCore.releaseFifo();
    }
}
