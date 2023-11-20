#ifndef SCAN_LOOP_INFO_H
#define SCAN_LOOP_INFO_H

#include "LoopActionBaseInfo.h"

class ScanLoopInfo {
    public:
        virtual ~ScanLoopInfo() = default;
        virtual unsigned size() const = 0;
        virtual const LoopActionBaseInfo *getLoop(unsigned n) const = 0;
};

#endif
