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
      ClipBoard<RawData>* data_buffer
    );

    ~FelixRxThread();

    void run();
    void stop();

    void subscribe();
    void on_data_callback(FelixID_t fid, const uint8_t* data, size_t size, uint8_t status);

    void flush(bool doflush) { m_doFlushBuffer = doflush; }

  private:

    std::unique_ptr<std::thread> thread_ptr;

    std::shared_ptr<SharedClient> m_client;  

    // For now multiple FelixRxThreads write to the same RawData ClipBoard from FelixRxCore
    // TODO: separate RawData ClipBoard in FelixRxThread consumed by separate DataProcessors?
    ClipBoard<RawData>* m_rawData; // owned by FelixRxCore

    std::atomic<bool> m_doFlushBuffer {false};

    std::map<FelixID_t, FelixTools::QueueStatistics> m_fidStats; // link statistics
    size_t m_maxMessageSize {0}; // if set to >0, on_data drops messages with larger sizes

    // Receiver queue status
    std::atomic<uint64_t> m_total_data_in {0}; // total number of data received
    std::atomic<uint64_t> m_total_bytes_in {0};
};

#endif