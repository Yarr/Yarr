#ifndef FELIXTOOLS_H
#define FELIXTOOLS_H

#include <cstdint>
#include <bitset>
#include <atomic>
#include <tuple>
#include <vector>
#include <algorithm>

namespace FelixTools {

  //Enum for declaring various FELIX firmware flavors as defined in section 2.1 of https://edms.cern.ch/ui/file/2681548/1/FELIX_Phase2_firmware_specs.pdf
  //Firmware flavor determined by "FIRMWARE_MODE" FELIX register
  enum FELIX_FW_MODE {
    GBT_mode = 0,
    FULL_mode = 1,
    LTDB_mode = 2,
    FEI4_mode = 3,
    ITK_Pixel = 4,
    ITK_Strip = 5,
    FELIG = 6,
    FULL_mode_emulator = 7,
    FELIX_MROD_mode = 8,
    lpGBT_mode = 9,
    Interlaken_25G = 10,
    Unknown = -1
  };

  using FelixID_t = uint64_t;

  // Take from https://gitlab.cern.ch/atlas-tdaq-felix/ftools/-/blob/master/src/flxdefs.h
  constexpr unsigned BLOCK_LNK_MASK = 0x07C0;
  constexpr unsigned BLOCK_LNK_SHIFT = 6;
  constexpr unsigned BLOCK_EGROUP_MASK_LPGBT = 0x001C;
  constexpr unsigned BLOCK_EGROUP_SHIFT_LPGBT = 2;
  constexpr unsigned BLOCK_EPATH_MASK_LPGBT = 0x0003;
  constexpr unsigned FLX_LINKS = 12; // Number of links per logical FLX device
  constexpr unsigned FLX_TOHOST_EGROUPS = 7; // Number of decoding egroups
  constexpr unsigned FLX_TOFLX_EGROUPS = 5; // Number of encoding egroups

  // FELIX ID definition
  // Take from https://atlas-project-felix.web.cern.ch/atlas-project-felix/user/docs/LinkMappingSpecification.pdf, Figure 3
  // Version [63:60] is always 0x1
  constexpr unsigned FELIXID_VER_SHIFT = 60;
  // Detector ID [59:52], 8 bits
  constexpr unsigned FELIXID_DID_NBITS = 8;
  constexpr unsigned FELIXID_DID_SHIFT = 52;
  // Connector ID [51:36], 16 bits
  constexpr unsigned FELIXID_CID_NBITS = 16;
  constexpr unsigned FELIXID_CID_SHIFT = 36;
  // Is virtual link [35], 1 bit
  constexpr unsigned FELIXID_ISVIRT_NBITS = 1;
  constexpr unsigned FELIXID_ISVIRT_SHIFT = 35;
  // Link (GBT) ID [34:22], 13 bits
  constexpr unsigned FELIXID_LINKID_NBITS = 13;
  constexpr unsigned FELIXID_LINKID_SHIFT = 22;
  // E-link [21:16], 6 bits
  constexpr unsigned FELIXID_ELINK_NBITS = 6;
  constexpr unsigned FELIXID_ELINK_SHIFT = 16;
  // Link direction [15] (0 = to host; 1 = to FELIX), 1 bit
  constexpr unsigned FELIXID_TOFLX_NBITS = 1;
  constexpr unsigned FELIXID_TOFLX_SHIFT = 15;
  // Protocol [14:8], 7 bits
  // https://atlas-project-felix.web.cern.ch/atlas-project-felix/user/docs/LinkMappingSpecification.pdf, Table 2
  constexpr unsigned FELIXID_PROTO_NBITS = 7;
  constexpr unsigned FELIXID_PROTO_SHIFT = 8;
  // Stream ID [7:0], 8 bits
  constexpr unsigned FELIXID_STREAM_NBITS = 8;
  constexpr unsigned FELIXID_STREAM_SHIFT = 0;

  FelixID_t get_fid(uint8_t detectorID, uint16_t connectorID, bool is_virtual, uint16_t linkID, uint8_t elink, bool to_felix, uint8_t protocol, uint8_t streamID);

  // Utfilities to convert channel numbers to elink numbers
  // chn[18:6] is the link ID; chn[5:0] is the e-link number
  inline uint16_t link_from_chn(uint32_t chn) {
    return (chn & BLOCK_LNK_MASK) >> BLOCK_LNK_SHIFT;
  }

  inline uint8_t elink_from_chn(uint32_t chn) {
    return chn & (BLOCK_EGROUP_MASK_LPGBT | BLOCK_EPATH_MASK_LPGBT);
  }

