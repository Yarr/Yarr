#include "AsyncReg.h"

#include <thread>

#include "RxCore.h"
#include "TxCore.h"
#include "logging.h"

namespace {
auto logger = logging::make_log("AsyncReg");
}

enum class ReadRegSM {
  INIT,
  // Wait for some data
  READY_FOR_DATA,
  FOUND_DATA,
  READ_COMPLETE,

  TIMEOUT,
};

/// State for one read register command
struct ReadRegStateCommon {
  using clk = std::chrono::steady_clock;

  std::unique_ptr<std::atomic<ReadRegSM>> state;

  clk::time_point last_state_change;

  std::function<void (TxCore &)> send_cb;
  std::function<bool (const RawData &)> filter_cb;

  std::chrono::milliseconds ms_timeout;

  ReadRegStateCommon(std::function<void (TxCore &)> send,
               std::function<bool (const RawData &)> filter,
               std::chrono::milliseconds timeout)
    : state(std::make_unique<std::atomic<ReadRegSM>>(ReadRegSM::INIT)),
      last_state_change{clk::now()},
      send_cb(send),
      filter_cb(filter),
      ms_timeout(timeout)
  {
    logger->trace("Init async read reg state (common)");
  }

  ReadRegStateCommon() = delete;
  ReadRegStateCommon(const ReadRegStateCommon &) = delete;
  ReadRegStateCommon(ReadRegStateCommon &&rhs) = default;
  ReadRegStateCommon &operator=(const ReadRegStateCommon &) = delete;
  ReadRegStateCommon &operator=(ReadRegStateCommon &&rhs) = default;
  virtual ~ReadRegStateCommon() = default;

  /// Set exception on promise
  virtual void notifyTimeout() = 0;

  /// Use call back and set promise
  virtual void processData(RawData &) = 0;

  void newState(ReadRegSM s) {
    *state = s;
    last_state_change = clk::now();

    logger->trace("State: {}", stateName());
  }

  const char *stateName() const {
    switch(*state) {
    case ReadRegSM::INIT: return "INIT";
    case ReadRegSM::READY_FOR_DATA: return "READY_FOR_DATA";
    case ReadRegSM::FOUND_DATA: return "FOUND_DATA";
    case ReadRegSM::READ_COMPLETE: return "READ_COMPLETE";
    case ReadRegSM::TIMEOUT: return "TIMEOUT";
    default: return "UNEXPECTED";
    }
  }

  void takeStep(TxCore &tx) {
    logger->trace("Async read reg step");

    if((*state != ReadRegSM::TIMEOUT)
       && (clk::now() - last_state_change) > ms_timeout) {
      logger->trace("Async read reg timeout from {}", stateName());
      newState(ReadRegSM::TIMEOUT);
      return notifyTimeout();
    }

    switch(*state) {
    case ReadRegSM::INIT:
      send_cb(tx);
      newState(ReadRegSM::READY_FOR_DATA);
      break;
    default:
      // READING handled elsewhere
      break;
    }
  }

  bool checkData(RawData &rawData) {
    // logger->trace("Async read reg check data");
    if(*state != ReadRegSM::READY_FOR_DATA) {
      // Not interested
      // logger->trace("Async read received data in wrong state {}", stateName());
      return false;
    }
    if(filter_cb(rawData)) {
      newState(ReadRegSM::FOUND_DATA);
      processData(rawData);
      newState(ReadRegSM::READ_COMPLETE);
      return true;
    }
    return false;
  }
};

template<typename RegType>
struct ReadRegState : ReadRegStateCommon {
  std::function<RegType (const RawData &)> process_cb;

  std::promise<RegType> promise;

  ReadRegState(std::function<void (TxCore &)> send,
               std::function<bool (const RawData &)> filter,
               std::function<RegType (const RawData &)> process,
               std::chrono::milliseconds timeout,
               std::promise<RegType> &&promise)
    : ReadRegStateCommon(send, filter, timeout),
      process_cb(process),
      promise(std::move(promise))
  {
    logger->trace("Init async read reg state");
  }

  ReadRegState() = delete;
  ReadRegState(const ReadRegState &) = delete;
  ReadRegState(ReadRegState &&rhs) = default;
  ReadRegState &operator=(const ReadRegState &) = delete;
  ReadRegState &operator=(ReadRegState &&rhs) = default;

  void notifyTimeout() override {
    try {
      throw std::runtime_error("Timeout exception");
    } catch(...) {
      promise.set_exception(std::current_exception());
      return;
    }
  }

