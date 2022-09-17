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
#include <vector>


class RingBuffer  {
	public:
		RingBuffer(uint32_t size);
		~RingBuffer();
	private:
                std::vector<uint32_t> buffer;

		// this is the size of the elements in the buffer/array
		static constexpr uint32_t element_size = sizeof(uint32_t);

		// these indicate the index in the ring buffer where the write and read "cursors" are
		std::atomic<std::uint32_t> write_index;
		std::atomic<std::uint32_t> read_index;

		std::mutex mtx;
		std::condition_variable cv;

	public:
		// the main functionality of the class - write to and read from the ring buffer
		void write32(uint32_t word);
		uint32_t read32();
		uint32_t readBlock32(uint32_t *buf, uint32_t length);

		// useful utility functions
		bool isEmpty();
		uint32_t getCurSize();
		void dump();
};
#endif
