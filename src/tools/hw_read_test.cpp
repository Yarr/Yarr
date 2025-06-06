#include <csignal>
#include <fstream>
#include <iostream>

#include <getopt.h>

#include "logging.h"
#include "LoggingConfig.h"

#include "AllHwControllers.h"
#include "ScanHelper.h"

namespace {
    auto logger = logging::make_log("hw_read_test");
}

struct Config {
    std::string controllerConfig;
    std::string dataFile;
    std::vector<uint32_t> read_channels;
};

void printHelp() {
    std::cerr << "Usage: hw_read_test -c <controller_config> -r <read_channel>\n";
    std::cerr << "  Direct read from HwController to file\n";
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
    while ((opt = getopt_long(argc, argv, "hc:d:r:", long_options, nullptr)) != -1) {
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

    if(config.read_channels.empty()) {
        config.read_channels.push_back(0);
    }

    return config;
}

std::atomic<bool> stop_signalled{false};

int main(int argc, char* argv[]) {
    json loggerConfig;
    loggerConfig["pattern"] = "[%T:%e]%^[%=8l][%=15n][%t]:%$ %v";
    loggerConfig["log_config"][0]["name"] = "all";
    loggerConfig["log_config"][0]["level"] = "info";
    loggerConfig["outputDir"] = "";
    logging::setupLoggers(loggerConfig);

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
        logger->error("Failed to create hardware controller for: {}", c.controllerConfig);
        return 1;
    }

    std::fstream data_file(c.dataFile, std::ios::out | std::ios::binary);

    if(!data_file.is_open()) {
        logger->error("Failed to open output data file: {}", c.dataFile);
        return 1;
    }

    RxCore &rxCore = *hwCtrl;

    signal(SIGINT, [](int signum){
        stop_signalled = true;
        logger->info("Received signal {}, stopping...", signum);
    });

    rxCore.initRxChannels(c.read_channels);
    rxCore.setRxEnable(c.read_channels);

    using clk = std::chrono::steady_clock;
    clk::time_point start_time = std::chrono::steady_clock::now();
    uint32_t packet_count = 0;

    while(!stop_signalled) {
        auto d = rxCore.readData();
        if (d.empty()) {
            // logger->warn("No data received");
            continue;
        }

        for(const auto &dd: d) {
            packet_count ++;
            // Write to file
            struct {
                uint32_t adr;
                uint32_t size;
            } header {dd->getAdr(),  dd->getSize()};

            data_file.write((const char *)&header, sizeof(header));
            data_file.write(reinterpret_cast<const char*>(dd->getBuf()), dd->getSize() * sizeof(uint32_t));
        }
    }

    clk::time_point end_time = std::chrono::steady_clock::now();
    auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    logger->info("Packet count: {}", packet_count);
    logger->info("Elapsed time: {} ms", elapsed_time);
    logger->info("Packet rate: {} kHz", packet_count/(double)elapsed_time);

    return 0;
}
