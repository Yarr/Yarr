#ifndef NETIORXCORE_H
#define NETIORXCORE_H

/********************************
 * NetioRxCore
 * Author: Carlos.Solans@cern.ch
 * Author: Roland.Sipos@cern.ch
 * Description: NetIO Receiver
 * Date: June 2017
 *********************************/

#include "RxCore.h"
#include "RawData.h"
#include "NetioHandler.h"
#include "netio.hpp"

#include <cstdint>
#include <chrono>
#include <vector>
#include <mutex>



#include "storage.hpp"

/**
 * @brief Implementation of the RxCore for NetIO communication.
 *        This class subscribes to felix using a low_latency_socket.
 **/
class NetioRxCore : virtual public RxCore {
public:

  /**
   * @brief Default constructor
   * Initialize the netio context and start statistics thread
   **/
  NetioRxCore();

  /**
   * @brief Default destructor
   * Stop the NetioHandler. Delete the channels from the NetioHandler.
   **/
  ~NetioRxCore() override;

  void setRxEnable(uint32_t val) override;
  void setRxEnable(std::vector<uint32_t> channels) override;
  void disableRx() override;
  void maskRxEnable(uint32_t val, uint32_t mask) override;

  void flushBuffer() override;
  std::vector<RawDataPtr> readData() override;

  /**
   * @brief get rate of events
   * @return number of events read counted by the statistics thread
   **/
  uint32_t getDataRate() override;

  /**
   * @brief get the estimated size of the first queue
   * @return the size of the first queue in bytes
   **/
  uint32_t getCurCount() override;

  /**
   * @brief check if the NetioHandler is not receiving data still
   * @return true if the NetioHandler is not receiving data
   **/
  bool isBridgeEmpty() override;

  /**
   * @brief write configuration to json
   * @param j reference to json where to write the configuration to
   * Not implemented
   **/
  void writeConfig(json &j);

  /**
   * @brief read configuration from json
   * @param j reference to string where to write the configuration from
   * Json structure should be {"NetIO":{"host":"hostname","rxPort":port}}
   **/
  void loadConfig(const json &j);

private:
  // to keep track of amount of data received at rxcore
  int rxDataCount;

  // used as flag for merge
  std::string m_feType;


  void enableChannel(uint64_t chn);
  void disableChannel(uint64_t chn);

  void disableAllChannels();

  std::string m_felixhost;          //! felix hostname
  uint16_t m_felixport;             //! felix port for reading

  std::map<uint64_t,bool> m_elinks; //! elinks map

  bool m_cont;                      //! bool variable to stop statistics thread
  double m_rate;                    //! rate of bytes per second
  uint32_t m_bytesReceived;         //! number of bytes received
  std::mutex m_mutex;               //! mutex to increase bytes in statistics thread
  std::thread m_statistics;         //! statistics thread

  NetioHandler m_nioh;              //! NetioHandler object
  std::chrono::steady_clock::time_point m_t0; //! clock used for time measurements  

};

#endif

