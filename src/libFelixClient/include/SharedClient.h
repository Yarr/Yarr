#ifndef SHAREDCLIENT_H
#define SHAREDCLIENT_H

#include <shared_mutex>

// felix client
#include "felix/felix_client_thread.hpp"
#include "felix/felix_client_properties.h"

// Yarr
#include "storage.hpp" // json
#include "FelixTools.h"

// Followed examples from 
// https://gitlab.cern.ch/atlas-tdaq-felix/felix-client/-/blob/master/examples/felix_client_thread_mt_subscribe.cpp
// https://gitlab.cern.ch/atlas-tdaq-felix/felix-client/-/blob/master/examples/felix_client_thread_mt_sca_loopback.cpp

class SharedClient {
  public:

    explicit SharedClient(const json &cfg);
    ~SharedClient();

    using FelixID_t = FelixTools::FelixID_t;
    using DataCallback = std::function<void(FelixID_t, const uint8_t*, size_t, uint8_t)>;

    FelixClientThread* getClient() { return m_client.get(); }

    void subscribe(FelixID_t fid, const DataCallback& callback, bool enable=true);
    void resubscribe(FelixID_t fid, bool enable=true);
    void unsubscribe(FelixID_t fid);

    void enableTx(FelixID_t fid);
    void disableTx(FelixID_t fid);
    void enableRx(FelixID_t fid);
    void disableRx(FelixID_t fid);
    // enable/disable all channels
    void enableTx(); // enable all Tx links
    void disableTx(); // disable all Tx links
    void enableRx(); // enable all Rx links
    void disableRx(); // disable all Rx links

    bool isTxEnabled(FelixID_t fid);
    bool isRxEnabled(FelixID_t fid);

  private:

    using DataCallbacks = std::unordered_map<uint64_t, DataCallback>;

    DataCallbacks m_callbacks;

    FelixID_t m_last_id = -1;
    DataCallbacks::iterator m_last_it;

    std::unique_ptr<FelixClientThread> m_client;

    mutable std::shared_mutex mtx;

    std::map<FelixID_t, bool> m_txEnables;
    std::map<FelixID_t, bool> m_rxEnables;

    // Felix client callbacks    
    void on_init() {}
    void on_connect(FelixID_t fid);
    void on_disconnect(FelixID_t fid);
    void on_data_received(FelixID_t fid, const uint8_t* data, size_t size, uint8_t status);
};

#endif