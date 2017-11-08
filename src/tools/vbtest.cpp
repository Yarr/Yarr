#include <SpecCom.h>
#include <SpecTxCore.h>
#include <iostream>
#include <stdint.h>
#include <string.h>

void check(uint32_t off, uint32_t expected, SpecCom *mySpec, unsigned *errcnt){
  uint32_t rec = mySpec->readSingle(off);
  if ( rec != expected ) {
    std::cout << "Error reading adr " << off << " - Recieved: " << rec << ", Expected: " << expected << std::endl;
    ++errcnt;
  }
}

int main(int argc, char **argv) {
    int specNum = 0;
    if (argc == 2)
        specNum = atoi(argv[1]);
    SpecTxCore mySpec(specNum);
    unsigned err_count = 0;
    
    int trig_mask = 0x2; // 0b00010
    int trig_edge = 0x0; // 0b00000
    int trig_logic = 0x4; // 0b100 Trigger on single hit ch1
    int trig_deadtime = 200;

    std::cout << "Starting trigger config test ..." << std::endl;
    
    mySpec.setTriggerLogicMask(trig_mask);
    std::cout << "... set trigger mask to " << trig_mask << std::endl;
    mySpec.setTriggerLogicConfig(trig_logic);
    std::cout << "... set trigger logic to " << trig_logic << std::endl;
    mySpec.setTriggerEdge(trig_edge);
    std::cout << "... set trigger edge to " << trig_edge << std::endl;
    mySpec.setTriggerDelay(1, 3);
    // mySpec.setTriggerDelay(2, 6); 
    mySpec.setTriggerDelay(10, 0); // invalid channel
    std::cout << "... set trigger delays" << std::endl;
    mySpec.setTriggerDeadtime(trig_deadtime);
    std::cout << "... set deadtime to " << trig_deadtime << std::endl;

    check(TRIG_LOGIC_ADR | TRIG_LOGIC_MASK, trig_mask, &mySpec, &err_count);
    check(TRIG_LOGIC_ADR | TRIG_LOGIC_CONFIG, trig_logic, &mySpec, &err_count);
    check(TRIG_LOGIC_ADR | TRIG_LOGIC_EDGE, trig_edge, &mySpec, &err_count);
    check((TRIG_LOGIC_ADR | TRIG_LOGIC_DELAY) + 1, 3, &mySpec, &err_count);
    // check(TRIG_LOGIC_ADR | TRIG_LOGIC_DELAY + 2, 6, &mySpec, &err_count);
    check(TRIG_LOGIC_ADR | TRIG_LOGIC_DEADTIME, trig_deadtime, &mySpec, &err_count);

    if (err_count == 0)
        std::cout << "Success! No errors." << std::endl;

    return 0;
}
