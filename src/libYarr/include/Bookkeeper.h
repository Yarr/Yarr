#ifndef BOOKKEEPER_H
#define BOOKKEEPER_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Bookkeeper
// # Comment: Global container for data
// ################################

#include <iostream>
#include <mutex>
#include <deque>

#include "RawData.h"
#include "Fei4EventData.h"
#include "HistogramBase.h"
#include "ResultBase.h"
#include "ClipBoard.h"


#include "Fei4.h"
#include "TxCore.h"
#include "RxCore.h"

class Bookkeeper {
    public:
        Bookkeeper(TxCore *arg_tx, RxCore *arg_rx);
        ~Bookkeeper();

        void addFe(unsigned channel, unsigned chipId);
		int removeFe(unsigned channel);
		int removeFe(Fei4 *fe);

		int prepareMap();

		uint32_t setFeActive(Fei4 *fe);
		uint32_t setFeInactive(Fei4 *fe);
		uint32_t collectActiveMask();
//		uint31_t setActiveMask(uint32_t);	// Not allowed => change the FEs instead!!

//	    std::map<unsigned, ClipBoard<Fei4Data>* > eventMap;

        Fei4 *g_fe;
        TxCore *tx;
        RxCore *rx;

        ClipBoard<RawDataContainer> *data;
		std::vector<Fei4*> feList;

    private:
		uint32_t activeMask;
};

#endif
