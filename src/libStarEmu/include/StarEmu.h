#ifndef __STAR_EMU_H__
#define __STAR_EMU_H__

#include "StarChipsetEmu.h"
#include "EmuCom.h"
#include <atomic>


/**
 * Emulation of data returned by HCCStars in one channel
 */
class StarEmu {
public:

    StarEmu(std::vector<ClipBoard<RawData>*> &rx, EmuCom * tx, EmuCom * tx2,
            const std::string& json_emu_file_path,
            const std::vector<std::string>& json_chip_file_path,
            unsigned hpr_period, int abc_version, int hcc_version);
    ~StarEmu();

    // the main loop which recieves commands from yarr
    void executeLoop();
    
    /** When set (by EmuController) shutdown executeLoop (i.e. the thread) */
    std::atomic<bool> run{};
    
private:

    /// Decode LCB command
    void decodeLCB(LCB::Frame);

    /// Decode R3L1 command
    void decodeR3L1(uint16_t);

    ///
    void doL0A(bool bcr, uint8_t l0a_mask, uint8_t l0a_tag) {
        for (auto& emu : chipEmus) emu->doL0A(bcr, l0a_mask, l0a_tag);
    }

    void doRegReadWrite(LCB::Frame frame) {
        for (auto& emu : chipEmus) emu->doRegReadWrite(frame);
    }

    void doFastCommand(uint8_t data6b) {
        for (auto& emu : chipEmus) emu->doFastCommand(data6b);
    }

    void doHPR(LCB::Frame frame) {
        for (auto& emu : chipEmus) emu->doHPR(frame);
    }

    void doPRLP(uint8_t mask, uint8_t l0tag) {
        for (auto& emu : chipEmus) emu->doPRLP(mask, l0tag);
    }

    // BC counter
    void updateBC() {
        if (m_resetbc) {
            m_bccnt = 0;
            m_resetbc = false;
        } else {
            // Increment BC counter
            m_bccnt += 4;
        }

        for (auto& emu : chipEmus) emu->setBC(m_bccnt);
    }

    // L0 buffer
    void fillL0Buffer() {
        for (auto& emu : chipEmus) emu->fillL0Buffer();
    }

    ////////////////////////////////////////
    /** These are ring buffers owned by EmuController */
    EmuCom * m_txRingBuffer;  // for LCB
    EmuCom * m_txRingBuffer2; // for R3L1

    uint16_t m_bccnt;
    bool m_resetbc;

    std::vector< std::unique_ptr<StarChipsetEmu> > chipEmus;
};

#endif //__STAR_EMU_H__
