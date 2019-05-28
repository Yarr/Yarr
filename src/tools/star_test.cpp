#include <iostream>

#include "SpecController.h"
#include "StarChips.h"
#include "LCBUtils.h"

int main(int argc, char *argv[]) {
    int specNum = 0;
    if (argc > 1)
            specNum = std::stoi(argv[1]);

    SpecController spec;
    spec.init(specNum);
    spec.toggleTrigAbort();
    spec.setTrigEnable(0);
    
    //Send IO config to active FMC
    spec.writeSingle(0x6<<14 | 0x0, 0x9ce730);
    spec.writeSingle(0x6<<14 | 0x1, 0xF);
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

    return 0;
}
