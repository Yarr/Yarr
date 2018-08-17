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

#include <stdint.h>
#include <string>

#include <SpecDevice.h>
#include <KernelMemory.h>
#include <UserMemory.h>
#include <Exception.h>

#define ARRAYLENGTH 252

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

        bool isInitialized();
        int getId();
        int getBarSize(unsigned int bar);

        void init(unsigned int id);

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
