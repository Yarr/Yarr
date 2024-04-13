#ifndef CLIPBOARD_H
#define CLIPBOARD_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Data Clipboard
// # Comment: Saves data between processes
// ################################

#include <atomic>
#include <memory>
#include <mutex>
#include <deque>
#include <condition_variable>
#include <chrono>

#include "RawData.h"
#include "logging.h"

#include <typeinfo>

namespace
{
    auto clipboardLogger = logging::make_log("ClipboardDebug");
}

template <class T>
class ClipBoard {
    public:

        ClipBoard() : doneFlag(false), numDataIn(0), numDataOut(0) {}
        ~ClipBoard() {
            while(!dataQueue.empty()) {
                std::unique_ptr<T> tmp = this->popData();
            }
        }

        ClipBoard(ClipBoard &&o) = delete;
        ClipBoard(const ClipBoard &o) = delete;
        ClipBoard& operator=(const ClipBoard &l) = delete;
        ClipBoard& operator=(const ClipBoard &&l) = delete;

        void startMonitor(std::string arg_name, unsigned arg_monitorCycleTime = 10000) {
            name = arg_name; // clipboard monitor name
            monitorCycleTime = arg_monitorCycleTime; // microseconds

            runMonitor = true;
            thread_ptr.reset(new std::thread(&ClipBoard::sizeMonitor, this));
        }

        void joinMonitor() {
            if(runMonitor) {
                runMonitor = false;
                thread_ptr->join();
            }
        }

        void sizeMonitor() {
            clipboardLogger->info("{}: Started custom clipboard monitor thread", name);
            while(runMonitor) 
            {
                clipboardLogger->info("| {:^24} | InCount:{:<8} OutCount:{:<8} QueueSize:{:<8} InSize:{:<12} OutSize:{:<12}", name, numDataIn, numDataOut, dataQueue.size(), sizeDataIn, sizeDataOut);
                std::this_thread::sleep_for(std::chrono::microseconds(monitorCycleTime)); // microseconds  
            }
            clipboardLogger->info("{}: Ended custom clipboard monitor thread", name);
        }

        void pushData(std::unique_ptr<T> data) {
            queueMutex.lock();
            if (data != NULL) {
                sizeDataIn += sizeof(*data);
                dataQueue.push_back(std::move(data));
                numDataIn++;
            }
            queueMutex.unlock();
            //static unsigned cnt = 0;
            //std::cout << "Pushed " << cnt++ << " " << typeid(T).name() << " objects so far" << std::endl;
            cvNotEmpty.notify_all();
        }

        // User has to take of deletin popped data
        std::unique_ptr<T> popData() {
            queueMutex.lock();
            std::unique_ptr<T> tmp;
            if(!dataQueue.empty()) {
                tmp = std::move(dataQueue.front());
                dataQueue.pop_front();
                sizeDataOut += sizeof(*tmp);
                numDataOut++;
            }
            queueMutex.unlock();
            return tmp;
        }

        int size() const {
          return dataQueue.size();
        }

        long long getNumDataIn() const {
            return numDataIn;
        }

        unsigned getNumDataOut() const {
            return numDataOut;
        }

        bool empty() {
            std::lock_guard<std::mutex> lock(queueMutex);
            return rawEmpty();
        }

        bool isDone() const {
            return doneFlag;
        }

        void finish() {
            doneFlag = true;
            cvNotEmpty.notify_all();
        }

        void waitNotEmptyOrDone() {
          std::unique_lock<std::mutex> lk(queueMutex);
          cvNotEmpty.wait(lk,
                            [&] { return doneFlag || !rawEmpty(); } );
        }

        template<typename Rep, typename TimeUnit>
        bool waitNotEmptyOrDoneOrTimeout(std::chrono::duration<Rep, TimeUnit> timeout) {
          std::unique_lock<std::mutex> lk(queueMutex);
          return cvNotEmpty.wait_for(lk, timeout,
                            [&] { return doneFlag || !rawEmpty(); } );
        }

        void reset() {
            doneFlag = false;
            numDataIn = 0;
            numDataOut = 0;
            sizeDataIn = 0;
            sizeDataOut = 0;
        };

    private:
        bool rawEmpty() {
            return dataQueue.empty();
        }

        std::condition_variable cvNotEmpty;

        std::mutex queueMutex;
        std::deque<std::unique_ptr<T>> dataQueue;

        std::atomic<bool> doneFlag;
        std::atomic<unsigned> numDataIn;
        std::atomic<unsigned> numDataOut;

        std::atomic<unsigned> sizeDataIn;
        std::atomic<unsigned> sizeDataOut;

        std::string name;
        std::unique_ptr<std::thread> thread_ptr;
        unsigned monitorCycleTime;
        bool runMonitor;
};

template class ClipBoard<RawData>;
// template class ClipBoard<Fei4Data>;


#endif
