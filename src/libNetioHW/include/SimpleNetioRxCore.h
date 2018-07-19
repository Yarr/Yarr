#ifndef NETIORXCORE_H
#define NETIORXCORE_H

/********************************
 * NetioRxCore
 * Author: Carlos.Solans@cern.ch
 * Description: NetIO Receiver
 * Date: June 2017
 *********************************/

#include "RxCore.h"
#include "RawData.h"
#include "netio/netio.hpp"
#include <queue>
#include <cstdint>
#include <chrono>
#include <vector>
#include <mutex>

#include "json.hpp"

using json=nlohmann::basic_json<std::map, std::vector, std::string, bool, std::int32_t, std::uint32_t, float>;

class SimpleNetioRxCore : virtual public RxCore {
public:

  //! Constructor
  SimpleNetioRxCore();

  //! Destructor
  ~SimpleNetioRxCore();

  void setRxEnable(uint32_t val) override;
  void maskRxEnable(uint32_t val, uint32_t mask) override;

  RawData* readData() override;

  //! getDataRate
  uint32_t getDataRate() override;

  //! getCurCount
  uint32_t getCurCount() override;

  bool isBridgeEmpty() override;

  // Configuration
  void toFileJson(json &j);
  void fromFileJson(json &j);

private:

  bool m_verbose;
  bool m_debug;
  uint32_t m_datasize;

  std::string m_felixhost;
  uint16_t m_felixport;

  netio::context * m_context;
  netio::low_latency_subscribe_socket * m_socket;
  void decode(netio::endpoint& ep, netio::message& msg);

  //! read data
  RawData* readData(uint64_t chn);

  //! enable channel
  void enableChannel(uint64_t chn);

  //! disable channel
  void disableChannel(uint64_t chn);

  std::thread m_bgthread;
  std::chrono::steady_clock::time_point m_t0;
  double m_rate;
  uint32_t m_bytesReceived;
  std::map<netio::tag,uint32_t> m_elinks;
  std::map<netio::tag,std::queue<uint8_t> > m_data;
  std::mutex m_mutex;

  std::thread m_statistics;

};

#endif

