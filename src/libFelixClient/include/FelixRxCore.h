#ifndef FELIXRXCORE_H
#define FELIXRXCORE_H

#include "felix/felix_client_thread.hpp"
#include "storage.hpp"

#include "RxCore.h"
#include "RawData.h"
#include "ClipBoard.h"
#include "FelixTools.h"

class FelixRxCore : virtual public RxCore {

public:

  FelixRxCore();
  ~FelixRxCore() override;

  void setRxEnable(uint32_t val) override;
  void setRxEnable(std::vector<uint32_t> channels) override;
  void disableRx() override;
  void maskRxEnable(uint32_t val, uint32_t mask) override;

  void flushBuffer() override;
  RawData* readData() override;

  uint32_t getDataRate() override;
  uint32_t getCurCount() override;
  bool isBridgeEmpty() override;

  void checkDataRate();

protected:

  void writeConfig(json &j);
  void loadConfig(const json &j);
  void setClient(FelixClientThread* client); // set Felix client

  // on data callback
  void on_data(uint64_t fid, const uint8_t* data, size_t size, uint8_t status);

private:

  using FelixID_t = FelixTools::FelixID_t;

  // Channel control
  void enableChannel(FelixID_t fid);
  void enableChannel(std::vector<FelixID_t> fids);
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
  ClipBoard<RawData> rawData;

  // Owned by FelixController
  FelixClientThread* fclient {nullptr};

  // Receiver queue status
  std::atomic<uint64_t> messages_received {0}; // total number of messages received
  std::atomic<uint64_t> bytes_received {0}; // total number of bytes received

  double msg_rate {-1}; // message rate
  double byte_rate {-1}; // total data rate
  std::chrono::steady_clock::time_point m_t0; // clock used for time measurement

};
#endif
