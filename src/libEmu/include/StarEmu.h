#ifndef __STAR_EMU_H__
#define __STAR_EMU_H__

#include "StarChips.h"
#include "StarEmuCfg.h"

#include <atomic>
#include <memory>
#include <queue>

#include <cstdint>
#include <iterator>
#include <algorithm>

class EmuCom;

namespace emu { class StarCfg; }

/**
 * Emulation of data returned by HCCStar.
 */
class StarEmu {
public:

    enum class PacketTypes {
        PR = 1, LP = 2, ABCRegRd = 4, ABCPacketTransRegRd = 7, HCCRegRd = 8,
        ABCFullTransRegRd = 11, ABCHPR = 13, HCCHPR = 14
    };
    
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
        sendPacket(&(*std::begin(iterable)), &(*std::end(iterable)));
    }

    /// Send response packet (excluding SOP/EOP)
    void sendPacket(uint8_t *byte_s, uint8_t *byte_e);

    /// Build data packet
    std::vector<uint8_t> buildPhysicsPacket(PacketTypes, uint8_t, uint8_t,
                                           uint16_t endOfPacket=0x6fed);
    std::vector<uint8_t> buildABCRegisterPacket(PacketTypes, uint8_t, uint8_t,
                                               unsigned, uint16_t);
    std::vector<uint8_t> buildHCCRegisterPacket(PacketTypes, uint8_t, unsigned);

    ///
    void DecodeLCB(LCB::Frame);
    unsigned int countTriggers(LCB::Frame);
    
    void doL0A(uint16_t);
    void doFastCommand(uint8_t);
    void doRegReadWrite(LCB::Frame);
    void execute_command_sequence();
    void writeRegister(const uint32_t, const uint8_t, bool isABC=false,
                       const unsigned ABCID=0);
    void readRegister(const uint8_t, bool isABC=false, const unsigned ABCID=0);

    void getClusters();

    uint16_t clusterFinder_sub(uint64_t&, uint64_t&, bool);
    std::vector<uint16_t> clusterFinder(const std::array<unsigned,8>&,
                                        const uint8_t maxCluster = 64);
    std::array<unsigned, 8> getFrontEndData(unsigned int, uint8_t bc_index=0);
    
    // Utilities
    bool getParity_8bits(uint8_t);
    bool getBit_128b(uint8_t, uint64_t, uint64_t);
    void setBit_128b(uint8_t, bool, uint64_t&, uint64_t&);
    
    ////////////////////////////////////////
    EmuCom * m_txRingBuffer;
    ClipBoard<RawData> &m_rxQueue;

    /** log level control */
    bool verbose  { false };
    
    ////////////////////////////////////////
    // Internal states
    
    // For register command sequence
    bool m_ignoreCmd;
    bool m_isForABC;
    
    // buffer for register read/write command sequence
    std::queue<uint8_t> m_reg_cmd_buffer;
    
    // cluster container for input channels
    std::vector<std::vector<uint16_t>> m_clusters; // filled in getClusters()
    
    // BC counter
    uint8_t m_bccnt;

    ////////////////////////////////////////
    // HCCStar and ABCStar configurations
    std::unique_ptr<emu::StarCfg> m_starCfg;

    bool debug = false;
};

#endif //__STAR_EMU_H__
