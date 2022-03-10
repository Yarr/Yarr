#ifndef QUEUE_MONITOR_HH_
#define QUEUE_MONITOR_HH_

/********************************
 * QueueMonitor
 * Author: Roland.Sipos@cern.ch
 * Description: Monitors a set of
 *   assigned SPSC circular buffers.
 *   Quite thrilling...
 * Date: November 2017
*********************************/

#include "ProducerConsumerQueue.h"

#include <thread>
#include <iostream>
#include <map>
#include <vector>
#include <bitset>
#include <string>

#include "logging.h"

class QueueMonitor {
public:
  /* Explicitly using the default constructor to
  *  underline the fact that it does get called. */
  QueueMonitor(size_t monitorID, const std::vector<uint64_t>& queueIDs,
               std::map<uint64_t, std::shared_ptr<folly::ProducerConsumerQueue<uint32_t>>>& pcqs,
               size_t sensitivity, size_t delay) :
    m_monitorID(monitorID), m_queueIDs(queueIDs),
    m_pcqs(pcqs)
  {
    std::cerr << "### QueueMonitor::QueueMonitor() ID: " << m_monitorID << "\n"
              << "     -> queue(s) to monitor: [ ";
    for (auto i : m_queueIDs){
      std::cerr << i << " ";
    }
    std::cerr << "]"<< std::endl;
  }

  ~QueueMonitor() {
    std::cerr << "### QueueMonitor::~QueueMonitor() -> Joining "
              << whatAmI << "[" << m_monitorID << "] thread...\n";
  }

  // This may be a bad idea! Should not move at all? Or this is good, just passing the refs?
  // Otherwise force delete: QueueMonitor(QueueMonitor&& other) = delete;
  QueueMonitor(QueueMonitor&& other) //noexcept {
    : m_monitorID( other.m_monitorID ),
      m_queueIDs( std::ref(other.m_queueIDs) ),
      m_pcqs( std::ref(other.m_pcqs) ) {
  }

  const std::map<uint64_t, bool>& getStability(){ return std::ref(m_stability); }

private:
  static logging::Logger &logger() {
    static logging::LoggerStore instance = logging::make_log("NetioHW::QueueMonitor");
    return *instance;
  }

  // Actual thread handler members.
  std::string whatAmI = "plain";

  size_t m_monitorID;

  // Containers to calculate the stability of monitored queues.
  const std::vector<uint64_t>& m_queueIDs; // assigned queues
  std::map<uint64_t, std::shared_ptr<folly::ProducerConsumerQueue<uint32_t>>>& m_pcqs;
  std::map<uint64_t, bool> m_stability;
};

#endif /* QUEUE_MONITOR_HH_ */

