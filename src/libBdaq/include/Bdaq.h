#ifndef BDAQ53_H
#define BDAQ53_H

#include <string>
#include <map>

#include "BdaqRBCP.h"
#include "Bdaq_i2c.h"
#include "BdaqSi570.h"
#include "BdaqAuroraRx.h"
#include "BdaqDriver.h"
#include "BdaqTCP.h"
#include "BdaqSiTcpFifo.h"
#include "BdaqGPIO.h"

struct bdaqConfig {
    std::string ipAddr;
    uint        udpPort;
    uint        tcpPort;
    bool        configSi570;
    uint        fifoAddr;
    uint        i2cAddr;
    uint        controlAddr;
    uint        rxAddr;
    uint        cmdAddr;
    std::string feType;
};

struct daqVersion {
    std::string fwVersion;
    std::string boardVersion;
    int         numRxChannels;
    int         boardOptions;
    std::string connectorVersion;
};

class Bdaq {
	public:
        const std::string VERSION = "1.8"; // Must match FW version.

        BdaqRBCP rbcp;
        Bdaq_i2c i2c;
        BdaqSi570 si570;
        std::vector<BdaqAuroraRx> rx;
        BdaqDriver cmd;
        BdaqTCP tcp;
        BdaqSiTcpFifo fifo;
        BdaqGPIO bdaqControl;

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

        Bdaq();
		~Bdaq() = default;

        void initialize(bdaqConfig c);
		daqVersion getDaqVersion();
        bool waitForPllLock(uint timeout=1000);
        void setupAurora();
        void setChipTypeRD53A();
        void setChipTypeITkPixV1();
        void enableAutoSync();
        void disableAutoSync();
        void setMonitorFilter(BdaqAuroraRx::userkFilterMode mode);
        int  chipType;

	private:
        daqVersion dv;
};

#endif
