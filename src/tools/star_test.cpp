#include <bitset>
#include <iostream>

#include "SpecController.h"
#include "AllHwControllers.h"
#include "StarCmd.h"
#include "LCBUtils.h"
#include "LoggingConfig.h"
#include "ScanHelper.h"
#include "StarChipPacket.h"
#include "logging.h"

static void printHelp();

void sendCommand(const std::array<uint16_t, 9>& cmd, HwController& hwCtrl);
void sendCommand(uint16_t cmd, HwController& hwCtrl);
void configureChips(StarCmd &star, HwController& hwCtrl, bool doReset);
void runTests(StarCmd &star, HwController& hwCtrl);

void reportData(RawData &data, std::string controllerType) {
  std::cout << "Raw data from RxCore:\n";
  std::cout << data.adr << " " << data.buf << " " << data.words << "\n";

  bool do_spec_specific = controllerType == "spec";

  for (unsigned j=0; j<data.words;j++) {
    auto word = data.buf[j];

    if(do_spec_specific) {
      if((j%2) && (word == 0xd3400000)) continue;
      if(!(j%2) && ((word&0xff) == 0xff)) continue;

      if((word&0xff) == 0x5f) continue;

      if(word == 0x1a0d) continue; // Idle on chan 6
      if(word == 0x19f2) continue; // Idle on chan 6

      word &= 0xffffc3ff; // Strip of channel number
    }

    std::cout << "[" << j << "] = " << std::setfill('0') << std::hex << std::setw(8) << word << std::dec << " " << std::bitset<32>(word) << std::endl;
  }

  StarChipPacket packet;
  packet.add_word(0x13C); //add SOP
  for(unsigned iw=0; iw<data.words; iw++) {
    for(int i=0; i<4;i++){
      packet.add_word((data.buf[iw]>>i*8)&0xFF);
    }
  }
  packet.add_word(0x1DC); //add EOP
  if(packet.parse()) {
    std::cout << "Parse error\n";
  } else {
    auto packetType = packet.getType();
    if(packetType == TYP_LP || packetType == TYP_PR) {
      packet.print_clusters(std::cout);
    } else if(packetType == TYP_ABC_RR || packetType == TYP_HCC_RR || packetType == TYP_ABC_HPR || packetType == TYP_HCC_HPR) {
      packet.print_more(std::cout);
    }
  }
}

