#ifndef BOCCOM_H
#define BOCCOM_H

// #################################
// # Author: Marius Wensing
// # Email: marius.wensing at cern.ch
// # Project: Yarr
// # Description: SPEC cpp library
// # Comment: 
// ################################

#include <stdint.h>
#include <string>
#include <mutex>

// FPGA register offsets
#define BCF_OFFSET          0x0000
#define BMFS_OFFSET         0x4000
#define BMFN_OFFSET         0x8000

// BCF registers
#define BCF_REVISION        0x25

// BMF registers
#define BMF_FEATURE_LOW     0x14
#define BMF_FEATURE_HIGH    0x15
#define BMF_TRIG_CTRL       0x1C
#define BMF_TRIG_STATUS     0x1D
#define BMF_TRIG_WORD       0x1E
#define BMF_TRIG_CONFIG     0x1F
#define BMF_TXBROADCAST0    0x34
#define BMF_TXBROADCAST1    0x35
#define BMF_RXBROADCAST0    0x36
#define BMF_RXBROADCAST1    0x37

// emulator
#define BMF_EMU_OFFSET      0x80
#define BMF_EMU_CTRL        0x0
#define BMF_EMU_STATUS      0x1
#define BMF_EMU_EXTRA       0x2
#define BMF_EMU_CTRL2       0x3
#define BMF_EMU_HIT         0x6
#define BMF_EMU_HITCNT      0x7

// TX registers
#define BMF_TX_OFFSET       0xC00
#define BMF_TX_CTRL         0x0
#define BMF_TX_STATUS       0x1
#define BMF_TX_FIFO_CTRL    0x2
#define BMF_TX_FIFO_DATA    0x3
#define BMF_TX_CTRL2        0x4

// RX register
#define BMF_RX_OFFSET       0x800
#define BMF_RX_CTRL         0x0
#define BMF_RX_STATUS       0x1
#define BMF_RX_DATA_HIGH    0x2
#define BMF_RX_DATA_LOW     0x3
#define BMF_RX_DCNT_LOW     0x10
#define BMF_RX_DCNT_HIGH    0x11

class BocCom {
    public:
        BocCom(const std::string &host = "192.168.0.10");
        ~BocCom();

        // single read/write
        void writeSingle(uint16_t addr, uint8_t val);
        uint8_t readSingle(uint16_t addr);

        // multiple read/write
        void writeBlock(uint16_t addr, uint8_t *val, size_t words, bool incrementing);
        void readBlock(uint16_t addr, uint8_t *val, size_t words, bool incrementing);

        // 16-bit read (useful for RX FIFO)
        void read16(uint16_t high_addr, uint16_t low_addr, uint16_t *val, size_t words);

        // incrementing read/write
        void writeInc(uint16_t addr, uint8_t *val, size_t words);
        void readInc(uint16_t addr, uint8_t *val, size_t words);

        // non incrementing reads
        void writeNonInc(uint16_t addr, uint8_t *val, size_t words);
        void readNonInc(uint16_t addr, uint8_t *val, size_t words);

        // some helper functions
        uint8_t getBocRev();
        uint16_t getBmfFeatures(uint16_t bmf);

    private:
        // IPBus header
        struct IPBusHeader {
            unsigned int vers     :  4;
            unsigned int trans_id : 11;
            unsigned int words    :  9;
            unsigned int type     :  5;
            unsigned int d        :  1;
            unsigned int res      :  2;
        };

        static void header2bytes(IPBusHeader *header, uint8_t *buf);
        static void bytes2header(IPBusHeader *header, uint8_t* buf);
        static void print_header(IPBusHeader *header);

        int socketfd;
        uint32_t transaction_id;

        std::mutex socket_mutex;
};

#endif
