#ifndef FELIXRXCORE_H
#define FELIXRXCORE_H

#include "felix/felix_client_thread.hpp"

#include "storage.hpp"

#include "RxCore.h"
#include "RawData.h"
#include "ClipBoard.h"
#include "FelixTools.h"
#include "FelixRxThread.h"

class FelixRxCore : virtual public RxCore {

public:

  FelixRxCore();
  ~FelixRxCore() override;

  /// Register all Rx channels and distribute them to threads
  void initRxChannels(const std::vector<uint32_t>& channels) override;

  void setRxEnable(uint32_t val) override;
  void setRxEnable(std::vector<uint32_t> channels) override;
  void disableRx() override;
  void maskRxEnable(uint32_t val, uint32_t mask) override;

  void flushBuffer() override;

  // Clears out the m_rawData vector
  // This vector stores incoming data read in the onData() function
  void clearRawData();
  std::vector<RawDataPtr> readData() override;

  uint32_t getDataRate() override;
  uint32_t getCurCount() override;
  bool isBridgeEmpty() override;

  void runMonitor(bool print_info=false);
  void stopMonitor();

  FelixTools::FelixID_t fid_from_channel(uint32_t chn);
  FelixTools::FelixID_t ic_fid_from_channel(uint32_t chn); // get the fid for ic communication from the channel number

protected:

  using FelixID_t = FelixTools::FelixID_t;

  void writeConfig(json &j);
  void loadConfig(const json &j);
  void setClient(const FelixClientThread::Config& fcConfig); // set Felix clients

  // Channel control
  void enableChannel(FelixID_t fid);
  void disableChannel(FelixID_t fid);

  bool channelIsEnabled(FelixID_t fid) {
    return m_rxThreads[m_fidThreadMap[fid]]->channelIsEnabled(fid);
  }

  unsigned m_flushWaitTime {50}; // in milliseconds

  // For Felix ID
  uint8_t m_did {0};  // detector ID; 0x00 reserved for local IDs
  uint16_t m_cid {0}; // connector ID; 0x0000 reserved for local IDs
  uint8_t m_protocol {0}; // protocol ID

  // Felix clients
  unsigned m_nThreads {1};
  std::vector<std::unique_ptr<FelixRxThread>> m_rxThreads;
  FelixClientThread::Config m_fcConfig; // Felix client configuration
  std::map<FelixID_t, unsigned> m_fidThreadMap; // map of Felix ID to thread index

  // Monitoring
  std::thread m_monitor_thread;
  std::atomic<bool> m_runMonitor {false};
  uint32_t m_interval_ms {1000}; // monitoring interval in ms
  uint64_t m_queue_limit {4000}; // MB
  size_t m_maxMessageSize {0}; // if set to >0, on_data drops messages with larger sizes
  std::chrono::steady_clock::time_point m_t0; // clock used for time measurement
};

#endif