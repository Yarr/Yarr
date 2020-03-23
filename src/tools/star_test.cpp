#include <bitset>
#include <iostream>

#include "SpecController.h"
#include "AllHwControllers.h"
#include "StarChips.h"
#include "LCBUtils.h"

int main(int argc, char *argv[]) {
    std::string controller;
    std::string controllerType;

    if (argc > 1)
      controller = argv[1];

    std::unique_ptr<HwController> hwCtrl = nullptr;
    if(controller.empty()) {
	controllerType = "spec";
        hwCtrl = StdDict::getHwController(controllerType);
        // hwCtrl->init(0);
    } else {
        json ctrlCfg;
        std::ifstream controllerFile(controller);
        try {
            ctrlCfg = json::parse(controllerFile);
        } catch (json::parse_error &e) {
            std::cerr << "#ERROR# Could not parse config: " << e.what() << std::endl;
            return 1;
        }
	controllerType = ctrlCfg["ctrlCfg"]["type"];
        hwCtrl = StdDict::getHwController(controllerType);
        hwCtrl->loadConfig(ctrlCfg["ctrlCfg"]["cfg"]);
    }

    HwController &spec = *hwCtrl;
    spec.toggleTrigAbort();
    spec.setTrigEnable(0);

    // In fact, mostly needed only for a specific test version of Spec FW
    bool do_spec_specific = controllerType == "spec";

    if(do_spec_specific) {
      //Send IO config to active FMC
      SpecController &s = *dynamic_cast<SpecController*>(&*hwCtrl);
      s.writeSingle(0x6<<14 | 0x0, 0x9ce730);
      s.writeSingle(0x6<<14 | 0x1, 0xF);
    }
    spec.setCmdEnable(0xFFFF); // LCB Port D
    spec.setRxEnable(0x0);

    StarChips star(&spec);

    uint32_t regNum = 41;
    // Default (power-on) value
    uint32_t regVal = 0x00020001;
    //Turn-off 8b10b
    // Bit 16 = 1, Bit 17 = 0
    regVal |= (1<<16);
    regVal &= ~(1<<17);

    std::array<uint16_t, 9> part1 = star.write_hcc_register(regNum, regVal);
    // Need to write two registers
    std::array<uint16_t, 9> part2 = star.write_hcc_register(regNum+1, regVal);

    spec.writeFifo((LCB::IDLE << 16) + LCB::IDLE);
    spec.writeFifo((part1[0] << 16) + part1[1]);
    spec.writeFifo((part1[2] << 16) + part1[3]);
    spec.writeFifo((part1[4] << 16) + part1[5]);
    spec.writeFifo((part1[6] << 16) + part1[7]);
    spec.writeFifo((part1[8] << 16) + LCB::IDLE);
    spec.writeFifo((LCB::IDLE << 16) + LCB::IDLE);
    spec.writeFifo((part2[0] << 16) + part2[1]);
    spec.writeFifo((part2[2] << 16) + part2[3]);
    spec.writeFifo((part2[4] << 16) + part2[5]);
    spec.writeFifo((part2[6] << 16) + part2[7]);
    spec.writeFifo((part2[8] << 16) + LCB::IDLE);
    spec.writeFifo((LCB::IDLE << 16) + LCB::IDLE);

    // Test emulator
    if (controllerType == "emu_Star") {
      // Send fast commands:
      spec.writeFifo((LCB::IDLE << 16) + LCB::fast_command(LCB::ABC_HIT_COUNT_RESET, 0));
      spec.writeFifo((LCB::IDLE << 16) + LCB::fast_command(LCB::ABC_HIT_COUNT_START, 0));
      spec.writeFifo((LCB::IDLE << 16) + LCB::IDLE);

      // Write to ABCStar register CREG0 to enable hit counters
      std::array<LCB::Frame, 9> writeABCCmd = star.write_abc_register(32, 0x00000020); 
      spec.writeFifo((writeABCCmd[0] << 16) + writeABCCmd[1]);
      spec.writeFifo((writeABCCmd[2] << 16) + writeABCCmd[3]);
      spec.writeFifo((writeABCCmd[4] << 16) + writeABCCmd[5]);
      spec.writeFifo((writeABCCmd[6] << 16) + writeABCCmd[7]);
      spec.writeFifo((writeABCCmd[8] << 16) + LCB::IDLE);

      // send an L0A
      spec.writeFifo((LCB::IDLE << 16) + LCB::l0a_mask(10, 0, false));

      // read an HCCStar register
      //
      std::array<LCB::Frame, 9> readHCCCmd = star.read_hcc_register(44);
      spec.writeFifo((readHCCCmd[0] << 16) + readHCCCmd[1]);
      spec.writeFifo((readHCCCmd[2] << 16) + readHCCCmd[8]);

      // read another HCCStar register
      std::array<LCB::Frame, 9> readHCCCmd2 = star.read_hcc_register(17);
      spec.writeFifo((readHCCCmd2[0] << 16) + readHCCCmd2[1]);
      // the read command is interupted by an L0A
      spec.writeFifo((LCB::l0a_mask(1, 0, false) << 16) + readHCCCmd2[2]);
      spec.writeFifo((readHCCCmd2[8] << 16) + LCB::IDLE);

      // read an ABCStar register with broadcast addresses
      // HitCountReg60: hit counts for front-end channel 243, 242, 241, and 240
      std::array<LCB::Frame, 9> readABCCmd = star.read_abc_register(172);
      spec.writeFifo((readABCCmd[0] << 16) + readABCCmd[1]);
      //////
      // readABCCmd[3:7] not really needed
      // read_abc_register() returns a command sequence of 9 frames instead of 4 for a read sequence
      // They are ignored in the emulator
      spec.writeFifo((readABCCmd[2] << 16) + readABCCmd[3]);
      spec.writeFifo((readABCCmd[4] << 16) + readABCCmd[5]);
      spec.writeFifo((readABCCmd[6] << 16) + readABCCmd[7]);
      //////
      spec.writeFifo((readABCCmd[8] << 16) + LCB::IDLE);
    }

    spec.releaseFifo();

    if(do_spec_specific) {
      spec.setRxEnable(0x40); // Input from Channel 6 on Spec board
    }

    std::unique_ptr<RawData> data(spec.readData());

    bool nodata = true;

    while (true) {

      while (data) {
        nodata = false;

        std::cout << data->adr << " " << data->buf << " " << data->words << "\n";

        for (unsigned j=0; j<data->words;j++) {
          auto word = data->buf[j];

          if(do_spec_specific) {
            if((j%2) && (word == 0xd3400000)) continue;
            if(!(j%2) && ((word&0xff) == 0xff)) continue;

            if((word&0xff) == 0x5f) continue;

            if(word == 0x1a0d) continue; // Idle on chan 6
            if(word == 0x19f2) continue; // Idle on chan 6

            word &= 0xffffc3ff; // Strip of channel number
          }

          std::cout << "[" << j << "] = 0x" << std::hex << word << std::dec << " " << std::bitset<32>(word) << std::endl;
        }

        data.reset(spec.readData());
      }

      // wait a while if no data
      for(int i=0; i<1000; i++) {
        if(data) break;

        static const auto SLEEP_TIME = std::chrono::milliseconds(1);

        std::this_thread::sleep_for( SLEEP_TIME );

        data.reset(spec.readData());
      }

      if (data == nullptr) break;
    }

    if(nodata) {
      std::cout << "No data\n";
      return 1;
    }

    spec.setRxEnable(0x0);

    return 0;
}
