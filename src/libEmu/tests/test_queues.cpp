#include "catch.hpp"

#include <thread>

#include "RingBuffer.h"

TEST_CASE("RingBufferSimple", "[emu][ring]") {
  const size_t len = 16;
  RingBuffer rb(len);

  CHECK (rb.isEmpty() == true);
  CHECK (rb.getCurSize() == 0);

  rb.write32(0x12341234);

  CHECK (rb.isEmpty() == false);
  CHECK (rb.read32() == 0x12341234);

  CHECK (rb.isEmpty() == true);
  CHECK (rb.getCurSize() == 0);

  for(size_t i=0; i<len - 1; i++) {
    rb.write32(1 << i);
  }

  CHECK (rb.isEmpty() == false);
  // Cur size is in bytes
  CHECK (rb.getCurSize() == (len - 1) * 4);
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

TEST_CASE("RingBufferBlocks", "[emu][ring]") {
  const size_t len = 19;
  const size_t block_len = 13;

  const size_t send_count = len * block_len;
  const size_t receive_count = len;

  RingBuffer rb(len);

  auto data = [](size_t i) { return (i*(i+1)) % 0xffffffff; };

  std::thread sender
    ([&] () {
      for(size_t i=0; i<send_count; i++) {
        rb.write32(data(i));
      }
    });

  std::thread receiver
    ([&] () {
      for(size_t i=0; i<receive_count; i++) {
        std::array<uint32_t, block_len> buf;
        auto read_success = rb.readBlock32(buf.data(), block_len);
        // CHECK (read_success);
        if(!read_success) {
          // retry
          i -= 1;
          continue;
        }

        for(size_t j=0; j<block_len; j++) {
          CAPTURE(i, j);
          CHECK (buf[j] == data((i * block_len) + j));
        }
      }
    });

  sender.join();
  receiver.join();

  CHECK (rb.isEmpty() == true);
}
