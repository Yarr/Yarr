#ifndef __STAR_EMU_H__
#define __STAR_EMU_H__

#include "StarChips.h"
#include "StarCfg.h"
#include "StripModel.h"

#include <atomic>
#include <memory>
#include <queue>

#include <cstdint>
#include <iterator>
#include <algorithm>
#include <bitset>

class EmuCom;

class StarCfg;

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
    StarEmu(ClipBoard<RawData> &rx, EmuCom * tx, std::string& json_emu_file_path,
            std::string& json_chip_file_path, unsigned);
    ~StarEmu();

    // the main loop which recieves commands from yarr
    void executeLoop();
    
    /** When set (by EmuController) shutdown executeLoop (i.e. the thread) */
    std::atomic<bool> run;
    
private:

    // FE data format
    static constexpr unsigned NStrips = 256;
    using StripData = std::bitset<NStrips>;

    /////////////////////////////////////////////
    /// Send response packet (excluding SOP/EOP)
    template<typename T> void sendPacket(T &iterable) {
        sendPacket(&(*std::begin(iterable)), &(*std::end(iterable)));
    }

    /// Send response packet (excluding SOP/EOP)
    void sendPacket(uint8_t *byte_s, uint8_t *byte_e);

    /// Build data packet
    std::vector<uint8_t> buildPhysicsPacket(
        const std::vector<std::vector<uint16_t>>&, PacketTypes, uint8_t, uint8_t,
        uint16_t endOfPacket=0x6fed);
    std::vector<uint8_t> buildABCRegisterPacket(PacketTypes, uint8_t, uint8_t,
                                               unsigned, uint16_t);
    std::vector<uint8_t> buildHCCRegisterPacket(PacketTypes, uint8_t, unsigned);

    /// Decode LCB command
    void DecodeLCB(LCB::Frame);

    /// Register R/W commands
    void doRegReadWrite(LCB::Frame);
    void execute_command_sequence();
    void writeRegister(const uint32_t, const uint8_t, bool isABC=false,
                       const unsigned ABCID=0);
    void readRegister(const uint8_t, bool isABC=false, const unsigned ABCID=0);

    /// Fast commands
    void doFastCommand(uint8_t);
    void logicReset();
    void resetABCRegisters();
    void resetABCSEU();
    void resetABCHitCounts();
    void resetSlowCommand();
    void resetHCCRegisters();
    void resetHCCSEU();
    void resetHCCPLL();

    /// HPR
    void setHCCStarHPR(LCB::Frame);
    void setABCStarHPR(LCB::Frame, int);
    void doHPR_HCC(LCB::Frame);
    void doHPR_ABC(LCB::Frame, unsigned);
    void doHPR(LCB::Frame);

    /// Trigger and front end
    void doL0A(uint16_t);
    unsigned int countTriggers(LCB::Frame);
    uint16_t clusterFinder_sub(uint64_t&, uint64_t&, bool);
    std::vector<uint16_t> clusterFinder(const StripData&,
                                        const uint8_t maxCluster=63);
    void ackPulseCmd(int pulseType, uint8_t cmdBC);
    unsigned getL0BufferAddr(unsigned latency, uint8_t cmdBC);
    void clearFEData();

    // per ABC
    void countHits(AbcCfg& abc, const StripData& hits);
    std::vector<uint16_t> getClusters(const AbcCfg&, const StripData&);

    std::pair<uint8_t,StripData> getFEData(const AbcCfg& abc, uint8_t cmdBC);
    std::pair<uint8_t,StripData> generateFEData_StaticTest(const AbcCfg&, unsigned);
    std::pair<uint8_t,StripData> generateFEData_TestPulse(const AbcCfg&, unsigned);
    std::pair<uint8_t,StripData> generateFEData_CaliPulse(const AbcCfg&, unsigned);

    StripData getMasks(const AbcCfg& abc);
    StripData getCalEnables(const AbcCfg& abc);

    /// Utilities
    bool getParity_8bits(uint8_t);
    bool getBit_128b(uint8_t, uint64_t, uint64_t);
    void setBit_128b(uint8_t, bool, uint64_t&, uint64_t&);
    
    ////////////////////////////////////////
    EmuCom * m_txRingBuffer;
    ClipBoard<RawData> &m_rxQueue;
    
    ////////////////////////////////////////
    // Internal states
    
    // For register command sequence
    bool m_ignoreCmd;
    bool m_isForABC;
    
    // buffer for register read/write command sequence
    std::queue<uint8_t> m_reg_cmd_buffer;
    
    // front-end data container
    // For nABCs and *Four* bunch crossings
    // m_l0buffer_lite[iABC][iBC] = 8-bit BC + 256-bit strip hits
    //std::vector< std::array<StripData, 4> > m_l0buffers_lite;

    // Front-end data pipeline
    // Simplified L0 buffer
    // Instead of storing the hit data, it only takes note when a calibration or test pulse command is received.
    // The actual 256b strip data is generated when a trigger is received.
    // Since pulse commands are broadcasted to all chips, there is no need to have separate L0 pipelines for different ABCStar chips
    // 512 deep and each entry is 8b BCID + 2 bits indicating a cal or test pulse
    // 0b01: calibration pulse; 0b10: digital test pulse
    static constexpr unsigned L0BufDepth = 512;
    static constexpr unsigned L0BufWidth = 10;
    std::array<std::bitset<L0BufWidth>, L0BufDepth> m_l0buffer_lite;

    // number of untriggered data in the pipeline
    unsigned int m_ndata_l0buf;

    // BC counter
    uint16_t m_bccnt;

    // Count hits
    bool m_startHitCount;
    uint8_t m_bc_sel;

    // Clock counter for HPR packets
    unsigned hpr_clkcnt; // unit: BC (25 ns)
    // Flag indicating if at least one HPR packet has been sent
    std::vector<bool> hpr_sent;
    const unsigned HPRPERIOD; // HPR packet period
    
    ////////////////////////////////////////
    // HCCStar and ABCStar configurations
    std::unique_ptr<StarCfg> m_starCfg;

    ////////////////////////////////////////
    // Analog FE
    std::array<StripModel, NStrips> m_stripArray;
};

#endif //__STAR_EMU_H__
