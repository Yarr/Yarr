#include <iostream>
#include <chrono>
#include <unistd.h>
#include "SpecController.h"
#include "Rd53a.h"

#define EN_RX2 0x1
#define EN_RX1 0x2
#define EN_RX4 0x4
#define EN_RX3 0x8
#define EN_RX6 0x10
#define EN_RX5 0x20
#define EN_RX8 0x40
#define EN_RX7 0x80

#define EN_RX10 0x100
#define EN_RX9 0x200
#define EN_RX12 0x400
#define EN_RX11 0x800
#define EN_RX14 0x1000
#define EN_RX13 0x2000
#define EN_RX16 0x4000
#define EN_RX15 0x8000

#define EN_RX18 0x10000
#define EN_RX17 0x20000
#define EN_RX20 0x40000
#define EN_RX19 0x80000
#define EN_RX22 0x100000
#define EN_RX21 0x200000
#define EN_RX24 0x400000
#define EN_RX23 0x800000

int main(void) {

    SpecController spec;
    spec.init(0);
    //Send IO config to FMC
    //spec.writeSingle(0x6<<14 | 0x0, 0x55555555);
    spec.writeSingle(0x6<<14 | 0x0, EN_RX1 | EN_RX3 | EN_RX4 | EN_RX5);
    spec.writeSingle(0x6<<14 | 0x1, 0xF);
    spec.setCmdEnable(0x1);
    //spec.writeFifo(0x6666d271);
    
    Rd53a fe(&spec);
    fe.configure();
#if 0
    fe.wrRegister(0xF, 44, 0x4); // Reset aurora global pulse route
    spec.writeFifo(0x5c5cd28b); // Global pulse

    spec.writeFifo(0x69696969);
    spec.writeFifo(0x69696969);
    spec.writeFifo(0x69696969);
    spec.writeFifo(0x69696969);
    
    while(!spec.isCmdEmpty());
    
    fe.wrRegister(0xF, 21, 0); // PrmpVbp to 0 to see current change
    fe.wrRegister(0xF, 64, 0x41b); // 160Mbps mode (8=1280Mbps, 9=640Mbps, a=320Mbps, b=160Mbps)
    fe.wrRegister(0xF, 61, 0x3c);
    //fe.wrRegister(0xF, 68, 0x99);
    fe.wrRegister(0xF, 37, 32); // Trigger latency lower
    fe.wrRegister(0xF, 45, 100); // Reg read interval
    fe.wrRegister(0xF, 63, 4); // GP LVDS mux
    fe.wrRegister(0xF, 68, 0x55); // Select serializer (1 = chip data)
    fe.wrRegister(0xF, 69, 0x3F); // Enable all 4 lanes
    fe.wrRegister(0xF, 70, 600); // PreEmph
    fe.wrRegister(0xF, 71, 0); // PreEmph
    fe.wrRegister(0xF, 72, 0); // PreEmph

    //fe.wrRegister(0xF, 74, 0xF1); //CCSend = 1, CCWait=255
    //fe.wrRegister(0xF, 74, 0x0F); 
    
    fe.wrRegister(0xF, 32, 0xFFFF); // Enable digital cores
    fe.wrRegister(0xF, 33, 0xFFFF);
    fe.wrRegister(0xF, 34, 0x1);
    fe.wrRegister(0xF, 35, 0xFFFF);
    fe.wrRegister(0xF, 36, 0x1);
    
    spec.writeFifo(0x69696969);
    spec.writeFifo(0x69696969);
    spec.writeFifo(0x69696969);
    spec.writeFifo(0x69696969);

    while(!spec.isCmdEmpty());

    //fe.wrRegister(0xF, 44, 0x30); // Reset aurora global pulse route
    //spec.writeFifo(0x5c5cd28b); // Global pulse

    spec.writeFifo(0x69696969);
    spec.writeFifo(0x69696969);
    spec.writeFifo(0x69696969);
    spec.writeFifo(0x69696969);
    
    while(!spec.isCmdEmpty());
    
    fe.wrRegister(0xF, 44, (0x1 << 8)); // Enable 'monitor' (reg reads)
    spec.writeFifo(0x5c5cd28b);
    

    spec.writeFifo(0x69696969);
    spec.writeFifo(0x69696969);
    spec.writeFifo(0x69696969);
    spec.writeFifo(0x69696969);

    spec.writeFifo(0x5a5a6969); // ECR
    spec.writeFifo(0x59596969); // BCR
    
    sleep(2); 
    spec.writeFifo(0x69696969);
    spec.writeFifo(0x69696969);
    spec.writeFifo(0x69696969);
    spec.writeFifo(0x69696969);
    while(!spec.isCmdEmpty());
    
    
    spec.writeFifo(0x69696969);
    spec.writeFifo(0x69696969);
    spec.writeFifo(0x69696969);
    spec.writeFifo(0x69696969);
    spec.writeFifo(0x69696969);
    spec.writeFifo(0x69696969);
    spec.writeFifo(0x69696969);
    spec.writeFifo(0x69696969);
    
    spec.writeFifo(0x566a566c); // Trigger, Trigger
    //spec.writeFifo(0x4b6a4b6c); // T0T0, T0T0
    
    //spec.writeFifo(0x6a6c6ac6);
    //spec.writeFifo(0x69696969);
    //spec.writeFifo(0x5a5a6969);
#endif
    return 0;
}
