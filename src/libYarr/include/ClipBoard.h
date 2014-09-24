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
#include <queue>

#include "RawData.h"
#include "Fei4EventData.h"

template <class T>
class ClipBoard {
    public:

        void pushData(T *data) {
            queueMutex.lock();
            dataQueue.push(data);
            queueMutex.unlock();
        }

        T* popData() {
            queueMutex.lock();
            T *tmp = dataQueue.front();
            dataQueue.pop();
            queueMutex.unlock();
            return tmp;
        }

        bool empty() {
            return dataQueue.empty();
        }

        unsigned size() {
            return dataQueue.size();
        }

    private:
        std::mutex queueMutex;
        std::queue<T*> dataQueue;

};

template class ClipBoard<RawData>;
template class ClipBoard<Fei4Data>;


#endif
