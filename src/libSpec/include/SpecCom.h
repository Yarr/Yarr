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
#include "SpecCaps.h"

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

#define SPEC_GREG_SOFTRST 0xF

// Read-only DMA statistics of the control block (firmware with TLP and cycle counters)
#define SPEC_GREG_FEATURES 0x8       // bit 0: TLP counters, bit 1: cycle counters
#define SPEC_GREG_CAPS 0x1E          // capabilities, see SpecCaps.h
#define SPEC_GREG_STAT_TLP_128 0xA   // device-to-host TLPs with 128 B payload
#define SPEC_GREG_STAT_TLP_256 0xB
#define SPEC_GREG_STAT_TLP_512 0xC
#define SPEC_GREG_STAT_TLP_OTHER 0xD // shorter remainder TLPs
#define SPEC_GREG_STAT_BUSY 0x10     // cycles with an active device-to-host transfer
#define SPEC_GREG_STAT_STARVED 0x11  // ... waiting for data from the memory side
#define SPEC_GREG_STAT_BACKPRESS 0x12 // ... waiting for the PCIe side
#define SPEC_GREG_STAT_HDRWAIT 0x13  // ... waiting at a TLP header

#define RESET_OPTION_BRAM_CNT 0x1
#define RESET_OPTION_WSHEXP_CORE 0x2
#define RESET_OPTION_TX_CORE 0x3
#define RESET_OPTION_RX_CORE 0x4
#define RESET_OPTION_RX_BRIDGE 0x5
#define RESET_OPTION_TRIG_LOGIC 0x6
#define RESET_OPTION_SPI 0x7
#define RESET_OPTION_CTRL_REG 0x8
#define RESET_OPTION_BRAM 0x9
#define RESET_OPTION_EXCEPT_CTRL_REG 0xE
#define RESET_OPTION_ALL 0xF

#define SOFTRST_WSHEXP_CORE 0x00000001
#define SOFTRST_TX_CORE 0x00000002
#define SOFTRST_RX_CORE 0x00000004
#define SOFTRST_RX_BRIDGE 0x00000008
#define SOFTRST_TRIGGER_LOGIC 0x00000020
#define SOFTRST_SPI 0x00000040
#define SOFTRST_CTRL_REG 0x00000080
#define SOFTRST_BRAM 0x00000100
#define SOFTRST_BRAM_CNT 0x00000200
#define SOFTRST_ALL 0x000003FF
#define SOFTRST_EXCEPT_CTRL_REG 0x0000037F

const static std::string specIdentHw[] = {"undefined", "Trenz TEF1001_R1", "Trenz TEF1001_R2",
    "PLDA XpressK7 160", "PLDA XpressK7 325", "Xilinx KC705", "Numato Nereid K7-160"};
const static std::string specIdentChip[] = {"undefined", "FE-I4", "FE65-P2", "RD53A/B/C", "ABC/HCCStar"};
const static std::string specIdentFmc[] = {"undefined", "Creotech 32Ch LVDS (VHDCI)", "Ohio Card (Display Port)"};
const static std::string specIdentSpeed[] = {"undefined", "160Mbps", "320Mbps", "640Mbps", "1280Mbps"};
const static std::string specIdentChCfg[] = {"undefined", "4x4", "16x1", "8x4", "32x1", "3x4 TLU", "3x4 Ext Trig", "12x1 TLU", "12x1 Ext Trig"};
const static uint32_t specIdentLaneCfg[] = {0, 4, 1, 4, 1, 4, 4, 1, 1};

static std::map<uint32_t, std::string> specIdentFw = {
    {0xe38fcb2, "v1.7.1"},
    {0xf70bc8d, "v1.7.0"},
    {0xacb87f4, "v1.6.0"},
    {0x7b14549, "v1.5.0"},
    {0xe7985d6, "v1.4.0"},
    {0x4256c32, "v1.3.1"},
    {0x2779a56, "v1.3"},
    {0x4d9ff6d, "v1.2.1"},
    {0x1493b73, "v1.1.1"}};

