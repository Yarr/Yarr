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
#include <queue>


class RingBuffer  {
	public:
		RingBuffer() = default;
		~RingBuffer() = default;
	private:
        std::queue<uint32_t> buffer;

		// this is the size of the elements in the buffer/array
		static constexpr uint32_t element_size = sizeof(uint32_t);

		// these indicate the index in the ring buffer where the write and read "cursors" are
		std::atomic<std::uint32_t> size = 0;

		std::mutex mtx;
		std::condition_variable cv;


	public:
		// the main functionality of the class - write to and read from the ring buffer
		inline void write32(uint32_t word) {
            std::unique_lock<std::mutex> lk(mtx);
            buffer.push(word);
            size++;
            cv.notify_all();
        }
        inline void write32(const std::vector<uint32_t> &v) {
            std::unique_lock<std::mutex> lk(mtx);
            for(auto const &el: v) buffer.push(el);
            size.fetch_add(v.size());
            cv.notify_all();
        }
		inline uint32_t read32() {
            std::unique_lock<std::mutex> lk(mtx);
            cv.wait(lk, [&] { return size.load()>0; });
            uint32_t ret=buffer.front();
            buffer.pop();
            size--;
            cv.notify_all();
            return ret;
        }

        inline uint32_t readAll(std::vector<uint32_t> &buf){
            if(isEmpty()) return 0;
            std::unique_lock<std::mutex> lk(mtx);
            uint32_t current_size = size.load();
            for(int i=0;i<current_size;++i) {
                buf.push_back(buffer.front());
                buffer.pop();
            }
            size.fetch_sub(current_size);
            cv.notify_all();
            return current_size;
        }

		inline uint32_t readBlock32(uint32_t *buf, uint32_t length){
            std::unique_lock<std::mutex> lk(mtx);
            cv.wait(lk, [&] { return size.load()>=length; });
            for(int i=0;i<length;++i) {
                buf[i]=buffer.front();
                buffer.pop();
            }
            size.fetch_sub(length);
            cv.notify_all();
            return 1;
         }

		// useful utility functions
		inline bool isEmpty() { return size.load() == 0 ; };
		inline uint32_t getCurSize() { return size.load() * element_size;};
};
#endif
