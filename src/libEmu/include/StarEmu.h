#ifndef __STAR_EMU_H__
#define __STAR_EMU_H__

#include "StarChips.h"

#include <atomic>
#include <memory>

#include <cstdint>
#include <iterator>

class EmuCom;

/**
 * Emulation of data returned by HCCStar.
 */
class StarEmu {
public:
    /** These are ring buffers are owned by EmuController */
    StarEmu(EmuCom * rx, EmuCom * tx, std::string json_file_path);
    ~StarEmu();
    
    // the main loop which recieves commands from yarr
    void executeLoop();
    
    /** When set (by EmuController) shutdown executeLoop (i.e. the thread) */
    std::atomic<bool> run;
    
private:

    /// Send response packet (excluding SOP/EOP)
    template<typename T> void sendPacket(T &iterable) {
      sendPacket(std::begin(iterable), std::end(iterable));
    }

    /// Send response packet (excluding SOP/EOP)
    void sendPacket(uint8_t *byte_s, uint8_t *byte_e);

    EmuCom * m_txRingBuffer;
    EmuCom * m_rxRingBuffer;

    /** log level control */
    bool verbose  { false };
};

#endif //__STAR_EMU_H__
