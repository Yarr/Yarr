
#include <iostream>

#include "SpecController.h"
#include "Fe65p2.h"
#include "Bookkeeper.h"

int main(void) {

    std::cout << "Initialising .." << std::endl;
    SpecController spec;
    spec.init(0);
    Bookkeeper bookie(&spec, &spec);
    bookie.initGlobalFe(new Fe65p2(&spec));

    Fe65p2 *fe = bookie.globalFe<Fe65p2>();

    std::cout << "Configuring FE .." << std::endl;
    spec.setCmdEnable(0x1);
    
    fe->setPlsrDac(1000);
    fe->enAnaInj();
    fe->clocksOn();
    while(!spec.isCmdEmpty());

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
