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

        void loadConfig(json &j) override {
            bdaqConfig c;
            // IP Address
            if (!j["ipAddr"].empty())
                c.ipAddr = j["ipAddr"];
            else
                c.ipAddr = "192.168.10.12";
            // UDP Port
            if (!j["udpPort"].empty())
                c.udpPort = j["udpPort"];
            else
                c.udpPort = 4660;
            // TCP Port
            if (!j["tcpPort"].empty())
                c.tcpPort = j["tcpPort"];
            else
                c.tcpPort = 24;
            // RX Wait Time in milliseconds (for last data chunk in StdDataLoop/StdDataGatherer)
            if (!j["rxWaitTime"].empty())
                rxWaitTime = std::chrono::microseconds(uint(j["rxWaitTime"])*1000);
            else
                rxWaitTime = std::chrono::microseconds(10*1000); // converting from ms to us.            
            // Software AZ for Sycnhronous FE
            if (!j["softwareAZ"].empty())
                softwareAZ = j["softwareAZ"];
            else
                softwareAZ = true;
            // RX Module Address
            if (!j["rxAddr"].empty())
                c.rxAddr = std::stoi(j["rxAddr"], nullptr, 16);
            else
                c.rxAddr = 0x6000;
            // 2c Module Address
            if (!j["i2cAddr"].empty())
                c.i2cAddr = std::stoi(j["i2cAddr"], nullptr, 16);
            else
                c.i2cAddr = 0x1000;
            // cmd_rd53 Module Address
            if (!j["cmdAddr"].empty())
                c.cmdAddr = std::stoi(j["cmdAddr"], nullptr, 16);
            else
                c.cmdAddr = 0x9000;
            // GPIO Module Address
            if (!j["gpioAddr"].empty())
                c.controlAddr = std::stoi(j["controlAddr"], nullptr, 16);
            else
                c.controlAddr = 0x2100;
            // Initialize controller with the above configuration
            initialize(c);
        }
        
        void setupMode() override final {
            BdaqTxCore::m_softwareAZ = softwareAZ;
        }

        void runMode() override final {
            BdaqRxCore::m_waitTime = rxWaitTime;
        }

    private:
        std::chrono::microseconds rxWaitTime;
        bool softwareAZ;
};

#endif
