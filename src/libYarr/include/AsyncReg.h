#ifndef YARR_ASYNC_REG_H
#define YARR_ASYNC_REG_H

#include <functional>
#include <future>
#include <memory>

class RawData;
class RxCore;
class TxCore;

namespace AsyncAccess {
  namespace detail {
    struct AsyncContextImpl;
  }

  /**
   * Context for a bunch of async accesses.
   *
   * Basically, this stores info about what is being waited for etc.
   * We run a thread in the background which reads back data and
   * processes it in arrival order.
   *
   * The thread keeps running until all data it has been given is
   * processed.
   */
  class AsyncContext {
  public:
    AsyncContext() = delete;
    ~AsyncContext();
    AsyncContext(TxCore &, RxCore &);

    std::unique_ptr<detail::AsyncContextImpl> impl;
  };

  template<typename Response>
  class AsyncReadData {
  public:
    /// Send and read back register data
    /**
     * Parameters:
     * @param ctxt Async context.
     * @param send Function to send read sequence.
     * @param filter Return true if this packet is interesting.
     * @param process Process packet to extract register data.
     */
    AsyncReadData(AsyncContext &ctxt,
             std::function<void (TxCore &)> send,
             std::function<bool (const RawData &)> filter,
             std::function<Response (const RawData &)> process,
             std::chrono::milliseconds ms_timeout = std::chrono::milliseconds(100));
    std::future<Response> result;
  };

  using AsyncReg = AsyncReadData<void>;
  using AsyncReg32 = AsyncReadData<uint32_t>;

} // End namespace AsyncReg

#endif
