#ifndef FELIXTOOLS_H
#define FELIXTOOLS_H

#include <cstdint>
#include <bitset>
#include <atomic>

namespace FelixTools {

  using FelixID_t = uint64_t;

  // Take from https://gitlab.cern.ch/atlas-tdaq-felix/ftools/-/blob/master/src/flxdefs.h
  constexpr unsigned int BLOCK_LNK_MASK = 0x07C0;
  constexpr unsigned int BLOCK_LNK_SHIFT = 6;
  constexpr unsigned int BLOCK_EGROUP_MASK_LPGBT = 0x001C;
  constexpr unsigned int BLOCK_EGROUP_SHIFT_LPGBT = 2;
  constexpr unsigned int BLOCK_EPATH_MASK_LPGBT = 0x0003;

  // Utfilities to convert channel numbers to elink numbers
  // chn[18:6] is the link ID; chn[5:0] is the e-link number
  inline uint16_t link_from_chn(uint32_t chn) {
    return (chn & BLOCK_LNK_MASK) >> BLOCK_LNK_SHIFT;
  }

  inline uint8_t elink_from_chn(uint32_t chn) {
    return chn & (BLOCK_EGROUP_MASK_LPGBT | BLOCK_EPATH_MASK_LPGBT);
  }

  inline uint8_t egroup_from_chn(uint32_t chn) {
    return (chn & BLOCK_EGROUP_MASK_LPGBT) >> BLOCK_EGROUP_SHIFT_LPGBT;
  }

  inline uint8_t epath_from_chn(uint32_t chn) {
    return chn & BLOCK_EPATH_MASK_LPGBT;
  }

  FelixID_t get_fid(uint8_t detectorID, uint16_t connectorID, bool is_virtual, uint16_t linkID, uint8_t elink, bool to_felix, uint8_t protocol, uint8_t streamID);

  unsigned link_from_fid(FelixID_t);
  unsigned elink_from_fid(FelixID_t);

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
