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
    void doRegReadWrite(LCB::Frame);

    void writeRegister(const uint32_t, const uint8_t, bool isABC=false,
                       const unsigned ABCID=0);
    void readRegister(const uint8_t, bool isABC=false, const unsigned ABCID=0);
    
    EmuCom * m_txRingBuffer;
    ClipBoard<RawData> &m_rxQueue;

    /** log level control */
    bool verbose  { false };

    ////////////////////////////////////////
    // HCCStar and ABCStar registers (for now)
    // These should be replaced by StarCfg eventually
    unsigned int m_HCCID;
    std::vector<unsigned int> m_ABCIDs;
    unsigned int m_nABCs;
    
    ////////////////////////////////////////
    // Internal states

    // For register command sequence
    bool ignoreCmd;
    bool isForABC;

    // buffer for register read/write command sequence
    std::queue<uint8_t> reg_cmd_buffer;
    
};

#endif //__STAR_EMU_H__
