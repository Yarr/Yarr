#ifndef STDDATAACTION_H
#define STDDATAACTION_H

/*
 * Authors: T. Heim <timon.heim@cern.ch>,
 * Date: 2018-May-30
 */

#include "ClipBoard.h"

class StdDataAction {
    public:
        void connect(std::map<unsigned, ClipBoard<RawDataContainer> > *clipboards) {
            storage = clipboards;
        }
    protected:
        std::map<unsigned, ClipBoard<RawDataContainer> > *storage;
};

#endif
