#ifndef STDDATAACTION_H
#define STDDATAACTION_H

/*
 * Authors: T. Heim <timon.heim@cern.ch>,
 * Date: 2018-May-30
 */

#include "ClipBoard.h"

class StdDataAction {
    public:
        void connect(ClipBoard<RawDataContainer> *clipboard) {
            storage = clipboard;
        }
    protected:
        ClipBoard<RawDataContainer> *storage;
};

#endif
