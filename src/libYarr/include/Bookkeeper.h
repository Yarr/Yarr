#ifndef BOOKKEEPER_H
#define BOOKKEEPER_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Bookkeeper
// # Comment: Global container for data
// ################################

#include <mutex>

#include "RawData.h"
#include "EventDataBase.h"
#include "HistogramBase.h"
#include "ResultBase.h"
#include "ClipBoard.h"

#include "FrontEnd.h"
#include "TxCore.h"
#include "RxCore.h"

struct BookEntry {
    FrontEnd *fe = nullptr;

    bool active = false;
    
    uint32_t txChannel = 666;
    uint32_t rxChannel = 666;
};

class Bookkeeper {
    public:
        Bookkeeper(TxCore *arg_tx, RxCore *arg_rx);
        ~Bookkeeper();

        void initGlobalFe(FrontEnd *fe) {g_fe = fe;}
        void initGlobalFe(std::string chipType);

        // TODO should only add generic Fe class
        void addFe(FrontEnd *fe, unsigned txChannel, unsigned rxChannel);
        void addFe(FrontEnd *fe, unsigned channel);
		
        void delFe(unsigned id);
		void delFe(FrontEnd *fe);

		FrontEnd* getFe(unsigned id);
		FrontEndCfg* getFeCfg(unsigned id);
        FrontEnd* getLastFe();
        FrontEnd* getGlobalFe() const {
            return g_fe;
        }

        // Construct mask of active channels
        std::vector<uint32_t> getTxMask();
        std::vector<uint32_t> getRxMask();

        // mask of unique active channels
        std::vector<uint32_t> getTxMaskUnique();
        std::vector<uint32_t> getRxMaskUnique();

        void setTargetTot(int v) {target_tot = v;}
        int getTargetTot() const {return target_tot;}
        
        void setTargetCharge(int v) {target_charge = v;}
        int getTargetCharge() const {return target_charge;}

        template<typename T> T* globalFe() {return dynamic_cast<T*>(g_fe);}
        // TODO make private, not nice like that
        FrontEnd *g_fe;
        TxCore *tx;
        RxCore *rx;

        unsigned getId(FrontEnd *fe);
        BookEntry &getEntry(unsigned id);
        unsigned getNumOfEntries() {return bookEntries.size();};

        std::vector<unsigned> &getRxToId(unsigned rx);

    private:
        
        // Index of vector is UID 
        std::vector<BookEntry> bookEntries;
        std::map<FrontEnd* , unsigned> idMap;
        std::map<unsigned, std::vector<unsigned>> rxToIdMap;


        int target_tot;
        int target_threshold;
        int target_charge;
};

#endif
