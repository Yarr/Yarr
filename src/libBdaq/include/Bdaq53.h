#ifndef BDAQ53_H
#define BDAQ53_H

#include <string>
#include <map>

#include "BdaqRBCP.h"
#include "Bdaq_i2c.h"
#include "BdaqSi570.h"
#include "BdaqAuroraRx.h"
#include "BdaqCmdRd53.h"
#include "BdaqTCP.h"
#include "BdaqSiTcpFifo.h"
#include "BdaqGPIO.h"

struct bdaqConfig {
    std::string ipAddr;
    uint        udpPort;
    uint        tcpPort;
    uint        fifoAddr;
    uint        i2cAddr;
    uint        gpioAddr;
    uint        rxAddr;
    uint        cmdAddr;
};

struct daqVersion {
    std::string fwVersion;
    std::string boardVersion;
    int         boardOptions;
    std::string connectorVersion;
    int         rxLanes;
    int         rxChannels; 
};

class Bdaq53 {
	public:
        const std::string VERSION = "0.11"; // Must match FW version.

        BdaqRBCP rbcp;
        Bdaq_i2c i2c;
        BdaqSi570 si570;
        BdaqAuroraRx auroraRx;
        BdaqCmdRd53 cmd;
        BdaqTCP tcp;
        BdaqSiTcpFifo fifo;
        BdaqGPIO dpControl;
        
        std::map<int, std::string> hwMap = {
            {0, "SIMULATION"},
            {1, "BDAQ53"},
            {2, "USBPix3"},
            {3, "KC705"},
            {4, "GENESYS 2"}
        };

        std::map<int, std::string> hwConMap = {
            {0, "SMA"},
            {1, "FMC_LPC"},
            {2, "FMC_HPC"},
            {3, "Displayport"},
            {4, "Cocotb"}
        };
        
        std::map<int, std::string> boardOptionsMap = {
            {0x01, "640Mbps"}
        };
        
        Bdaq53();
		~Bdaq53() {}
        
        void initialize(bdaqConfig c);
		daqVersion getDaqVersion();
        bool waitForPllLock(uint timeout=1000);
        void setupAurora();
                
	private:
        daqVersion dv;
};

#endif
