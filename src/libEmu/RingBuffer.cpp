/*
 * Author: N. Whallon <alokin@uw.edu>
 * Date: 2017-VI
 * Description: a class to facilitate communication between programs using a typical buffer
 */

#include "RingBuffer.h"

RingBuffer::RingBuffer(uint32_t size)
{
    ringbuffer_size = size;
    element_size = sizeof(uint32_t);

    buffer = (uint32_t *) malloc(ringbuffer_size * element_size);

    read_index = 0;
    write_index = 0;

//    sem_init(&read_sem, 0, 0);
//    sem_init(&write_sem, 0, ringbuffer_size / element_size - 2);
}

RingBuffer::~RingBuffer()
{
    free(buffer);
}

void RingBuffer::write32(uint32_t word)
{
    // wait if the write index would catch up to the read index
    std::unique_lock<std::mutex> lk(mtx);
    cv.wait(lk, [&] { return ((write_index + element_size >= ringbuffer_size) ? 0 : write_index + element_size) != read_index; });

//    sem_wait(&write_sem);

    // do the write
    memcpy(&buffer[write_index], &word, element_size);

    // update the write pointer
    write_index += element_size;

    // check if the write_index must wrap
    if (write_index >= ringbuffer_size)
    {
        write_index = 0;
    }

    // notify the cv that a read/write has occured
    cv.notify_all();

//    sem_post(&read_sem);
}

uint32_t RingBuffer::read32()
{
    uint32_t word;

    // wait if the read pointer has caught up to the write pointer
    std::unique_lock<std::mutex> lk(mtx);
    cv.wait(lk, [&] { return read_index != write_index; });

//    sem_wait(&read_sem);

    // do the read
    memcpy(&word, &buffer[read_index], element_size);

    // update the read pointer
    read_index += element_size;

    // check if the read_index must wrap
    if (read_index >= ringbuffer_size)
    {
        read_index = 0;
    }

    // notify the cv that a read/write has occured
    cv.notify_all();

//    sem_post(&write_sem);

    return word;
}

uint32_t RingBuffer::readBlock32(uint32_t* buf, uint32_t length)
{
    if ((length * element_size) > this->getCurSize()) {
        std::cerr << __PRETTY_FUNCTION__
            << " -> ERROR : not enough data in buffer! This should not be possible! Requested: "
            << length * element_size << ", actual: " << this->getCurSize() << std::endl;
        return 0;
    }

for (uint32_t i = 0; i < length; i++) buf[i] = this->read32();

return 1;

    //
    // 0 1 2 3 4 5 6 7
    //     w     r | |
    if (read_index + (length * element_size) >= ringbuffer_size) {
        //split transfer
        // length of first part

        uint32_t length_p1 = ringbuffer_size - read_index;
        uint32_t length_p2 = length * element_size - length_p1;
        memcpy(&buf[0], &buffer[read_index], length_p1);
        memcpy(&buf[(length_p1) / element_size], &buffer[0], length_p2);
    } else {
        //one transfer
        memcpy(&buf[0], &buffer[read_index], element_size * length);
    }
    // update the read pointer

    read_index += element_size * length;

    // check if the read_index must wrap
    if (read_index >= ringbuffer_size)
    {
        read_index = read_index - (ringbuffer_size - element_size) - element_size;
    }

//    for (uint32_t i = 0; i < length; i++) { sem_post(&write_sem); }

    // notify the cv that a read/write has occured
    cv.notify_all();

    return 1;
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
    return ((write_index - read_index) + (ringbuffer_size)) % (ringbuffer_size);
}

void RingBuffer::dump()
{
    for (uint32_t i = 0; i < ringbuffer_size / element_size; i++)
    {
        std::cout << "[" << i << "]\t\t0x" << std::hex << (uint32_t) *((uint32_t*) &buffer[i * element_size]) << std::dec << std::endl;
    }
}
