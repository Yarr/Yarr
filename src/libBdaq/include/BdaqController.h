#ifndef BDAQCONTROLLER_H
#define BDAQCONTROLLER_H

// #################################
// # Author: Rafael Gama
// # Email: rafael.gama at cern.ch
// # Project: Yarr
// # Description:
// # Comment:
// ################################

#include "HwController.h"
#include "Bdaq53.h"
#include "BdaqTxCore.h"
#include "BdaqRxCore.h"


#include "storage.hpp"

class BdaqController : public HwController, public BdaqTxCore, public BdaqRxCore {
    public:
        //BdaqController() {} 
        
        void loadConfig(json &j) override {
            bdaqConfig c;
            //read the parameters below from the JSON config file.
            c.ipAddr   = "192.168.10.12";
            c.udpPort  = 4660;
            c.tcpPort  = 24;
            c.rxAddr   = 0x6000;
            c.i2cAddr  = 0x1000;
            c.cmdAddr  = 0x9000;
            c.gpioAddr = 0x2100;
            waitTime = std::chrono::microseconds(10*1000); //converting from ms to us, integer only.
            initialize(c);
            
        }

        void runMode() override final {
            BdaqRxCore::m_waitTime = waitTime;
        }

    private:
        std::chrono::microseconds waitTime;
};

#endif
