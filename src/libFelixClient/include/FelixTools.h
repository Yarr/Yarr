#ifndef FELIXTOOLS_H
#define FELIXTOOLS_H

#include <cstdint>
#include <bitset>
#include <atomic>

namespace FelixTools {

  using FelixID_t = uint64_t;

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
