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
#include "Bdaq.h"
#include "BdaqTxCore.h"
#include "BdaqRxCore.h"

#include "storage.hpp"

class BdaqController : public HwController, public BdaqTxCore, public BdaqRxCore {
    public:

        void loadConfig(const json &j) override {
            bdaqConfig c;
            // IP Address
            if (j.contains("ipAddr"))
                c.ipAddr = j["ipAddr"];
            else
                c.ipAddr = "192.168.10.12";
            // UDP Port
            if (j.contains("udpPort"))
                c.udpPort = j["udpPort"];
            else
                c.udpPort = 4660;
            // TCP Port
            if (j.contains("tcpPort"))
                c.tcpPort = j["tcpPort"];
            else
                c.tcpPort = 24;
            // RX Wait Time in milliseconds (for last data chunk in StdDataLoop/StdDataGatherer)
            if (j.contains("rxWaitTime"))
                rxWaitTime = std::chrono::microseconds(uint(j["rxWaitTime"])*1000);
            else
                rxWaitTime = std::chrono::microseconds(10*1000); // converting from ms to us.            
            // Configure Si570 oscillator (MGT reference clock)
            if (j.contains("configSi570"))
                c.configSi570 = j["configSi570"];
            else
                c.configSi570 = true;
            // Software AZ for Sycnhronous FE
            if (j.contains("softwareAZ"))
                softwareAZ = j["softwareAZ"];
            else
                softwareAZ = true;
            // RX Module Address
            if (j.contains("rxAddr"))
                c.rxAddr = std::stoi(j["rxAddr"], nullptr, 16);
            else
                c.rxAddr = 0x6000;
            // 2c Module Address
            if (j.contains("i2cAddr"))
                c.i2cAddr = std::stoi(j["i2cAddr"], nullptr, 16);
            else
                c.i2cAddr = 0x1000;
            // cmd_rd53 Module Address
            if (j.contains("cmdAddr"))
                c.cmdAddr = std::stoi(j["cmdAddr"], nullptr, 16);
            else
                c.cmdAddr = 0x9000;
            // GPIO Module Address
            if (j.contains("gpioAddr"))
                c.controlAddr = std::stoi(j["controlAddr"], nullptr, 16);
            else
                c.controlAddr = 0x2100;
            // Chip Type
            if (j.contains("chipType")){
                c.feType = j["chipType"];
            }else{
                c.feType = "none";
            }


            // Initialize controller with the above configuration
            initialize(c);
        }
        
        void setupMode() override {
            BdaqTxCore::m_softwareAZ = softwareAZ;
            BdaqRxCore::setupMode();
        }

        void runMode() override {
            BdaqRxCore::m_waitTime = rxWaitTime;
            BdaqRxCore::runMode();
        }

    private:
        std::chrono::microseconds rxWaitTime;
        bool softwareAZ;
};

#endif
