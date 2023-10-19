#ifndef SPECCOM_H
#define SPECCOM_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: SPEC cpp library
// # Comment: Original driver taken from Marcus Guillermo
// #          Modified for SPEC card
// ################################

#include <cstdint>
#include <string>

#include "storage.hpp"
#include <SpecDevice.h>
#include <KernelMemory.h>
#include <UserMemory.h>
#include <Exception.h>

#define ARRAYLENGTH 252

template<typename TYPE, std::size_t SIZE>
std::size_t array_length(const TYPE (&)[SIZE]){return SIZE;}

enum SPEC_DMA_REG {
    DMACTRLR = 0x0,
    DMASTATR = 0x1,
    DMACSTARTR = 0x2,
    DMAHSTARTLR = 0x3,
    DMAHSTARTHR = 0x4,
    DMALENR = 0x5,
    DMAHNEXTLR = 0x6,
    DMAHNEXTHR = 0x7,
    DMAATTRIBR = 0x8
};

enum SPEC_DMA_STATUS {
    DMAIDLE = 0x0,
    DMADONE = 0x1,
    DMABUSY = 0x2,
    DMAERROR = 0x3,
    DMAABORTED = 0x4
};

#define SPEC_GREG (0x7 << 14)
#define SPEC_GREG_LPMEN 0x0
#define SPEC_GREG_LPMFREQ 0x0
#define SPEC_GREG_FWVERS 0x6
#define SPEC_GREG_FWIDENT 0x7

const static std::string specIdentHw[] = {"undefined", "Trenz TEF1001_R1", "Trenz TEF1001_R2",
    "PLDA XpressK7 160", "PLDA XpressK7 325", "Xilinx KC705", "CERN SPEC S6"};
const static std::string specIdentChip[] = {"undefined", "FE-I4", "FE65-P2", "RD53A/B", "ABC/HCCStar"};
const static std::string specIdentFmc[] = {"undefined", "Creotech 32Ch LVDS (VHDCI)", "Ohio Card (Display Port)"};
const static std::string specIdentSpeed[] = {"undefined", "160Mbps", "320Mbps", "640Mbps", "1280Mbps"};
const static std::string specIdentChCfg[] = {"undefined", "4x4", "16x1", "8x4", "32x1", "3x4 TLU", "3x4 Ext Trig", "12x1 TLU", "12x1 Ext Trig"};
const static uint32_t specIdentLaneCfg[] = {0, 4, 1, 4, 1, 4, 4, 1, 1};

static std::map<uint16_t, std::string> specIdentFw = {{0x4256c32, "v1.3.1"},{0x2779a56, "v1.3"}, {0x4d9ff6d, "v1.2.1"}, {0x1493b73, "v1.1.1"}};

struct dma_linked_list {
    uint32_t carrier_start;
    uint32_t host_start_l;
    uint32_t host_start_h;
    uint32_t length;
    uint32_t host_next_l;
    uint32_t host_next_h;
    uint32_t attr;
};


using namespace specDriver;

class SpecCom {
    public:
        SpecCom();
        SpecCom(unsigned int id);
        ~SpecCom();

        bool isInitialized() const;
        int getId() const;
        int getBarSize(unsigned int bar);

        void init(unsigned int id);
        const json getStatus();

        void writeSingle(uint32_t off, uint32_t val);
        uint32_t readSingle(uint32_t off);

        void write32(uint32_t off, uint32_t *val, size_t words = 1);
        void read32(uint32_t off, uint32_t *val, size_t words = 1);

        void writeBlock(uint32_t off, uint32_t *val, size_t words);
        void readBlock(uint32_t off, uint32_t *val, size_t words);

        int writeDma(uint32_t off, uint32_t *data, size_t words);
        int readDma(uint32_t off, uint32_t *data, size_t words);

        int progFpga(const void *data, size_t size);
        uint32_t readEeprom(uint8_t * buffer, uint32_t len);
        uint32_t writeEeprom(uint8_t * buffer, uint32_t len, uint32_t offs);
        void createSbeFile(std::string fnKeyword, uint8_t * buffer, uint32_t length);
        void getSbeFile(std::string pathname, uint8_t * buffer, uint32_t length);
    protected:
        void flushDma();

        uint32_t fw_vers;
        uint32_t fw_ident;

        std::string getSpecIdentHw   (uint32_t fw_ident);
        std::string getSpecIdentChip (uint32_t fw_ident);
        std::string getSpecIdentFmc  (uint32_t fw_ident);
        std::string getSpecIdentSpeed(uint32_t fw_ident);
        std::string getSpecIdentChCfg(uint32_t fw_ident);
        uint32_t getSpecIdentLaneCfg(uint32_t fw_ident);
   
    private:
        unsigned int specId;
        bool is_initialized;
        SpecDevice *spec;
        void *bar0, *bar4;

        void init();
        void configure();

        void write32(void *bar, uint32_t off, uint32_t val);
        uint32_t read32(void *bar, uint32_t off);
        void mask32(void *bar, uint32_t off, uint32_t mask, uint32_t val);

        void write32(void *bar, uint32_t off, uint32_t *val, size_t words);
        void read32(void *bar, uint32_t off, uint32_t *val, size_t words);

        void writeBlock(void *bar, uint32_t off, uint32_t *val, size_t words);
        void readBlock(void *bar, uint32_t off, uint32_t *val, size_t words);

        struct dma_linked_list* prepDmaList(UserMemory *um, KernelMemory *km, uint32_t off, bool write);
        void startDma();
        void abortDma();
        uint32_t getDmaStatus();
};
#endif
