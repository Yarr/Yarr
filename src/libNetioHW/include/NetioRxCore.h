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
#include "netio/netio.hpp"
#include <queue>
#include <cstdint>
#include <chrono>
#include <vector>
#include <mutex>

#include "json.hpp"

using json=nlohmann::basic_json<std::map, std::vector, std::string, bool, std::int32_t, std::uint32_t, float>;

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
  ~NetioRxCore();
  
  //! setRxEnable
  //void setRxEnable(uint32_t val);
  
  /**
   * @brief enable channel
   * @param chn the channel (elink) to enable
   * Add the channel to the NetioHandler.
   * Add channel to elink map.
   **/
  void enableChannel(uint64_t chn);
  
  /**
   * @brief disable channel
   * @param chn the channel (elink) to disable
   * Remove channel from elink map.
   **/
  void disableChannel(uint64_t chn);
  
  /**
   * @brief read data
   * @param chn channel (elink) to read-out
   * Read all the data available in given channel
   **/
  RawData* readData(uint64_t chn);

  /**
   * @brief read all data available in this netio socket
   * @return RawDataContainer pointer containing the 
   *         RawData for each enabled channel
   **/ 
  RawDataContainer* readAllData();
        
  /**
   * @brief get rate of events
   * @return number of events read counted by the statistics thread
   **/
  uint32_t getDataRate();

  /**
   * @brief get the estimated size of the first queue
   * @return the size of the first queue in bytes
   **/
  uint32_t getCurCount();

  /**
   * @brief check if the NetioHandler is not receiving data still
   * @return true if the NetioHanlder is not receiving data
   **/
  bool isDataReady();
  
  /**
   * @brief Start checking the queues in NetioHandler
   **/
  void connect();

  /**
   * @brief enable or disable the verbose mode
   * @param enable boolean value
   **/
  void setVerbose(bool enable);

  /**
   * @brief write configuration to string
   * @param s reference to string where to write the configuration to
   * Not implemented
   **/
  void toString(std::string &s);
  
  /**
   * @brief read configuration from string
   * @param s reference to string where to read the configuration from
   * String structure should be "hostname":port
   **/
  void fromString(std::string s);
  
  /**
   * @brief write configuration to json
   * @param j reference to json where to write the configuration to
   * Not implemented
   **/
  void toFileJson(json &j);
  
  /**
   * @brief read configuration from json
   * @param j reference to string where to write the configuration from
   * Json structure should be {"NetIO":{"host":"hostname","rxport":port}}
   **/
  void fromFileJson(json &j);

private:

  std::string m_felixhost;          //! felix hostname
  uint16_t m_felixport;             //! felix port for reading

  netio::context * m_context;       //! the netio context
  
  std::map<uint64_t,bool> m_elinks; //! elinks map
  bool m_verbose;                   //! verbose mode

  bool m_cont;                      //! bool variable to stop statistics thread
  double m_rate;                    //! rate of bytes per second
  uint32_t m_bytesReceived;         //! number of bytes received
  std::mutex m_mutex;               //! mutex to increase bytes in statistics thread
  std::thread m_statistics;         //! statistics thread

  NetioHandler& m_nioh;             //! NetioHandler object
  std::chrono::steady_clock::time_point m_t0; //! clock used for time measurements

};

#endif

