#ifndef FELIXTOOLS_H
#define FELIXTOOLS_H

#include <cstdint>
#include <bitset>
#include <atomic>
#include <tuple>
#include <vector>
#include <algorithm>

namespace FelixTools {

  /// Enum for FELIX firmware flavor determined by "FIRMWARE_MODE" register
  /// Defined in section 2.1 of https://edms.cern.ch/ui/file/2681548/1/FELIX_Phase2_firmware_specs.pdf
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
  constexpr unsigned BLOCK_ELINK_MASK = 0x003F;
  constexpr unsigned BLOCK_EGROUP_MASK_LPGBT = 0x001C;
  constexpr unsigned BLOCK_EGROUP_SHIFT_LPGBT = 2;
  constexpr unsigned BLOCK_EPATH_MASK_LPGBT = 0x0003;
  constexpr unsigned FLX_LINKS = 12; // Number of links per logical FLX device
  constexpr unsigned FLX_TOHOST_EGROUPS = 7; // Number of decoding egroups
  constexpr unsigned FLX_TOFLX_EGROUPS = 5; // Number of encoding egroups

  /*
  FELIX ID definition
  Take from https://atlas-project-felix.web.cern.ch/atlas-project-felix/user/docs/LinkMappingSpecification.pdf, Figure 3
  */
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

  /// Get 64-bit Felix ID
  FelixID_t get_fid(uint8_t detectorID, uint16_t connectorID, bool is_virtual, uint16_t linkID, uint8_t elink, bool to_felix, uint8_t protocol, uint8_t streamID);

  /**
   * Extract fields from Felix ID.
   */
  std::string print_fid(uint64_t fid);

  /*
  Utilities to extract link and e-link information from channel numbers
  */

  /// @brief Get lpGBT link ID from channel number
  /// @param chn Channel number (link ID and elink number)
  /// @return LpGBT link ID
  inline uint16_t link_from_chn(uint32_t chn) {
    return (chn & BLOCK_LNK_MASK) >> BLOCK_LNK_SHIFT;
  }

  /// @brief Get elink number from channel number
  /// @param chn Channel number (link ID and elink number)
  /// @return Elink number
  inline uint8_t elink_from_chn(uint32_t chn) {
    return chn & BLOCK_ELINK_MASK; // lowest 6 bits
  }

  /// @brief Get egroup from elink number
  /// @param elink Elink number
  /// @return Egroup
  inline uint8_t egroup_from_elink(uint8_t elink) {
    return (elink & BLOCK_EGROUP_MASK_LPGBT) >> BLOCK_EGROUP_SHIFT_LPGBT;
  }

  /// @brief Get epath from elink number
  /// @param elink Elink number
  /// @return Epath
  inline uint8_t epath_from_elink(uint8_t elink) {
    return elink & BLOCK_EPATH_MASK_LPGBT;
  }

  /*
  Special cases for ITk Strips LCB encoder e-links
  FELIX Phase 2 FW spec (v1.037)
  Section 8.5.9 ITK STRIPS LCB ENCODER Table 8.31
  Subject to change in the future FELIX firmware
  */

  /// @brief Get egroup from elink number for the ITk Strip LCB encoder
  /// @param elink Elink number
  /// @return Egroup
  inline uint8_t egroup_from_elink_lcb(uint8_t elink) { return elink / 5;}

  /// @brief Get epath from elink number for the ITk Strip LCB encoder
  /// @param elink Elink number
  /// @return Epath
  inline uint8_t epath_from_elink_lcb(uint8_t elink) { return elink % 5;}

  /// @brief Get LCB config, command, and trickle channel numbers given a 32-bit channel number for the ITk Strip LCB encoder
  /// @param chn Channel number (link ID and elink number)
  /// @return A tuple of three channel numbers for the LCB config, command, and trickle configureation associated with the input channel number
  std::tuple<uint32_t,uint32_t,uint32_t> lcbChns_from_chn(uint32_t chn);

  /// @brief Get link information including lpGBT link ID, egroup, and epath based on the channel number and firmware mode
  /// @param chn Channel number (link ID and elink number)
  /// @param toflx A boolen flag to indicate the direction of the channel
  /// @param fwmode FELIX firmware mode as defined in enum FELIX_FW_MODE
  /// @return A tuple of link ID, egroup, and epath number
  std::tuple<uint16_t,uint8_t,uint8_t> linkInfo_from_chn(uint32_t chn, bool toflx, FELIX_FW_MODE fwmode);

  /// @brief Extract lpGBT link ID from FELIX ID
  /// @param fid 64-bit FELIX ID
  /// @return Link ID
  inline uint16_t link_from_fid(FelixID_t fid) {
    return (fid >> FELIXID_LINKID_SHIFT) & ((1<<FELIXID_LINKID_NBITS) - 1);
  }

  /// @brief Extract the flag that indicates the link direction from the FELIX ID
  /// @param fid 64-bit FELIX ID
  /// @return True if the link direction is to FELIX, false if to host
  inline bool toflx_from_fid(FelixID_t fid) {
    return (fid >> FELIXID_TOFLX_SHIFT) & 1;
  }

