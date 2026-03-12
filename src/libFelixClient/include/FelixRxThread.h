#ifndef FELIXRXTHREAD_H
#define FELIXRXTHREAD_H

#include <memory>
#include <map>
#include <thread>
#include <span>
// felix client
#ifdef YARR_CONFIG_FELIX_PROXY
#include "felix_proxy/ClientThread.h"
using FelixClientThread = felix_proxy::ClientThread;
#else
#include "felix/felix_client_thread.hpp"
#endif

#include "FelixTools.h"
#include "RawData.h"
#include "ClipBoard.h"

class FelixRxThread {

  public:

    using FelixID_t = FelixTools::FelixID_t;

    FelixRxThread(FelixClientThread::ConfigV2 fcConfig, const std::vector<FelixID_t>& fid_list, size_t maxMessageSize);
    ~FelixRxThread();

    void run();
    void stop();

    void subscribe();
    void unsubscribe();

    bool allConnected() const;
    bool allDisconnected() const;

    void enableChannel(FelixID_t fid);
    void enableChannel();
    void disableChannel(FelixID_t fid);
    void disableChannel();

    bool channelIsEnabled(FelixID_t fid) { return m_enables[fid]; }

    void flush(bool doflush) { m_doFlushBuffer = doflush; }

    void clearRawData();

    RawDataPtr readData();

    uint32_t getDataRate() const;
    uint32_t getCurCount() const;
    uint32_t getCurBytes() const;

    void resetStatistics();
    void computeRates(const double& time);
    void reportStatistics();

    std::vector<FelixID_t> getFIDs() const {
      std::vector<FelixID_t> fids;
      for (const auto& [fid, stats] : m_fidStats) {
        fids.push_back(fid);
      }
      return fids;
    }

    std::string getThreadID() const {
      std::stringstream ss;
      ss << "0x" << std::hex << thread_ptr->get_id();
      return ss.str();
    }

  private:

    std::unique_ptr<std::thread> thread_ptr;

    std::unique_ptr<FelixClientThread> m_client;

    ClipBoard<RawData> m_rawData;

    std::atomic<bool> m_doFlushBuffer {false};

    // Receiver queue status
    std::atomic<uint64_t> m_total_data_in {0}; // total number of data received
    std::atomic<uint64_t> m_total_data_out {0}; // total number of data read out
    std::atomic<uint64_t> m_total_bytes_in {0};
    std::atomic<uint64_t> m_total_bytes_out {0};

    std::map<FelixID_t, bool> m_enables; // enable flag for each elink
    std::map<FelixID_t, FelixTools::QueueStatistics> m_fidStats; // link statistics

    size_t m_maxMessageSize {0}; // if set to >0, on_data drops messages with larger sizes

    void on_init();
    void on_connect(FelixID_t fid);
    void on_disconnect(FelixID_t fid);
    void on_data_received(FelixID_t fid, std::span<const uint8_t>, uint8_t status);
};

#endif