  inline uint8_t egroup_from_elink(uint8_t elink) {
    return (elink & BLOCK_EGROUP_MASK_LPGBT) >> BLOCK_EGROUP_SHIFT_LPGBT;
  }

  inline uint8_t epath_from_elink(uint8_t elink) {
    return elink & BLOCK_EPATH_MASK_LPGBT;
  }

  // Special cases for ITk Strips LCB encoder e-links
  // FELIX Phase 2 FW spec (v1.037)
  // Section 8.5.9 ITK STRIPS LCB ENCODER Table 8.31
  // Subject to change in the future FELIX firmware
  inline uint8_t egroup_from_elink_lcb(uint8_t elink) { return elink / 5;}
  inline uint8_t epath_from_elink_lcb(uint8_t elink) { return elink % 5;}
  /// Get LCB config, command, and trickle channel numbers
  std::tuple<uint32_t,uint32_t,uint32_t> lcbChns_from_chn(uint32_t chn);

  std::tuple<uint16_t,uint8_t,uint8_t> linkInfo_from_chn(uint32_t chn, bool toflx, FELIX_FW_MODE fwmode);

  inline uint16_t link_from_fid(FelixID_t fid) {
    return (fid >> FELIXID_LINKID_SHIFT) & ((1<<FELIXID_LINKID_NBITS) - 1);
  }

  inline bool toflx_from_fid(FelixID_t fid) {
    return (fid >> FELIXID_TOFLX_SHIFT) & 1;
  }

  inline bool all_toflx_from_fids(const std::vector<FelixID_t>& fids) {
    if (fids.empty()) {
      return false;
    } else {
      return std::all_of(fids.begin(), fids.end(), [](FelixID_t fid){return toflx_from_fid(fid);});
    }
  }

  inline bool all_tohost_from_fids(const std::vector<FelixID_t>& fids) {
    if (fids.empty()) {
      return false;
    } else {
      return std::all_of(fids.begin(), fids.end(), [](FelixID_t fid){return not toflx_from_fid(fid);});
    }
  }

  inline uint8_t elink_from_fid(FelixID_t fid) {
    return (fid >> FELIXID_ELINK_SHIFT) & ((1<<FELIXID_ELINK_NBITS) - 1);
  }

  std::tuple<uint16_t,uint8_t,uint8_t,bool> linkInfo_from_fid(FelixID_t fid, FELIX_FW_MODE fwmode);

  // FELIX register names for elink control
  std::string getICEnableRegName(uint16_t linkId, bool toflx);
  std::string getECEnableRegName(uint16_t linkId, bool toflx);
  std::string getELinkRegName(unsigned linkId, unsigned egroup, bool toflx, const std::string& suffix);
  std::string getELinkEnableRegName(unsigned linkId, unsigned egroup, bool toflx);
  std::string getELinkWidthRegName(unsigned linkId, unsigned egroup, bool toflx);
  // overload using FELIX ID
  std::string getICEnableRegName(FelixID_t fid);
  std::string getECEnableRegName(FelixID_t fid);
  std::string getELinkRegName(FelixID_t fid, FELIX_FW_MODE fwmode, const std::string& suffix);
  std::string getELinkEnableRegName(FelixID_t fid, FELIX_FW_MODE fwmode);
  std::string getELinkWidthRegName(FelixID_t fid, FELIX_FW_MODE fwmode);

  // Full list of register names
  std::vector<std::string> getAllICEnableRegNames(bool toflx);
  std::vector<std::string> getAllECEnableRegNames(bool toflx);
  std::vector<std::string> getAllELinkEnableRegNames(bool toflx);

  struct QueueStatistics {

    std::atomic<uint64_t> messages_received {0}; // number of messages received
    std::atomic<uint64_t> bytes_received {0}; // number of bytes received

    std::atomic<uint64_t> error {0}; // FELIX_STATUS_FW_MALF, FELIX_STATUS_SW_MALF
    std::atomic<uint64_t> crc {0}; // FELIX_STATUS_FW_CRC
    std::atomic<uint64_t> truncated {0}; // FELIX_STATUS_SW_TRUNC

    std::atomic<bool> connected {0};

    std::atomic<double> msg_rate {-1.}; // Hz
    std::atomic<double> byte_rate {-1.}; // B/s

    void reset_errors() {
      error = 0;
      crc = 0;
      truncated = 0;
    }

    void reset_counters() {
      messages_received = 0;
      bytes_received = 0;
    }

  };

}

#endif