int main(int argc, char *argv[]) {
    std::string controller;
    std::string controllerType;

    {
      json j; // Start empty
      std::string defaultLogPattern = "[%T:%e]%^[%=8l][%=15n]:%$ %v";
      j["pattern"] = defaultLogPattern;
      j["log_config"][0]["name"] = "all";
      j["log_config"][0]["level"] = "info";
      logging::setupLoggers(j);
    }

    // Original Spec version
    int rxChannel = 6;
    int txChannel = 0xFFFF;
    int c;
    while ((c = getopt(argc, argv, "hl:r:t:")) != -1) {
      switch(c) {
      case 'h':
        printHelp();
        return 0;
      case 'l':
        try {
          std::string logPath = std::string(optarg);
          auto j = ScanHelper::openJsonFile(logPath);
          logging::setupLoggers(j);
        } catch (std::runtime_error &e) {
          spdlog::error("Opening logger config: {}", e.what());
          return 1;
        }
        break;
      case 'r':
        rxChannel = atoi(optarg);
        break;
      case 't':
        txChannel = atoi(optarg);
        break;
      default:
        spdlog::critical("Error while parsing command line parameters!");
        return -1;
      }
    }

    if (optind != argc) {
      // First positional parameter (optind is first not parsed by getopt)
      controller = argv[optind];
    }
    std::cout << std::endl;
    std::unique_ptr<HwController> hwCtrl = nullptr;
    if(controller.empty()) {
	controllerType = "spec";
        hwCtrl = StdDict::getHwController(controllerType);
        // hwCtrl->init(0);
    } else {
      try {
        std::cout << " Using controller from " << controller << "\n";
        json ctrlCfg = ScanHelper::openJsonFile(controller);
        controllerType = ctrlCfg["ctrlCfg"]["type"];
        hwCtrl = StdDict::getHwController(controllerType);
        hwCtrl->loadConfig(ctrlCfg["ctrlCfg"]["cfg"]);
      } catch (std::runtime_error &e) {
        spdlog::error("Opening controller config: {}", e.what());
        return 1;
      }
    }

    hwCtrl->toggleTrigAbort();
    hwCtrl->setTrigEnable(0);

    // In fact, mostly needed only for a specific test version of Spec FW
    bool do_spec_specific = controllerType == "spec";

    if(do_spec_specific) {
      //Send IO config to active FMC
      SpecController &s = *dynamic_cast<SpecController*>(&*hwCtrl);
      s.writeSingle(0x6<<14 | 0x0, 0x9ce730);
      s.writeSingle(0x6<<14 | 0x1, 0xF);
    }

    hwCtrl->setCmdEnable(txChannel);

    // First disable all input
    hwCtrl->disableRx();
    hwCtrl->setRxEnable(rxChannel);

    StarCmd star;

    configureChips(star, *hwCtrl, true);
    runTests(star, *hwCtrl);

    std::unique_ptr<uint32_t[]> tidy_up;
    std::unique_ptr<RawData> data(hwCtrl->readData());
    if(data) {
      std::cout << "Use data: " << (void*)data->buf << " (init)\n";
      tidy_up.reset(data->buf);
    }

    bool nodata = true;
    auto start_reading = std::chrono::steady_clock::now();

    while (true) {

      while (data) {
        nodata = false;

        reportData(*data, controllerType);

        data.reset(hwCtrl->readData());
        if(data) {
          std::cout << "Use data: " << (void*)data->buf << " (main)\n";
          tidy_up.reset(data->buf);
        }

        auto run_time = std::chrono::steady_clock::now() - start_reading;
        if(run_time > std::chrono::seconds(2))
          break;
      }

      // wait a while if no data
      for(int i=0; i<1000; i++) {
        if(data) break;

        static const auto SLEEP_TIME = std::chrono::milliseconds(1);

        std::this_thread::sleep_for( SLEEP_TIME );

        data.reset(hwCtrl->readData());
        if(data) {
          std::cout << "Use data: " << (void*)data->buf << " (while)\n";
          tidy_up.reset(data->buf);
        }
      }

      if (data == nullptr) break;
    }

    hwCtrl->disableRx();

    if(nodata) {
      std::cout << "No data\n";
      return 1;
    }

    return 0;
}

void configureChips(StarCmd &star, HwController& hwCtrl, bool doReset) {
  // reset
  if (doReset) {
    sendCommand( LCB::fast_command(LCB::LOGIC_RESET, 0), hwCtrl);
    sendCommand( LCB::fast_command(LCB::HCC_REG_RESET, 0), hwCtrl);
  }

  //////////
  // Configure HCCStar first to establish communication with ABCStars
  // All commands are broadcasted

  // Register 32 (Delay1): delays for signals to ABCStar
  sendCommand( star.write_hcc_register(32, 0x02400000), hwCtrl);

  // Register 33, 34 (Delay2, Delay3): delays for data from ABCStar
  sendCommand( star.write_hcc_register(33, 0x44444444), hwCtrl);
  sendCommand( star.write_hcc_register(34, 0x00000444), hwCtrl);

  // Register 38 (DRV1): enable driver and currents
  sendCommand( star.write_hcc_register(38, 0x0fffffff), hwCtrl);

  // Register 40 (ICenable): enable input channels
  sendCommand( star.write_hcc_register(40, 0x000007ff), hwCtrl);

  //////////
  // Configure ABCStar
  // reset ABCStar
  if (doReset) {
    sendCommand( LCB::fast_command(LCB::ABC_REG_RESET, 0), hwCtrl );
    sendCommand( LCB::fast_command(LCB::ABC_SLOW_COMMAND_RESET, 0), hwCtrl);
  }

  // Register 32 (CREG0): Set RR mode to 1, enable LP and PR
  sendCommand( star.write_abc_register(32, 0x00000700), hwCtrl);
}