/// Free-running counters of the device-to-host DMA master in the firmware. Take the difference
/// of two snapshots to get the statistics of the transfers in between.
struct DmaStats {
    bool has_tlp = false;     ///< firmware has the TLP counters
    bool has_cycles = false;  ///< firmware has the cycle counters
    uint32_t tlp_128 = 0, tlp_256 = 0, tlp_512 = 0, tlp_other = 0;
    uint32_t busy = 0, starved = 0, backpressure = 0, header_wait = 0;  ///< wb_clk cycles

    DmaStats operator-(const DmaStats &o) const {
        DmaStats d = *this;
        d.tlp_128 -= o.tlp_128; d.tlp_256 -= o.tlp_256; d.tlp_512 -= o.tlp_512; d.tlp_other -= o.tlp_other;
        d.busy -= o.busy; d.starved -= o.starved; d.backpressure -= o.backpressure; d.header_wait -= o.header_wait;
        return d;
    }
};

/// Words of a descriptor that the firmware reads (and that are written to the DMA registers for the first one)
constexpr size_t kDmaDescWords = 7;

/// One DMA descriptor. The card fetches the next descriptor from host memory with one read of 7 words. A PCIe memory
/// read must not cross a 4 KB boundary, and a host root complex treats one that does as a malformed TLP (fatal error,
/// the DMA never finishes). Descriptors of 28 bytes in a row cross a page at the 147th one, so each is padded to
/// 32 bytes: 128 fit in a page, none crosses it (the list starts on a page boundary).
struct dma_linked_list {
    uint32_t carrier_start;
    uint32_t host_start_l;
    uint32_t host_start_h;
    uint32_t length;
    uint32_t host_next_l;
    uint32_t host_next_h;
    uint32_t attr;
    uint32_t pad;
};
static_assert(sizeof(dma_linked_list) == 32, "descriptors must be 32 bytes so that none crosses a 4 KB boundary");
static_assert(sizeof(dma_linked_list) >= kDmaDescWords * sizeof(uint32_t), "descriptor smaller than what the firmware reads");


using namespace specDriver;

class SpecCom {
    public:
        SpecCom();
        SpecCom(unsigned int id, bool do_reset_arg=true);
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

        /// Snapshot of the DMA statistics counters (see DmaStats)
        DmaStats getDmaStats();

        /// Merge physically contiguous pages of device-to-host buffers into descriptors of up to 32 KB. Fewer
        /// descriptors mean fewer gaps between them; false gives one descriptor per page. Default: automatic, merged
        /// only if the firmware advertises (capability register) that it keeps all data with long DMA bursts. Older
        /// firmware loses data with descriptors larger than a few KB and can crash the host. This call forces the
        /// setting; forcing it on with firmware without the capability logs a warning.
        void setMergeDmaRead(bool merge);
        /// Back to automatic
        void setMergeDmaReadAuto() { merge_mode = MergeMode::Auto; }
        /// Merged descriptors are used now (the setting and what the firmware can do)
        bool getMergeDmaRead() const { return mergedReadsEnabled(merge_mode, fw_caps); }
        MergeMode getMergeMode() const { return merge_mode; }
        const SpecCaps &getFirmwareCaps() const { return fw_caps; }
        /// Largest merged device-to-host descriptor in bytes (default and maximum 32768). Smaller values keep the
        /// bursts shorter; the value is rounded down to a multiple of 4096 and clamped to 4096..32768.
        void setMergedDescBytes(uint32_t bytes);
        uint32_t getMergedDescBytes() const { return merged_desc_bytes; }
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
        unsigned int specId = 0;
        bool is_initialized = false;
        bool do_reset = true;
        MergeMode merge_mode = MergeMode::Auto;
        SpecCaps fw_caps;
        uint32_t merged_desc_bytes = 32768;

        /// Largest descriptor for a DMA in the given direction
        uint32_t descLimit(bool write) const;
        SpecDevice *spec = nullptr;
        void *bar0 = nullptr, *bar4 = nullptr;

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
