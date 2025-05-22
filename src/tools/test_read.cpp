#include <iostream>
#include <fstream>
#include <getopt.h>

#include "logging.h"

#include "AllHwControllers.h"
#include "ScanHelper.h"

namespace {
    auto logger = logging::make_log("test_read");
}

struct Config {
    std::string controllerConfig;
    std::string dataFile;
    std::vector<uint32_t> read_channels;
};

void printHelp() {
    std::cerr << "Usage: test_read -c <controller_config> -r <read_channel>\n";
    std::cerr << "Options:\n";
    std::cerr << "  -h, --help    Show this help message\n";
    std::cerr << "  -c, --controller-config <file>      Path to the controller configuration file\n";
    std::cerr << "  -d, --data-file <file>              Path to file containing packet data\n";
    std::cerr << "  -r, --read-channel <channel> <channel>     Channel numbers to read\n";
}

Config parseOptions(int argc, char* argv[]) {
    Config config;

    const struct option long_options[] =
      {
        {"help", no_argument, nullptr, 'h'},
        {nullptr, 0, nullptr, 0},
      };

    int opt;
    while ((opt = getopt(argc, argv, "hc:r:")) != -1) {
        switch (opt) {
            case 'h':
                printHelp();
                break;
            case 'c':
                config.controllerConfig = optarg;
                break;
            case 'r':
                optind -= 1;
                for (; optind < argc && *argv[optind] != '-'; optind += 1) {
                  try {
                    // Try parsing as number and throw if not
                    config.read_channels.push_back(std::stoi(optarg));
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
        std::cerr << "Failed to create hardware controller for: " << c.controllerConfig << std::endl;
        return 1;
    }

    std::fstream data_file(c.controllerConfig, std::ios::out | std::ios::binary);

    RxCore &rxCore = *hwCtrl;

    rxCore.setRxEnable(c.read_channels);

    while(1) {
        auto d = rxCore.readData();
        if (d.empty()) {
            logger->error("No data received");
            break;
        }

        for(const auto &dd: d) {
            // Write to file
            struct {
                uint32_t adr;
                uint32_t size;
            } header {dd->getAdr(),  dd->getSize()};

            data_file.write((const char *)&header, sizeof(header));
            data_file.write(reinterpret_cast<const char*>(dd->getBuf()), dd->getSize() * sizeof(uint32_t));
        }
    }
}
