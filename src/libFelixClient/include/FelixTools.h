#ifndef FELIXTOOLS_H
#define FELIXTOOLS_H

#include <cstdint>
#include <bitset>
#include <atomic>

namespace FelixTools {

  using FelixID_t = uint64_t;

  // Take from https://gitlab.cern.ch/atlas-tdaq-felix/ftools/-/blob/master/src/flxdefs.h
  constexpr unsigned BLOCK_LNK_MASK = 0x07C0;
  constexpr unsigned BLOCK_LNK_SHIFT = 6;
  constexpr unsigned BLOCK_EGROUP_MASK_LPGBT = 0x001C;
  constexpr unsigned BLOCK_EGROUP_SHIFT_LPGBT = 2;
  constexpr unsigned BLOCK_EPATH_MASK_LPGBT = 0x0003;

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

  std::tuple<uint16_t,uint8_t,uint8_t> linkInfo_from_chn(uint32_t chn) {
    uint16_t linkId = link_from_chn(chn);
    uint8_t elink = elink_from_chn(chn);
    uint8_t egroup = egroup_from_elink(elink);
    uint8_t epath = epath_from_elink(elink);

    // return linkId, egroup, epath
    return std::make_tuple(linkId, egroup, epath);
  }

  inline uint16_t link_from_fid(FelixID_t fid) {
    return (fid >> FELIXID_LINKID_SHIFT) & ((1<<FELIXID_LINKID_NBITS) - 1);
  }

  inline bool toflx_from_fid(FelixID_t fid) {
    return (fid >> FELIXID_TOFLX_SHIFT) & 1;
  }

  inline uint8_t elink_from_fid(FelixID_t fid) {
    return (fid >> FELIXID_ELINK_SHIFT) & ((1<<FELIXID_ELINK_NBITS) - 1);
  }

  std::tuple<uint16_t,uint8_t,uint8_t,bool> linkInfo_from_fid(FelixID_t fid) {
    uint16_t linkId = link_from_fid(fid);
    uint8_t elink = elink_from_fid(fid);
    uint8_t egroup = egroup_from_elink(elink);
    uint8_t epath = epath_from_elink(elink);
    bool toflx = toflx_from_fid(fid);

    // return linkId, egroup, epath, toflx
    return std::make_tuple(linkId, egroup, epath, toflx);
  }

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
