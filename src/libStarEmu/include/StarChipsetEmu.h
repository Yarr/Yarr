#ifndef __STAR_CHIPSET_EMU_H__
#define __STAR_CHIPSET_EMU_H__

#include "StarCfg.h"
#include "LCBUtils.h"
#include "StripModel.h"

#include <queue>
#include <bitset>

class StarCfg;

/**
 * Emulation of one HCCStar and its connected ABCStars.
 */

class StarChipsetEmu {
public:

  enum class PacketTypes {
    PR = 1, LP = 2, ABCRegRd = 4, ABCPacketTransRegRd = 7, HCCRegRd = 8,
    ABCFullTransRegRd = 11, ABCHPR = 13, HCCHPR = 14
  };

  StarChipsetEmu(ClipBoard<RawData>* rx, const std::string& json_emu_file_path,
                 std::unique_ptr<StarCfg> regCfg, unsigned hpr_period,
                 int abc_version, int hcc_version, bool addressing_mode_dynamic);
  ~StarChipsetEmu();

  void doL0A(bool, uint8_t, uint8_t);
  void doPRLP(uint8_t mask, uint8_t l0tag);
  void doFastCommand(uint8_t);
  void doRegReadWrite(LCB::Frame);
  void doHPR(LCB::Frame);

  uint16_t getBC() const {return m_bccnt;}
  void setBC(uint16_t bc) {m_bccnt = bc;}

  void fillL0Buffer();

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

  /// Register R/W commands
  void execute_command_sequence();
  void writeRegister(const uint32_t, const uint8_t, bool isABC=false,
                     const unsigned ABCID=0);
  void readRegister(const uint8_t, bool isABC=false, const unsigned ABCID=0);

  /// Fast commands
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

  /// Trigger and front end
  unsigned int countTriggers(LCB::Frame);
  uint16_t clusterFinder_sub(uint64_t&, uint64_t&, bool);
  std::vector<uint16_t> clusterFinder(const StripData&,
                                      const uint8_t maxCluster=63);
  void ackPulseCmd(int pulseType, uint8_t cmdBC);
  void clearFEData();

  /// per ABC
  void countHits(AbcCfg& abc, const StripData& hits) const;
  std::vector<uint16_t> getClusters(const AbcCfg&, const StripData&);
  unsigned getL0BufferAddr(const AbcCfg& abc, uint8_t cmdBC) const;

  std::pair<uint8_t,StripData> getFEData(const AbcCfg& abc, unsigned l0addr);
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
  ClipBoard<RawData>* m_rxbuffer;

  ////////////////////////////////////////
  // Internal states
  //
  // For register command sequence
  bool m_ignoreCmd{true};
  bool m_isForABC{false};

  // buffer for register read/write command sequence
  std::queue<uint8_t> m_reg_cmd_buffer{};

  // Front-end data pipeline
  // Simplified L0 buffer
  // Instead of storing the hit data, it only takes note when a calibration or test pulse command is received.
  // The actual 256b strip data is generated when a trigger is received.
  // Since pulse commands are broadcasted to all chips, there is no need to have separate L0 pipelines for different ABCStar chips
  // 512 deep and each entry is 8b BCID + 2 bits indicating a cal or test pulse
  // 0b01: calibration pulse; 0b10: digital test pulse
  static constexpr unsigned L0BufDepth = 512;
  static constexpr unsigned L0BufWidth = 10;
  using L0BufData = std::bitset<L0BufWidth>;
  std::array<L0BufData, L0BufDepth> m_l0buffer_lite;

  // number of untriggered data in the pipeline
  unsigned int m_ndata_l0buf;

  // Event buffer
  // 128 deep; L0tag is used as the address
  // Each entry: 9-bit L0buffer address + 8-bit BCID@L0A
  static constexpr unsigned EvtBufDepth = 128;
  static constexpr unsigned EvtBufWidth = 17;
  using EvtBufData = std::bitset<EvtBufWidth>;
  // key: ABC chip ID; value: event buffer
  std::map<int, std::array<EvtBufData, EvtBufDepth>> m_evtbuffers_lite;
  // (If ABCs never have different L0 latency settings, one array instead of a map of arrays would suffice.)

  // BC counter
  uint16_t m_bccnt{0};

  // Count hits
  bool m_startHitCount{false};
  uint8_t m_bc_sel{0};

  // Clock counter for HPR packets
  unsigned hpr_clkcnt; // unit: BC (25 ns)
  // Flag indicating if at least one HPR packet has been sent
  std::vector<bool> hpr_sent;
  const unsigned HPRPERIOD; // HPR packet period

  int m_abc_version;
  int m_hcc_version;
  bool m_addressing_mode_dynamic = true;

  ////////////////////////////////////////
  // HCCStar and ABCStar configurations
  std::unique_ptr<StarCfg> m_starCfg;

  ////////////////////////////////////////
  // Analog FE
  std::array<StripModel, NStrips> m_stripArray;
};

#endif //__STAR_CHIPSET_EMU_H__