  void processData(RawData &rawData) override {
    if constexpr (std::is_void_v<RegType>) {
      process_cb(rawData);
      promise.set_value();
    } else {
      promise.set_value(process_cb(rawData));
    }
  }
};

namespace AsyncAccess {
  namespace detail {
    /// Store info about what is being waited for etc.
    struct AsyncContextImpl {
      AsyncContextImpl() = delete;
      AsyncContextImpl(TxCore &tx, RxCore &rx);
      ~AsyncContextImpl() {
        thread_running = false;
        thread.join();
      }

      void dispatchNewData(RxCore &rxCore);

      /// Thread
      void run(std::atomic<bool> &stoken);

      void dispatchRead(std::unique_ptr<ReadRegStateCommon> new_read);

      RxCore &rxCore;
      TxCore &txCore;

      /// Protect access to the list
      std::mutex sm_mutex;
      std::vector<std::unique_ptr<ReadRegStateCommon>> allSMs;

      /// While this object exists the thread is running
      std::atomic<bool> thread_running;
      std::thread thread;
    };
  }
}

AsyncAccess::detail::AsyncContextImpl::AsyncContextImpl(TxCore &tx, RxCore &rx)
  : rxCore(rx),
    txCore(tx),
    allSMs{},
    thread_running{true},
    // Run last so we know everything else is set up
    thread([this](){ run(thread_running); })
{
}

void AsyncAccess::detail::AsyncContextImpl::dispatchNewData(RxCore &rxCore) {
  RawDataContainer rdc(LoopStatus{});

  std::vector<RawDataPtr> dataVec = rxCore.readData();
  if (dataVec.empty()) {
    return;
  }

  for(auto data : dataVec) {
    std::lock_guard<std::mutex> lk(sm_mutex);
    for(auto &sm: allSMs) {
      bool good = sm->checkData(*data);
      if (!good) {
        // Let the next sm look
        continue;
      }
      break;
    }
  }
}

void AsyncAccess::detail::AsyncContextImpl::dispatchRead(std::unique_ptr<ReadRegStateCommon> new_read)
{
  std::lock_guard<std::mutex> lm(sm_mutex);
  allSMs.push_back(std::move(new_read));
}

void AsyncAccess::detail::AsyncContextImpl::run(std::atomic<bool> &running_flag)
{
  logger->trace("Async read run thread");
  size_t list_size = 0;
  while(running_flag) {
    {
      std::lock_guard<std::mutex> lk(sm_mutex);
      if(list_size != allSMs.size()) {
        logger->trace("New list size {} -> {}", list_size, allSMs.size());
        list_size = allSMs.size();
      }
      for(auto &sm: allSMs) {
        sm->takeStep(txCore);
      }
      for(size_t i=0; i<allSMs.size(); i++) {
        auto &sm = *allSMs[i];
        if((*sm.state == ReadRegSM::READ_COMPLETE)
           || (*sm.state == ReadRegSM::READ_COMPLETE)) {
          allSMs.erase(allSMs.begin() + i);
          // // Only do one at a time
          // break;
        }
      }
    }

    dispatchNewData(rxCore);
  }
  logger->trace("Async read thread complete");
}

using namespace AsyncAccess;
using namespace AsyncAccess::detail;

AsyncContext::AsyncContext(TxCore&tx, RxCore&rx)
  : impl(std::make_unique<AsyncContextImpl>(tx, rx))
{}

// Define here due to unique_ptr to pimpl
AsyncContext::~AsyncContext() = default;

template<typename RegType>
AsyncReadData<RegType>::AsyncReadData(AsyncContext &ctxt,
                   std::function<void (TxCore &)> send,
                   std::function<bool (const RawData &)> filter,
                   std::function<RegType (const RawData &)> process,
                   std::chrono::milliseconds ms_timeout)
{
  logger->trace("Async read creation");

  std::promise<RegType> result_promise;

  result = result_promise.get_future();

  auto state = std::make_unique<ReadRegState<RegType>>(
        send,
        filter, process,
        ms_timeout,
        std::move(result_promise));

  ctxt.impl->dispatchRead(std::move(state));
  logger->trace("Async read submitted");
}

namespace AsyncAccess {

// Instantiate some particular variants (matching ReadDataType)
template class AsyncReadData<void>;
template class AsyncReadData<uint16_t>;
template class AsyncReadData<uint32_t>;

}
