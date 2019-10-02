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

        ClipBoard() : done_flag(false) {}
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
            cv_not_empty.notify_all();
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
            return dataQueue.empty();
        }

        bool is_done() {
            return done_flag;
        }

        void finish() {
            done_flag = true;
            cv_not_empty.notify_all();
        }

        void wait_not_empty_or_done() {
          std::unique_lock<std::mutex> lk(queueMutex);
          cv_not_empty.wait(lk,
                            [&] { return done_flag || !empty(); } );
        }

    private:
        std::condition_variable cv_not_empty;

        std::mutex queueMutex;
        std::deque<std::unique_ptr<T>> dataQueue;

        std::atomic<bool> done_flag;
};

template class ClipBoard<RawData>;
// template class ClipBoard<Fei4Data>;


#endif
