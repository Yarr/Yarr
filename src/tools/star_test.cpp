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
    
    if(controllerType == "spec") {
      //Send IO config to active FMC
      SpecController &s = *dynamic_cast<SpecController*>(&*hwCtrl);
      s.writeSingle(0x6<<14 | 0x0, 0x9ce730);
      s.writeSingle(0x6<<14 | 0x1, 0xF);
    }
    spec.setCmdEnable(0xFFFF); // LCB Port D
    spec.setRxEnable(0x0);

    StarChips star(&spec);
    std::array<uint16_t, 9> part1 = star.command_sequence(15, 15, 1, 41, 0x1); //Turn-off 8b10b
    std::array<uint16_t, 9> part2 = star.command_sequence(15, 15, 1, 41, 0x1); //Turn-off 8b10b

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
    spec.releaseFifo();

    spec.setRxEnable(0x40); // Channe 6

    RawData *data = spec.readData();

    for(int i=0; i<1000; i++) {
      if(data != nullptr) break;

      data = spec.readData();
    }

    if(data == nullptr) {
      std::cout << "No data\n";
      return 1;
    }

    std::cout << data->adr << " " << data->buf << " " << data->words << "\n";

    for (unsigned j=0; j<data->words;j++) {
      auto word = data->buf[j];
      if((j%2) && (word == 0xd3400000)) continue;
      if(!(j%2) && ((word&0xff) == 0xff)) continue;

      if((word&0xff) == 0x5f) continue;

      if(word == 0x1a0d) continue; // Ilde on chan 6
      if(word == 0x19f2) continue; // Ilde on chan 6

      word &= 0xffffc3ff; // Strip of channel number

      std::cout << "[" << j << "] = 0x" << std::hex << word << std::dec << " " << std::bitset<32>(word) << std::endl;
    }

    spec.setRxEnable(0x0);

    return 0;
}
