#ifndef FELIXRXCORE_H
#define FELIXRXCORE_H

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

  void initRxChannels(const std::vector<uint32_t>& channels) override;
  void setRxEnable(uint32_t val) override;
  void setRxEnable(std::vector<uint32_t> channels) override;
  void disableRx() override;
  void maskRxEnable(uint32_t val, uint32_t mask) override;

  void flushBuffer() override;
  std::vector<RawDataPtr> readData() override;

  uint32_t getDataRate() override;
  uint32_t getCurCount() override;
  bool isBridgeEmpty() override;

  void runMonitor(bool print_info=false);
  void stopMonitor();

  FelixTools::FelixID_t fid_from_channel(uint32_t chn);

protected:

  using FelixID_t = FelixTools::FelixID_t;

  void writeConfig(json &j);
  void loadConfig(const json &j);
  void setClient(std::shared_ptr<SharedClient> client); // set Felix client

  // Channel control
  void enableChannel(FelixID_t fid);
  void disableChannel(FelixID_t fid);

  unsigned m_flushWaitTime {50}; // in milliseconds

  // For Felix ID
  uint8_t m_did {0};  // detector ID; 0x00 reserved for local IDs
  uint16_t m_cid {0}; // connector ID; 0x0000 reserved for local IDs
  uint8_t m_protocol {0}; // protocol ID

  // Felix client
  std::shared_ptr<SharedClient> m_client;
  std::vector<std::unique_ptr<FelixRxThread>> m_rxThreads;
  unsigned m_nThreads {1};

  // Monitoring
  bool m_runMonitor {false};
  uint32_t m_interval_ms {1000}; // monitoring interval in ms
  uint64_t m_queue_limit {4000}; // MB
  size_t m_maxMessageSize {0}; // if set to >0, on_data drops messages with larger sizes
};

#endif