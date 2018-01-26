#ifndef CLIPBOARD_H
#define CLIPBOARD_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Data Clipboard
// # Comment: Saves data between processes
// ################################

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
                T* tmp = this->popData();
                delete tmp;
            }
        }

        void pushData(T *data) {
            queueMutex.lock();
            if (data != NULL) dataQueue.push_back(data);
            queueMutex.unlock();
            //static unsigned cnt = 0;
            //std::cout << "Pushed " << cnt++ << " " << typeid(T).name() << " objects so far" << std::endl;
            cv.notify_all();
        }

        // User has to take of deletin popped data
        T* popData() {
            queueMutex.lock();
            T *tmp = NULL;
            if(!dataQueue.empty()) {
                tmp = dataQueue.front();
                dataQueue.pop_front();
            }
            queueMutex.unlock();
            return tmp;
        }

        bool empty() {
            return dataQueue.empty();
        }

        unsigned size() {
            return dataQueue.size();
        }

        typename std::deque<T*>::iterator begin() {
            return dataQueue.begin();
        }
       

        typename std::deque<T*>::iterator end() {
            return dataQueue.end();
        }

        void clear() {
            while(!dataQueue.empty()) {
                T* tmp = this->popData();
                delete tmp;
            }
        }

        std::condition_variable cv;
        
    private:
        std::mutex queueMutex;
        std::deque<T*> dataQueue;

};

template class ClipBoard<RawData>;
// template class ClipBoard<Fei4Data>;


#endif
