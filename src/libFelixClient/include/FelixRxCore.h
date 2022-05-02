#ifndef FELIXRXCORE_H
#define FELIXRXCORE_H

#include "felix/felix_client_thread.hpp"
#include "storage.hpp"

#include "RxCore.h"
#include "RawData.h"
#include "ClipBoard.h"
#include "FelixTools.h"

#include <thread>

class FelixRxCore : virtual public RxCore {

public:

  FelixRxCore();
  ~FelixRxCore() override;

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

protected:

  using FelixID_t = FelixTools::FelixID_t;

  void writeConfig(json &j);
  void loadConfig(const json &j);
  void setClient(std::shared_ptr<FelixClientThread> client); // set Felix client

  // on data callback
  void on_data(FelixID_t fid, const uint8_t* data, size_t size, uint8_t status);
  // on connect/disconnect callbacks
  void on_connect(FelixID_t fid);
  void on_disconnect(FelixID_t fid);

  // Channel control
  void enableChannel(FelixID_t fid);
  void disableChannel(FelixID_t fid);

  std::map<FelixID_t, bool> m_enables; // enable flag for each elink

  std::atomic<bool> m_doFlushBuffer {false};
  unsigned m_flushTime {50}; // in milliseconds

  // For Felix ID
  FelixID_t fid_from_channel(uint32_t chn);
  uint8_t m_did {0};  // detector ID; 0x00 reserved for local IDs
  uint16_t m_cid {0}; // connector ID; 0x0000 reserved for local IDs
  uint8_t m_protocol {0}; // protocol ID

  // Data container
  ClipBoard<RawData> m_rawData;

  // Receiver queue status
  std::atomic<uint64_t> m_total_data_in {0}; // total number of data received
  std::atomic<uint64_t> m_total_data_out {0}; // total number of data read out
  std::atomic<uint64_t> m_total_bytes_in {0};
  std::atomic<uint64_t> m_total_bytes_out {0};

  // Per channel statistics
  std::map<FelixID_t, FelixTools::QueueStatistics> m_qStats;

  std::thread m_monitor_thread;
  std::atomic<bool> m_runMonitor {false};
  uint32_t m_interval_ms {1000}; // monitoring interval in ms
  uint64_t m_queue_limit {4000}; // MB

  double msg_rate {-1}; // message rate
  double byte_rate {-1}; // total data rate
  std::chrono::steady_clock::time_point m_t0; // clock used for time measurement

  std::shared_ptr<FelixClientThread> fclient;
};
#endif
