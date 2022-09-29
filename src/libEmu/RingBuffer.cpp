/*
 * Author: N. Whallon <alokin@uw.edu>
 * Date: 2017-VI
 * Description: a class to facilitate communication between programs using a typical buffer
 */

#include "RingBuffer.h"

#include <cstring>
#include <iostream>
#include <sstream>

RingBuffer::RingBuffer(uint32_t size)
{
    buffer.resize(size);

    read_index = 0;
    write_index = 0;
}

RingBuffer::~RingBuffer() = default;

void RingBuffer::write32(uint32_t word)
{
    // wait if the write index would catch up to the read index
    std::unique_lock<std::mutex> lk(mtx);
    cv.wait(lk, [&] {
        auto next_write = (write_index + 1) % buffer.size();
        return next_write != read_index;
      });

    // do the write
    buffer[write_index] = word;

    // update the write pointer
    write_index = (write_index + 1) % buffer.size();

    // notify the cv that a read/write has occured
    cv.notify_all();
}

uint32_t RingBuffer::read32()
{

    // wait if the read pointer has caught up to the write pointer
    std::unique_lock<std::mutex> lk(mtx);
    cv.wait(lk, [&] { return read_index != write_index; });

    // do the read
    uint32_t word = buffer[read_index];

    // update the read pointer
    read_index = (read_index + 1) % buffer.size();

    // notify the cv that a read/write has occured
    cv.notify_all();

    return word;
}

uint32_t RingBuffer::readBlock32(uint32_t* buf, uint32_t length)
{
    auto occupancy = this->getCurSize();
    if ((length * element_size) > occupancy) {
        std::cerr << __PRETTY_FUNCTION__
            << " -> ERROR : not enough data in buffer! This should not be possible! Requested: "
            << length * element_size << ", actual: " << occupancy << std::endl;
        return 0;
    }

#if 1
    for (uint32_t i = 0; i < length; i++) {
      buf[i] = this->read32();
    }

return 1;
#else
    // This needs more testing (should at least have a lock),
    // but this method is not used, so go for the simpler method
    //
    // 0 1 2 3 4 5 6 7
    //     w     r | |
    if ((read_index + length) >= buffer.size()) {
        //split transfer
        // length of first part

        uint32_t length_p1 = buffer.size() - read_index;
        uint32_t length_p2 = length - length_p1;
        memcpy(&buf[0], &buffer[read_index], length_p1 * element_size);
        memcpy(&buf[length_p1], &buffer[0], length_p2 * element_size);
    } else {
        //one transfer
        memcpy(&buf[0], &buffer[read_index], element_size * length);
    }

    // update the read pointer
    read_index = (read_index + length) % buffer.size();

    // notify the cv that a read/write has occured
    cv.notify_all();

    return 1;
#endif
}

bool RingBuffer::isEmpty()
{
    if (write_index == read_index)
    {
        return true;
    }
    return false;
}

uint32_t RingBuffer::getCurSize() {
    uint32_t w = write_index;
    uint32_t test_w, r;

    // Don't read inconsistent numbers, but also don't want to use mutex
    do {
        test_w = w;
        r = read_index;
        w = write_index;
    } while(test_w != w);

    // Extra size added to make sure it's positive
    //  which makes a difference if buffer size is not power of 2
    size_t offset = w + buffer.size() - r;

    auto element_count = offset % (buffer.size());
    return element_count * element_size;
}

void RingBuffer::dump()
{
    std::stringstream ss;
    for (uint32_t i = 0; i < buffer.size(); i++)
    {
      ss << "[" << i << "]\t\t0x" << std::hex << buffer[i] << std::dec;
      if(i == read_index) {
        ss << " READ";
      }
      if(i == write_index) {
        ss << " WRITE";
      }
      ss << '\n';
    }
    std::cout << ss.str();
}