void runTests(StarCmd &star, HwController& hwCtrl) {

  // Turn off HCC HPR
  sendCommand( star.write_hcc_register(43, 0x00000100), hwCtrl);
  sendCommand( star.write_hcc_register(16, 0x00000001), hwCtrl);

  // Turn off ABC HPR
  sendCommand( star.write_abc_register(32, 0x00000740), hwCtrl);
  sendCommand( star.write_abc_register(0, 0x00000004), hwCtrl);

  //////////
  // Read HCCStar registers
  // register 17: Addressing
  sendCommand( star.read_hcc_register(17), hwCtrl);
  // register 40: ICenable
  sendCommand( star.read_hcc_register(40), hwCtrl);

  //////////
  // Read ABCStar registers
  // register 32: CREG0
  sendCommand( star.read_abc_register(32), hwCtrl);
  // register 34: CREG2
  sendCommand( star.read_abc_register(34), hwCtrl);

  //////////
  // Read data packets

  // Set some mask registers
  sendCommand( star.write_abc_register(19, 0xfffe0000), hwCtrl); // MaskInput3
  sendCommand( star.write_abc_register(23, 0xfffe0000), hwCtrl); // MaskInput7

  // Enable hit counters
  sendCommand( star.write_abc_register(32, 0x00000760), hwCtrl);

  // Reset and start ABCStar hit counters
  sendCommand(LCB::fast_command(LCB::ABC_HIT_COUNT_RESET, 0), hwCtrl);
  sendCommand(LCB::fast_command(LCB::ABC_HIT_COUNT_START, 0), hwCtrl);
  // Enable PR & LP
  sendCommand(LCB::fast_command(LCB::HCC_START_PRLP, 0), hwCtrl);
  // BC Reset
  sendCommand(LCB::lonely_bcr(), hwCtrl);

  /////
  // Static test mode: TM = 1
  sendCommand( star.write_abc_register(32, 0x00010760), hwCtrl);

  // Send a trigger
  sendCommand( LCB::l0a_mask(1, 42, false), hwCtrl);

  /////
  // Test pulse mode: TM = 2
  // Single digital pulse: TestPulseEnable = 1, TestPattEnable = 0
  sendCommand( star.write_abc_register(32, 0x00020770), hwCtrl);

  // Set the L0 pipeline latency to a smaller value: 15 BC
  sendCommand( star.write_abc_register(34, 0x0000000f), hwCtrl);

  // Also set BCIDrstDelay in the HCC to L0 latency - 2 to avoid BCID errors
  sendCommand( star.write_hcc_register(44, 0x0000000d), hwCtrl);

  sendCommand( LCB::lonely_bcr(), hwCtrl);

  // Send a digital pulse followed by a trigger
  std::array<uint16_t, 9> cmd = {
    LCB::IDLE, LCB::IDLE,
    LCB::fast_command(LCB::ABC_DIGITAL_PULSE, 0), LCB::IDLE,
    LCB::IDLE, LCB::IDLE,
    LCB::l0a_mask(1, 43, false), LCB::IDLE,
    LCB::IDLE
  };
  sendCommand(cmd, hwCtrl);

  // Read a ABCStar hit count
  // register 191: HitCountREG63
  sendCommand(star.read_abc_register(191), hwCtrl);
}

void sendCommand(const std::array<uint16_t, 9>& cmd, HwController& hwCtrl) {
  hwCtrl.writeFifo((LCB::IDLE << 16) + LCB::IDLE);
  hwCtrl.writeFifo((cmd[0] << 16) + cmd[1]);
  hwCtrl.writeFifo((cmd[2] << 16) + cmd[3]);
  hwCtrl.writeFifo((cmd[4] << 16) + cmd[5]);
  hwCtrl.writeFifo((cmd[6] << 16) + cmd[7]);
  hwCtrl.writeFifo((cmd[8] << 16) + LCB::IDLE);
  hwCtrl.releaseFifo();
}

void sendCommand(uint16_t cmd, HwController& hwCtrl) {
  hwCtrl.writeFifo((LCB::IDLE << 16) + cmd);
  hwCtrl.releaseFifo();
}

void printHelp() {
  std::cout << "Usage: [OPTIONS] ... [HW_CONFIG]\n";
  std::cout << "   Run Star FE tests with HardwareController configuration from HW_CONFIG\n";
  std::cout << " -h: Show this help.\n";
  std::cout << " -r <channel> : Rx channel to enable.\n";
  std::cout << " -t <channel> : Tx channel to enable.\n";
  std::cout << " -l <log_config> : Configure loggers.\n";
}
