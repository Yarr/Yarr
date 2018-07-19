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

#include "felixbase/client.hpp"

#include <netio/netio.hpp>

#include <thread>
#include <iostream>
#include <vector>
#include <bitset>
#include <string>

class QueueMonitor {
public:
  /* Explicitly using the default constructor to
  *  underline the fact that it does get called. */
  QueueMonitor(size_t monitorID, const std::vector<uint64_t>& queueIDs,
               std::map<uint64_t, std::shared_ptr<folly::ProducerConsumerQueue<uint32_t>>>& pcqs,
               size_t sensitivity, size_t delay) :
    m_worker_thread(), m_monitorID(monitorID), m_queueIDs(queueIDs),
    m_pcqs(pcqs), m_sensitivity(sensitivity), m_delay(delay)
  {
    std::cout << "### QueueMonitor::QueueMonitor() ID: " << m_monitorID << "\n"
              << "     -> sensitivity: " << m_sensitivity << " matches"<< "\n"
              << "     -> delay: " << m_delay << " [us]"<< "\n"
              << "     -> queue(s) to monitor: [ ";
    for (auto i : m_queueIDs){
      std::cout << i << " ";
    }
    std::cout << "]"<< std::endl;
  }

  ~QueueMonitor() {
    std::cout << "### QueueMonitor::~QueueMonitor() -> Joining "
              << whatAmI << "[" << m_monitorID << "] thread...\n";
    m_stop_thread = true;
    if (m_worker_thread.joinable())
      m_worker_thread.join();
  }

  // This may be a bad idea! Should not move at all? Or this is good, just passing the refs?
  // Otherwise force delete: QueueMonitor(QueueMonitor&& other) = delete;
  QueueMonitor(QueueMonitor&& other) //noexcept {
    : m_monitorID( other.m_monitorID ),
      m_queueIDs( std::ref(other.m_queueIDs) ),
      m_pcqs( std::ref(other.m_pcqs) ),
      m_sensitivity( other.m_sensitivity ),
      m_delay( other.m_delay ) {
  }

  // Actually start the thread. Notice Move semantics of thread!
  void startMonitor() {
    whatAmI = "monitor";
    std::cout << "### QueueMonitor::startMonitor() -> Thread moved and starts as a " << whatAmI << " ...\n";
    m_worker_thread = std::thread(&QueueMonitor::MonitorQueues, this);
  }

  const std::map<uint64_t, bool>& getStability(){ return std::ref(m_stability); }

private:
  // Actual thread handler members.
  std::string whatAmI = "plain";
  std::thread m_worker_thread;
  std::atomic_bool m_stop_thread{ false };
  std::mutex m_mutex;

  size_t m_monitorID;
  bool m_verbose = false;

  // Containers to calculate the stability of monitored queues.
  const std::vector<uint64_t>& m_queueIDs; // assigned queues
  std::map<uint64_t, std::shared_ptr<folly::ProducerConsumerQueue<uint32_t>>>& m_pcqs;
  std::map<uint64_t, bool> m_stability;
  std::map<uint64_t, size_t> m_sizeBefore;
  std::map<uint64_t, size_t> m_sizeAfter;
  std::map<uint64_t, size_t> m_match;

  // Statistic thread options
  size_t m_sensitivity;
  size_t m_delay;

  // The main working thread.
  // Statistics thread for monitoring the stability of the Queues.
  void MonitorQueues() {
    // Reset maps
    for (uint32_t i=0; i<m_queueIDs.size(); ++i){
      size_t qid = m_queueIDs[i];
      m_stability[qid] = false;
      m_sizeBefore[qid] = 0;
      m_sizeAfter[qid] = 0;
      m_match[qid] = 0;
    }

    // Release...
    while(!m_stop_thread) {
      m_mutex.lock();
      for (uint32_t i=0; i<m_queueIDs.size(); ++i){ // For assigned channels.
        uint64_t qid = m_queueIDs[i];
        m_sizeAfter[qid] = m_pcqs[qid]->sizeGuess(); // store current size.
        if (m_sizeAfter[qid] == m_sizeBefore[qid]){  // if size is the same, match++
          m_match[qid]++;
        } else {                                   // otherwise null out match, stability false
          m_match[qid]=0; m_stability[qid]=false;
        }
        if ( m_match[qid] > m_sensitivity ) {       // If match is higher than a threshold, Queue is stable!
          m_match[qid]=m_sensitivity; // Don't let the size_t overflow. Keep it on the sensitivity level.
          m_stability[qid]=true;
        }
        m_sizeBefore[qid] = m_sizeAfter[qid];        // store currrent size as before for next iteration.
        if (m_verbose){
          std::cout << "MONITOR[" << m_monitorID << "] " << "QUEUE[" << qid<< "]"
                    << " match: " << m_match[qid] << " before: " << m_sizeBefore[qid]
                    << " after: " << m_sizeAfter[qid] << " stability: " << m_stability[qid] << " ..." << std::endl;
        }
      }
      m_mutex.unlock();
      std::this_thread::sleep_for(std::chrono::microseconds(m_delay));
    }
  }
};

#endif /* QUEUE_MONITOR_HH_ */

