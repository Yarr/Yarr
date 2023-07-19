#ifndef NETIO_HANDLER_H_
#define NETIO_HANDLER_H_

/********************************
 * NetioHandler
 * Author: Roland.Sipos@cern.ch
 * Description: Wrapper class for
 *   NETIO sockets and folly SPSC
 *   circular buffers.
 * Date: November 2017
 *********************************/

#include "netio/netio.hpp"

#include "RawData.h"
#include "ClipBoard.h"

#include <map>
#include <memory>
#include <thread>

class NetioHandler
{
public:
  // Prevent copying and moving.
  NetioHandler(NetioHandler const&) = delete;             // Copy construct
  NetioHandler(NetioHandler&&) = delete;                  // Move construct
  NetioHandler& operator=(NetioHandler const&) = delete;  // Copy assign
  NetioHandler& operator=(NetioHandler &&) = delete;      // Move assign

  ClipBoard<RawData> rawData;
  void setFlushBuffer(bool);

  // return data received
  int getDataCount() const;

  // Functionalities
  void addChannel(uint64_t chn); // Enable an elink (prepare a queue, socket-pairs and sub to elink.
  void delChannel(uint64_t chn); // Enable an elink (prepare a queue, socket-pairs and sub to elink.
  void setFelixHost(std::string felixHost){m_felixHost=felixHost;}
  void setFelixRXPort(uint16_t felixRXPort){m_felixRXPort=felixRXPort;}

  // set flag to keep rd53a and strips specific things seperate
  void setFeType(std::string feType){m_feType=feType;}

  NetioHandler(std::string contextStr, std::string felixHost, uint16_t felixRXPort);
  //MW: FIX CLANG COMPILATION
  ~NetioHandler();

private:
  int handlerDataCount;

  // used as a flag to keep rd53a and strips specific things seperate
  std::string m_feType;

  std::vector<uint64_t> m_channels;

  // NETIO
  netio::context * m_context; // context
  std::string m_felixHost;    // hostname
  uint16_t m_felixRXPort;     // RX port (ususally 12345)
  std::thread m_netio_bg_thread;
  std::map<uint64_t, netio::low_latency_subscribe_socket*> m_sub_sockets; // subscribe sockets.

  // used to keep strips and rd53a specific things seperate
  std::string feType; // rd53a or fei4

  // Other statistics for channels:
  std::map<uint64_t, uint32_t> m_msgErrors;
};

#endif

