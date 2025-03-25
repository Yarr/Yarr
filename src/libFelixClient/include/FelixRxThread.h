#ifndef FELIXRXTHREAD_H
#define FELIXRXTHREAD_H

#include <memory>
#include <map>
#include <thread>

#include "SharedClient.h"
#include "FelixTools.h"
#include "RawData.h"
#include "ClipBoard.h"

class FelixRxThread {

  public:

    using FelixID_t = FelixTools::FelixID_t;

    FelixRxThread(
      std::shared_ptr<SharedClient> client,
      const std::vector<FelixID_t>& fid_list,
      size_t maxMessageSize = 0
    );

    ~FelixRxThread();

    void run();
    void stop();

    void flush(bool doflush) { m_doFlushBuffer = doflush; }

    RawDataPtr readData();

    uint32_t getDataRate() const;
    uint32_t getCurCount() const;

    void runMonitor(uint32_t interval_ms, uint64_t queue_limit, bool print_info=false);
    void stopMonitor();

    std::vector<FelixID_t> getFIDs() const {
      std::vector<FelixID_t> fids;
      for (const auto& [fid, stats] : m_fidStats) {
        fids.push_back(fid);
      }
      return fids;
    }

    std::string getThreadID() const {
      std::stringstream ss;
      ss << thread_ptr->get_id();
      return ss.str();
    }

  private:

    std::unique_ptr<std::thread> thread_ptr;

    std::shared_ptr<SharedClient> m_client;  

    ClipBoard<RawData> m_rawData;

    std::atomic<bool> m_doFlushBuffer {false};

    // Receiver queue status
    std::atomic<uint64_t> m_total_data_in {0}; // total number of data received
    std::atomic<uint64_t> m_total_data_out {0}; // total number of data read out
    std::atomic<uint64_t> m_total_bytes_in {0};
    std::atomic<uint64_t> m_total_bytes_out {0};

    std::map<FelixID_t, FelixTools::QueueStatistics> m_fidStats; // link statistics

    std::thread m_monitor_thread;
    std::atomic<bool> m_runMonitor {false};

    size_t m_maxMessageSize {0}; // if set to >0, on_data drops messages with larger sizes

    void subscribe();
    void on_data_callback(FelixID_t fid, const uint8_t* data, size_t size, uint8_t status);
};

#endif