#ifndef CLIPBOARD_H
#define CLIPBOARD_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Data Clipboard
// # Comment: Saves data between processes
// ################################

#include <memory>
#include <mutex>
#include <deque>
#include <condition_variable>

#include "RawData.h"

#include <iostream>
#include <typeinfo>

template <class T>
class ClipBoard {
    public:

        ClipBoard(){}
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
            cv.notify_all();
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

        std::condition_variable cv;
        
    private:
        std::mutex queueMutex;
        std::deque<std::unique_ptr<T>> dataQueue;

};

template class ClipBoard<RawData>;
// template class ClipBoard<Fei4Data>;


#endif