  /// @brief Check if all FELIX IDs are in the ToFLX direction
  /// @param fids A vector of FELIX IDs
  /// @return True if all links are in the ToFLX direction, false otherwise
  inline bool all_toflx_from_fids(const std::vector<FelixID_t>& fids) {
    if (fids.empty()) {
      return false;
    } else {
      return std::all_of(fids.begin(), fids.end(), [](FelixID_t fid){return toflx_from_fid(fid);});
    }
  }

  /// @brief Check if all FELIX IDs are in the ToHost direction
  /// @param fids A vector of FELIX IDs
  /// @return True if all links are in the ToHost direction, false otherwise
  inline bool all_tohost_from_fids(const std::vector<FelixID_t>& fids) {
    if (fids.empty()) {
      return false;
    } else {
      return std::all_of(fids.begin(), fids.end(), [](FelixID_t fid){return not toflx_from_fid(fid);});
    }
  }

  /// @brief Extract elink number of FELIX ID
  /// @param fid 64-bit FELIX ID
  /// @return Elink number
  inline uint8_t elink_from_fid(FelixID_t fid) {
    return (fid >> FELIXID_ELINK_SHIFT) & ((1<<FELIXID_ELINK_NBITS) - 1);
  }

  /// @brief Get link information including lpGBT link ID, egroup, and epath number based on the FELIX ID and firmware mode
  /// @param fid 64-bit FELIX ID
  /// @param fwmode FELIX firmware mode
  /// @return A tuple of link ID, egroup, epath number, and toflx (link direction)
  std::tuple<uint16_t,uint8_t,uint8_t,bool> linkInfo_from_fid(FelixID_t fid, FELIX_FW_MODE fwmode);

  /*
  FELIX register names for elink control
  */

  /// @brief Get the IC enable register name associated with a link
  /// @param linkId LpGBT link ID
  /// @param toflx Link direction
  /// @return IC enable register name
  std::string getICEnableRegName(uint16_t linkId, bool toflx);

  /// @brief Get the EC enable register name assocaited with a link
  /// @param linkId LpGBT link ID
  /// @param toflx Link direction
  /// @return EC enable register name
  std::string getECEnableRegName(uint16_t linkId, bool toflx);

  /// @brief Get the elink register name associated with a link
  /// @param linkId LpGBT link ID
  /// @param egroup Egroup number
  /// @param toflx Link direction
  /// @param suffix Suffix of the register name
  /// @return Register name
  std::string getELinkRegName(unsigned linkId, unsigned egroup, bool toflx, const std::string& suffix);

  /// @brief Get the elink enable register name associated with a link
  /// @param linkId LpGBT link ID
  /// @param egroup Egroup number
  /// @param toflx Link direction
  /// @return Elink enable register name
  std::string getELinkEnableRegName(unsigned linkId, unsigned egroup, bool toflx);

  /// @brief Get the elink width register name associated with a link
  /// @param linkId LpGBT link ID
  /// @param egroup Egroup number
  /// @param toflx Link direction
  /// @return Elink width register name
  std::string getELinkWidthRegName(unsigned linkId, unsigned egroup, bool toflx);

  /// @brief Get the elink path encoding register name associated with a link
  /// @param linkId LpGBT link ID
  /// @param egroup Egroup number
  /// @param toflx Link direction
  /// @return Elink width register name
  std::string getEgroupEncodingDecodingRegName(unsigned linkId, unsigned egroup, bool toflx);

  /* Overload using FELIX ID */
  /// @brief Get the IC enable register name associated with a link
  /// @param fid 64-bit FELIX ID
  /// @return IC enable register name
  std::string getICEnableRegName(FelixID_t fid);

  /// @brief Get the EC enable register name associated with a link
  /// @param fid 64-bit FELIX ID
  /// @return EC enable register name
  std::string getECEnableRegName(FelixID_t fid);

  /// @brief Get the elink register name associated with a link
  /// @param fid 64-bit FELIX ID
  /// @param fwmode FELIX firmware mode
  /// @param suffix Suffix of the register name
  /// @return Register name
  std::string getELinkRegName(FelixID_t fid, FELIX_FW_MODE fwmode, const std::string& suffix);

  /// @brief Get the elink enable register name associated with a link
  /// @param fid 64-bit FELIX ID
  /// @param fwmode FELIX firmware mode
  /// @return Elink enable register name
  std::string getELinkEnableRegName(FelixID_t fid, FELIX_FW_MODE fwmode);

  /// @brief Get the elink width register name associated with a link
  /// @param fid 64-bit FELIX ID
  /// @param fwmode FELIX firmware mode
  /// @return Elink width register name
  std::string getELinkWidthRegName(FelixID_t fid, FELIX_FW_MODE fwmode);

  /// @brief Get the names of all IC enable registers of a FELIX device
  /// @param toflx Link direction
  /// @return A vector of IC enable register names
  std::vector<std::string> getAllICEnableRegNames(bool toflx);

  /// @brief Get the names of all EC enable registers of a FELIX device
  /// @param toflx Link direction
  /// @return A vector of EC enable register names
  std::vector<std::string> getAllECEnableRegNames(bool toflx);

  /// @brief Get the names of all elink enable registers of a FELIX device
  /// @param toflx Link direction
  /// @return A vector of elink enable register names
  std::vector<std::string> getAllELinkEnableRegNames(bool toflx);

  /// @brief A struct for keeping track of rx queue statistics
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
