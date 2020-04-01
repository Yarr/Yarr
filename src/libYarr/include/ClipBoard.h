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

#include "RawData.h"

#include <typeinfo>

template <class T>
class ClipBoard {
    public:

        ClipBoard() : doneFlag(false) {}
        ~ClipBoard() {
            while(!dataQueue.empty()) {
                std::unique_ptr<T> tmp = this->popData();
            }
        }

        void pushData(std::unique_ptr<T> data) {
            queueMutex.lock();
            if (data != NULL) dataQueue.push_back(std::move(data));
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
            }
            queueMutex.unlock();
            return tmp;
        }

        bool empty() {
            std::lock_guard<std::mutex> lock(queueMutex);
            return rawEmpty();
        }

        bool isDone() {
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

    private:
        bool rawEmpty() {
            return dataQueue.empty();
        }

        std::condition_variable cvNotEmpty;

        std::mutex queueMutex;
        std::deque<std::unique_ptr<T>> dataQueue;

        std::atomic<bool> doneFlag;
};

template class ClipBoard<RawData>;
// template class ClipBoard<Fei4Data>;


#endif
