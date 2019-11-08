#ifndef __STAR_EMU_H__
#define __STAR_EMU_H__

#include "StarChips.h"

#include <atomic>
#include <memory>
#include <queue>

#include <cstdint>
#include <iterator>

class EmuCom;

/**
 * Emulation of data returned by HCCStar.
 */
class StarEmu {
public:
    /** These are ring buffers are owned by EmuController */
    StarEmu(ClipBoard<RawData> &rx, EmuCom * tx, std::string json_file_path);
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
    
    void DecodeLCB(LCB::Frame);
    
    void doL0A(uint16_t);
    void doFastCommand(uint8_t);
    void doRegReadWrite();
    
    EmuCom * m_txRingBuffer;
    ClipBoard<RawData> &m_rxQueue;

    /** log level control */
    bool verbose  { false };

    // buffer for register read/write command sequence
    std::queue<uint32_t> reg_cmd_buffer;
    
    ////////////////////////////////////////
    // HCCStar and ABCStar registers for now
    // They will be replaced by StarCfg eventually
    
    
};

#endif //__STAR_EMU_H__
