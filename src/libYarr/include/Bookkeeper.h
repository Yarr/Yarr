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
        
        // TODO should only add generic Fe class
        // TODO should be independent of chipId
        void addFe(unsigned chipId, unsigned txChannel, unsigned rxChannel);
        void addFe(unsigned chipId, unsigned channel);
		
        void delFe(unsigned rxChannel);
		void delFe(Fei4 *fe);

		Fei4* getFei4byChannel(unsigned channel);
		Fei4* getFe(unsigned rxChannel);
        Fei4* getLastFe();
        Fei4* getGlobalFe() {
            return g_fe;
        }

		bool isChannelUsed(unsigned arg_channel);
        
        // Construct mask of active channels
        uint32_t getTxMask();
        uint32_t getRxMask();

        void setTargetTot(int v) {target_tot = v;}
        int getTargetTot() {return target_tot;}
        
        void setTargetCharge(int v) {target_charge = v;}
        int getTargetCharge() {return target_charge;}

        void setTargetThreshold(int v) {target_threshold = v;}
        int getTargetThreshold() {return target_threshold;}
        
        // TODO make private, not nice like that
        Fei4 *g_fe;
        TxCore *tx;
        RxCore *rx;
        
        std::vector<Fei4*> feList;

        ClipBoard<RawDataContainer> rawData;

        // per rx link
	    std::map<unsigned, ClipBoard<Fei4Data> > eventMap;
	    std::map<unsigned, ClipBoard<HistogramBase> > histoMap;
	    std::map<unsigned, ClipBoard<HistogramBase> > resultMap;
		std::map<unsigned, std::mutex> mutexMap;	
        
		std::vector<Fei4*> activeFeList;

    private:
        uint32_t activeTxMask;
        uint32_t activeRxMask;

		uint32_t activeMask;
		uint32_t usedChannels;

        int target_tot;
        int target_threshold;
        int target_charge;
};

#endif
