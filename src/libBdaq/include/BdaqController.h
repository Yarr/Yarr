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
#include "json.hpp"

using json=nlohmann::basic_json<std::map, std::vector, std::string, bool, std::int32_t, std::uint32_t, float>;

class BdaqController : public HwController, public BdaqTxCore, public BdaqRxCore {
    public:
        BdaqController() {} 
        
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
            initialize(c);
            
        }        
    private:
};

#endif
