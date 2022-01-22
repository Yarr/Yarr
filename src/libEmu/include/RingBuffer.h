/*
 * Author: N. Whallon <alokin@uw.edu>
 * Date: 2017-VI
 * Description: a class to facilitate communication between programs using a typical buffer
 */

#ifndef RINGBUFFER
#define RINGBUFFER

#include <cstdint>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <semaphore.h>

#include "EmuCom.h"

class RingBuffer : public EmuCom {
	public:
		RingBuffer(uint32_t size);
		~RingBuffer() override;

		uint32_t * buffer;

		uint32_t ringbuffer_size;	// in bytes

		// this is the size of the elements in the buffer/array
		uint32_t element_size;

		// these indicate the index in the ring buffer where the write and read "cursors" are
		std::atomic<std::uint32_t> write_index;
		std::atomic<std::uint32_t> read_index;

		std::mutex mtx;
		std::condition_variable cv;

                sem_t read_sem;
                sem_t write_sem;

		// the main functionality of the class - write to and read from the ring buffer
		void write32(uint32_t word) override;
		uint32_t read32() override;
		uint32_t readBlock32(uint32_t *buf, uint32_t length) override;

		// useful utility functions
		bool isEmpty() override;
		uint32_t getCurSize() override;
		virtual void dump();
	private:
};

#endif
