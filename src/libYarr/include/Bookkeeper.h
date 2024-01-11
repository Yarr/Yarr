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
#include <memory>

#include "RawData.h"
#include "EventDataBase.h"
#include "HistogramBase.h"
#include "ResultBase.h"
#include "ClipBoard.h"

#include "FrontEnd.h"
#include "TxCore.h"
#include "RxCore.h"
#include "StdTriggerAction.h"

struct BookEntry {
    std::unique_ptr<FrontEnd> fe;

    bool active = false;
    
    uint32_t txChannel = 666;
    uint32_t rxChannel = 666;
};

class Bookkeeper {
    public:
        Bookkeeper(TxCore *arg_tx, RxCore *arg_rx);
        ~Bookkeeper() = default;

        void initGlobalFe(std::unique_ptr<FrontEnd> fe) {g_fe = std::move(fe);}
        void initGlobalFe(std::string chipType);

        // Bookkeeper always takes the ownership of the FrontEnd object
        void addFe(std::unique_ptr<FrontEnd> fe, const FrontEndConnectivity& cfg);
        void addFe(std::unique_ptr<FrontEnd> fe, unsigned channel);
	
        void delFe(unsigned id);
		void delFe(FrontEnd *fe);

		FrontEnd* getFe(unsigned id);
		FrontEndCfg* getFeCfg(unsigned id);
        FrontEnd* getLastFe();
        FrontEnd* getGlobalFe() const {
            return g_fe.get();
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

        template<typename T> T* globalFe() {return dynamic_cast<T*>(g_fe.get());}
        // TODO make private, not nice like that
        TxCore *tx;
        RxCore *rx;

        unsigned getId(FrontEnd *fe);
        BookEntry &getEntry(unsigned id);
        unsigned getNumOfEntries() {return bookEntries.size();};

        std::vector<unsigned> &getRxToId(unsigned rx);

        void setTriggerAction(std::shared_ptr<StdTriggerAction> trigLoop) {m_trigLoop = trigLoop;};
        std::shared_ptr<StdTriggerAction> getTriggerAction() {return m_trigLoop;};

    private:

        std::unique_ptr<FrontEnd> g_fe;

        // Index of vector is UID 
        std::vector<BookEntry> bookEntries;
        std::map<FrontEnd* , unsigned> idMap;
        std::map<unsigned, std::vector<unsigned>> rxToIdMap;
        std::shared_ptr<StdTriggerAction> m_trigLoop = nullptr;

        int target_tot;
        int target_threshold;
        int target_charge;
};

#endif
