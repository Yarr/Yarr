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
    
    void doL0A(uint16_t);
    void doFastCommand(uint8_t);
    void doRegReadWrite(LCB::Frame);
    void execute_command_sequence();
    void writeRegister(const uint32_t, const uint8_t, bool isABC=false,
                       const unsigned ABCID=0);
    void readRegister(const uint8_t, bool isABC=false, const unsigned ABCID=0);

    void getClusters(int);

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
    // HCCStar and ABCStar registers (for now)
    // These should be replaced by StarCfg eventually
    unsigned int m_HCCID;
    std::vector<unsigned int> m_ABCIDs;
    unsigned int m_nABCs;

    // Mask/Input registers
    std::vector<unsigned int> _MaskInput0; // 0x10: ch31 - ch0
    std::vector<unsigned int> _MaskInput1; // 0x11: ch63 - ch32
    std::vector<unsigned int> _MaskInput2; // 0x12: ch95 - ch64
    std::vector<unsigned int> _MaskInput3; // 0x13: ch127 - ch96
    std::vector<unsigned int> _MaskInput4; // 0x14: ch159 - ch128
    std::vector<unsigned int> _MaskInput5; // 0x15: ch191 - ch160
    std::vector<unsigned int> _MaskInput6; // 0x16: ch223 - ch192
    std::vector<unsigned int> _MaskInput7; // 0x17: ch255 - ch224

    // Test mode and Test pattern
    // configuration register 0x20
    std::vector<uint8_t> _TM; // [17:16] of register 0x20
    std::vector<uint8_t> _TestPatt1; // [23:20] of register 0x20
    std::vector<uint8_t> _TestPatt2; // [27:24] of register 0x20
    std::vector<bool> _TestPattEnable; // [18] of register 0x20
    std::vector<bool> _TestPulseEnable; // [4] of register 0x20
};

#endif //__STAR_EMU_H__
