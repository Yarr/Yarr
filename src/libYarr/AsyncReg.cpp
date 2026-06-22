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
struct ReadRegState {
  using clk = std::chrono::steady_clock;

  std::unique_ptr<std::atomic<ReadRegSM>> state;

  clk::time_point last_state_change;

  std::function<void (TxCore &)> send_cb;
  std::function<bool (const RawData &)> filter_cb;
  std::function<void (const RawData &)> process_cb;

  std::chrono::milliseconds ms_timeout;

  std::promise<void> promise;

  ReadRegState(std::function<void (TxCore &)> send,
               std::function<bool (const RawData &)> filter,
               std::function<void (const RawData &)> process,
               std::chrono::milliseconds timeout,
               std::promise<void> &&promise)
    : state(std::make_unique<std::atomic<ReadRegSM>>(ReadRegSM::INIT)),
      last_state_change{clk::now()},
      send_cb(send),
      filter_cb(filter),
      process_cb(process),
      ms_timeout(timeout),
      promise(std::move(promise))
  {
    logger->trace("Init async read reg state");
  }

  ReadRegState() = delete;
  ReadRegState(const ReadRegState &) = delete;
  ReadRegState(ReadRegState &&rhs) = default;
  ReadRegState &operator=(const ReadRegState &) = delete;
  ReadRegState &operator=(ReadRegState &&rhs) = default;

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
      try {
        throw std::runtime_error("Timeout exception");
      } catch(...) {
        promise.set_exception(std::current_exception());
        newState(ReadRegSM::TIMEOUT);
        return;
      }
      logger->critical("Async read reg failed to set timeout");
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
    logger->trace("Async read reg check data");
    if(*state != ReadRegSM::READY_FOR_DATA) {
      // Not interested
      logger->trace("Async read received data in wrong state");
      return false;
    }
    if(filter_cb(rawData)) {
      newState(ReadRegSM::FOUND_DATA);
      process_cb(rawData);
      promise.set_value();
      newState(ReadRegSM::READ_COMPLETE);
      return true;
    }
    return false;
  }
};

namespace AsyncAccess {
  namespace detail {
    /// Store info about what is being waited for etc.
    struct AsyncContextImpl {
      AsyncContextImpl() = delete;
      AsyncContextImpl(TxCore &tx, RxCore &rx);

      void dispatchNewData(RxCore &rxCore);

      /// Thread
      void run(std::stop_token stoken);

      void dispatchRead(ReadRegState new_read);

      RxCore &rxCore;
      TxCore &txCore;

      /// Protect access to the list
      std::mutex sm_mutex;
      std::vector<ReadRegState> allSMs;

      /// While this object exists the thread is running
      std::jthread thread;
    };
  }
}

AsyncAccess::detail::AsyncContextImpl::AsyncContextImpl(TxCore &tx, RxCore &rx)
  : rxCore(rx),
    txCore(tx),
    allSMs{},

    // Run last so we know everything else is set up
    thread([this](std::stop_token stoken){ run(stoken); })
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
      bool good = sm.checkData(*data);
      if (!good) {
        // Let the next sm look
        continue;
      }
      break;
    }
  }
}

void AsyncAccess::detail::AsyncContextImpl::dispatchRead(ReadRegState new_read)
{
  std::lock_guard<std::mutex> lm(sm_mutex);
  allSMs.push_back(std::move(new_read));
}

void AsyncAccess::detail::AsyncContextImpl::run(std::stop_token stoken)
{
  logger->trace("Async read run thread");
  while(!stoken.stop_requested()) {
    {
      std::lock_guard<std::mutex> lk(sm_mutex);
      for(auto &sm: allSMs) {
        sm.takeStep(txCore);
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

AsyncReg::AsyncReg(AsyncContext &ctxt,
                   std::function<void (TxCore &)> send,
                   std::function<bool (const RawData &)> filter,
                   std::function<void (const RawData &)> process,
                   std::chrono::milliseconds ms_timeout)
{
  logger->trace("Async read creation");
  std::promise<void> result_promise;

  result = result_promise.get_future();

  ReadRegState state(
        send,
        filter, process,
        ms_timeout,
        std::move(result_promise));

  ctxt.impl->dispatchRead(std::move(state));
  logger->trace("Async read submitted");
}
