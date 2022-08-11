#include "catch.hpp"

#include <thread>

#include "RingBuffer.h"

TEST_CASE("RingBufferSimple", "[emu][ring]") {
  const size_t len = 16;
  RingBuffer rb(len);

  CHECK (rb.isEmpty() == true);

  rb.write32(0x12341234);
  CHECK (rb.isEmpty() == false);
  CHECK (rb.read32() == 0x12341234);
  CHECK (rb.isEmpty() == true);

  // Can't do more than 2 at the moment...
  for(size_t i=0; i<2; i++) {
    rb.write32(0x12341234);
  }

  CHECK (rb.isEmpty() == false);
  // Cur size is in bytes
  CHECK (rb.getCurSize() == (2 * 4));
}

TEST_CASE("RingBufferThread", "[emu][ring]") {
  const size_t len = 16;
  const size_t send_count = 1600;
  RingBuffer rb(len);

  auto data = [](size_t i) { return (i*i) % 0xffffffff; };

  std::thread sender
    ([&] () {
      for(size_t i=0; i<send_count; i++) {
        rb.write32(data(i));
      }
    });

  std::thread receiver
    ([&] () {
      for(size_t i=0; i<send_count; i++) {
        CHECK (rb.read32() == data(i));
      }
    });

  sender.join();
  receiver.join();

  CHECK (rb.isEmpty() == true);
}
