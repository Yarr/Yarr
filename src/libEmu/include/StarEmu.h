#ifndef __STAR_EMU_H__
#define __STAR_EMU_H__

#include "StarChips.h"

#include <atomic>
#include <memory>
#include <queue>

#include <cstdint>
#include <iterator>
#include <algorithm>

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
    std::unique_ptr<StarCfg> m_starCfg;
};

/////////////////////////////////////////////////////

/////////////////////////////////////////////////////
// A mockup of the StarCfg API as defined in https://gitlab.cern.ch/YARR/YARR/blob/devel_FelixNetIO_StarChip/src/libStar/include/StarCfg.h
// A temporary solution before the two branches are merged
enum ABCStarRegs
{
    // Special register: 0x00
    // Analog and DCS registers: 0x01 - 0x0f
    // Input/Mask registers: 0x10 - 0x17
    MaskInput0=16, MaskInput1=17, MaskInput2=18, MaskInput3=19,
    MaskInput4=20, MaskInput5=21, MaskInput6=22, MaskInput7=23,
    // Configuration registers: 0x20 - 0x2f
    CREG0=32
    // Status registers: 0x30 - 0x3e
    // High Priority Register: 0x3f
    // TrimDAC registers: 0x40 - 0x67
    // Calibration Enable registers: 0x68 - 0x6f
    // Hit counters registers: 0x70 - 0xaf
};

class StarCfg {
  public:
    StarCfg(){}
    ~StarCfg(){}

    void initRegisterMaps()
    {
        // HCCStar
        
        // ABCStars
        for (int i=0; i<m_nABC; ++i) {
            registerMap[i+1][ABCStarRegs::MaskInput0] = 0; // ch31 - 0
            registerMap[i+1][ABCStarRegs::MaskInput1] = 0; // ch63 - 32
            registerMap[i+1][ABCStarRegs::MaskInput2] = 0; // ch95 - 64
            registerMap[i+1][ABCStarRegs::MaskInput3] = 0; // ch127 - 96
            registerMap[i+1][ABCStarRegs::MaskInput4] = 0; // ch159 - 128
            registerMap[i+1][ABCStarRegs::MaskInput5] = 0; // ch191 - 160
            registerMap[i+1][ABCStarRegs::MaskInput6] = 0; // ch223 - 192
            registerMap[i+1][ABCStarRegs::MaskInput7] = 0; // ch255 - 224

            registerMap[i+1][ABCStarRegs::CREG0] = 0x0ff00000;
            // SubRegisters currently in use
            // [27:24] TestPatt2; [23:20] TestPatt1; [18] TestPattEnable;
            // [17:16] TM; [4] TestPulseEnable
        }
    }
    
    const uint32_t getHCCRegister(uint32_t addr)
    {
        if (registerMap[0].find(addr) == registerMap[0].end())
            return 0xdeadbeef; // for now
        else
            return registerMap[0][addr];
    }
    
    void setHCCRegister(uint32_t addr, uint32_t val)
    {
        if (registerMap[0].find(addr) != registerMap[0].end())
            registerMap[0][addr] = val;
    }
    
    const uint32_t getABCRegister(uint32_t addr, int32_t chipID)
    {
        unsigned index = indexForABCchipID(chipID);
        if (index > m_nABC) { // found no ABC chip with the required chip ID
            std::cout << __PRETTY_FUNCTION__ << ": Cannot find an ABCStar chip with ID = " << chipID << std::endl;
            return -1;
        }
        
        if (registerMap[index].find(addr) == registerMap[index].end())
            return 0xabadcafe; // for now
        else
            return registerMap[index][addr];
    }
    
    void setABCRegister(uint32_t addr, uint32_t val, int32_t chipID)
    {
        unsigned index = indexForABCchipID(chipID);
        if (index > m_nABC) { // found no ABC chip with the required chip ID
            std::cout << __PRETTY_FUNCTION__ << ": Cannot find an ABCStar chip with ID = " << chipID << std::endl;
            return;
        }
        
        if (registerMap[index].find(addr) != registerMap[index].end())
            registerMap[index][addr] = val;
    }

    const uint32_t getABCSubRegValue(uint32_t addr, int32_t chipID,
                                     uint8_t msb, uint8_t lsb)
    {
        assert(msb>=lsb);
        uint32_t regVal = this->getABCRegister(addr, chipID);
        return (regVal >> lsb) & ((1<<(msb-lsb+1))-1);
    }
    
    const unsigned getHCCchipID() {return m_hccID;}
    void setHCCchipID(unsigned hccID) {m_hccID = hccID;}

    const unsigned getABCchipID(unsigned chipIndex) {return m_ABCchipIDs[chipIndex-1];}
    void setABCchipIDs() // for now
    {
        for (int i=0; i<m_nABC; ++i) {
            m_ABCchipIDs.push_back(i+10);
        }
    }

    const unsigned int indexForABCchipID(unsigned int chipID)
    {
        return std::distance(m_ABCchipIDs.begin(), std::find(m_ABCchipIDs.begin(), m_ABCchipIDs.end(), chipID)) + 1;
    }
    
    int m_nABC = 0;

  protected:

    unsigned m_hccID;
    std::vector<unsigned int> m_ABCchipIDs;
    
    // 2D map of HCCStar and ABCStars; chip_index: 0 for HCC, iABC+1 for ABC
    // registerMap[chip_index][addr]
    std::map<uint32_t, std::map<uint32_t, uint32_t> >registerMap;
};

#endif //__STAR_EMU_H__
