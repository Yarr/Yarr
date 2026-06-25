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

  class AsyncReg {
  public:
    /// Send and read back register data
    /**
     * Parameters:
     * @param ctxt Async context.
     * @param send Function to send read sequence.
     * @param filter Return true if this packet is interesting.
     * @param process Process packet to extract register data.
     */
    AsyncReg(AsyncContext &ctxt,
             std::function<void (TxCore &)> send,
             std::function<bool (const RawData &)> filter,
             std::function<void (const RawData &)> process);
    std::future<void> result;
  };

} // End namespace AsyncReg

#endif
