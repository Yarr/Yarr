
#include <iostream>

#include "SpecCom.h"
#include "SpecTxCore.h"
#include "SpecRxCore.h"
#include "Fe65p2.h"
#include "Bookkeeper.h"

int main(void) {

    std::cout << "Initialising .." << std::endl;
    SpecCom spec(0);
    SpecTxCore tx(&spec);
    SpecRxCore rx(&spec);
    Bookkeeper bookie(&tx, &rx);

    Fe65p2 *fe = bookie.g_fe65p2;

    std::cout << "Configuring FE .." << std::endl;
    tx.setCmdEnable(0x1);
    
    fe->setPlsrDac(1000);
    fe->enAnaInj();
    fe->clocksOn();
    while(!tx.isCmdEmpty());

    for(unsigned i=0; i<256; i+=1) {
        std::cout << i << std::endl;
        fe->setPulserDelay(i);
        usleep(500);
        fe->injectAndTrigger();
        std::string trash;
        std::cin >> trash; 
    }
    return 0;
}
