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

#include "ProducerConsumerQueue.h"
#include "QueueMonitor.h"
#include "netio/netio.hpp"

#include <map>
#include <memory>
#include <thread>
#include <mutex>
#include <algorithm>

class NetioHandler
{
public:
  // Prevent copying and moving.
  NetioHandler(NetioHandler const&) = delete;             // Copy construct
  NetioHandler(NetioHandler&&) = delete;                  // Move construct
  NetioHandler& operator=(NetioHandler const&) = delete;  // Copy assign
  NetioHandler& operator=(NetioHandler &&) = delete;      // Move assign

  // Custom types
  typedef folly::ProducerConsumerQueue<uint32_t> FollyQueue;
  typedef std::shared_ptr<FollyQueue> SharedQueue;

  // Functionalities
  void addChannel(uint64_t chn); // Enable an elink (prepare a queue, socket-pairs and sub to elink.
  void delChannel(uint64_t chn); // Enable an elink (prepare a queue, socket-pairs and sub to elink.
  void monitorSetup(size_t sensitivity, size_t delay, size_t numOf=0); // Configures monitoring threads.
  void configureMonitors(size_t sensitivity, size_t delay); // Configures monitors -> new dynamic way
  void startChecking(); // Starts the monitoring threads.
  void stopChecking();  // Stops the monitoring threads.
  void send(uint64_t chn, netio::message& msg){ m_send_sockets[chn]->send(msg); } // Sends the msg
  std::vector<uint32_t> pushOut(uint64_t chn); // Push out every records from channel queue.
  SharedQueue& getQueue(uint64_t chn){ return m_pcqs[chn]; } // Access for elink's queue.
  size_t getNumOfChannels() { return m_activeChannels; } // Get the number of active channels.
  bool isStable(uint64_t monitorID); // Returns the stability of elink's queue.
  bool isAllStable(); // Returns the aggregated stability of the queues.
  void setFelixHost(std::string felixHost){m_felixHost=felixHost;}
  void setFelixRXPort(uint16_t felixRXPort){m_felixRXPort=felixRXPort;}
  void setFelixTXPort(uint16_t felixTXPort){m_felixTXPort=felixTXPort;}

public:
  NetioHandler(std::string contextStr, std::string felixHost,
               uint16_t felixTXPort, uint16_t felixRXPort,
               size_t queueSize, bool verbose);

  ~NetioHandler();

private:
  bool m_isConfigured = false;
  std::vector<uint64_t> m_channels;
  enum E_MONITOR_MODE { single=1, dual=2, quad=4 };
  E_MONITOR_MODE m_monitor_mode = E_MONITOR_MODE::single; // R.S. FIXME hardcode
  std::map<uint32_t, std::vector<uint64_t>> m_monitor_config_dynamic{ };

  // Configuration -> HARDCODE... should come from configuration.
  std::map<uint32_t, std::vector<uint64_t>> m_monitor_config_basic {
    { 0, { 0 } },
    { 1, { 1 } },
    { 2, { 2 } },
    { 3, { 3 } }
  };
  std::map<uint32_t, std::vector<uint64_t>> m_monitor_config {
    { 0, { 0 , 1 , 2 , 3  } }, // monitor 0 - egroup 0
    { 1, { 4 , 5 , 6 , 7  } }, // monitor 1 - egroup 1
    { 2, { 8 , 9 , 10, 11 } }, // monitor 2 - egroup 2
    { 3, { 12, 13, 14, 15 } }  // monitor 3 - egroup 3
  };
  const uint32_t m_datasize = 3;
  const uint32_t m_headersize = sizeof(felix::base::FromFELIXHeader);

  // NETIO
  netio::context * m_context; // context
  std::string m_felixHost;    // hostname
  uint16_t m_felixTXPort;     // TX port (ususally 12340)
  uint16_t m_felixRXPort;     // RX port (ususally 12345)
  std::thread m_netio_bg_thread;
  std::map<uint64_t, netio::low_latency_subscribe_socket*> m_sub_sockets; // subscribe sockets.
  std::map<uint64_t, netio::low_latency_send_socket*> m_send_sockets;     // send sockets.

  size_t m_activeChannels;

  // Statistic thread options
  size_t m_sensitivity;
  size_t m_delay;
  size_t m_queueSize;

  // Verbosity
  bool m_verbose;

  // Queues and the stability check threads
  std::map<uint64_t, SharedQueue> m_pcqs; // Queues for elink RX.
  std::vector<QueueMonitor> m_monitors;   // Queue monitoring threads.

  // Other statistics for channels:
  std::map<uint64_t, uint32_t> m_msgErrors;

  std::mutex m_mutex;
};

#endif

