#ifndef BOOKKEEPER_H
#define BOOKKEEPER_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Bookkeeper
// # Comment: Global container for data
// ################################

#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "FrontEnd.h"
#include "StdTriggerAction.h"

class FrontEndCfg;
class RxCore;
class TxCore;
template<typename T>
class ClipBoard;
class HistogramBase;

struct BookEntry {
    std::unique_ptr<FrontEnd> fe;

    bool active = false;
    
    uint32_t txChannel = 666;
    uint32_t rxChannel = 666;
    uint32_t regRxChannel = 666;
};

/**
 * Keep track of objects in system.
 *
 * The fundamental objects are:
 *  TxCore, RxCore
 *
 * FrontEnds should then be added, each associated with output and input channels.
 * These both store configuration and access the chip via TxCore.
 *
 * Also attached are the resulting histograms.
 */
class Bookkeeper {
    public:
        /** Establish Bookkeeper with a TxCore and RxCore */
        Bookkeeper(TxCore *arg_tx, RxCore *arg_rx);
        ~Bookkeeper();

        /** Add Global FrontEnd */
        void initGlobalFe(std::unique_ptr<FrontEnd> fe) {g_fe = std::move(fe);}
        /** Create Global FrontEnd based on chip type */
        void initGlobalFe(const std::string& chipType);

        /**
         * Attach FrontEnd with connectivity (including tx and rx channel).
         *
         * Bookkeeper always takes the ownership of the FrontEnd object.
         */
        void addFe(std::unique_ptr<FrontEnd> fe, const FrontEndConnectivity& cfg);
        /**
         * Attach FrontEnd to the same tx and rx channel.
         *
         * Bookkeeper always takes the ownership of the FrontEnd object.
         */
        void addFe(std::unique_ptr<FrontEnd> fe, unsigned channel);

        /**
         * Delete FrontEnd referenced by ID.
         */  
        void delFe(unsigned id);
        /**
         * Delete FrontEnd by reference to pointer.
         */  
        void delFe(FrontEnd *fe);

        /** Retrieve FrontEnd with ID. */
        FrontEnd* getFe(unsigned id);
        /** Retrieve FrontEndCfg with ID. */
        FrontEndCfg* getFeCfg(unsigned id);

        /** Retrieve most recently added FrontEnd. */
        FrontEnd* getLastFe();
        /** Retrieve global FrontEnd */
        FrontEnd* getGlobalFe() const {
            return g_fe.get();
        }

        // Construct mask of active channels
        /** Return mask of active tx channels */
        std::vector<uint32_t> getTxMask();
        /** Return mask of active rx channels (including reg rx). */
        std::vector<uint32_t> getRxMask();

        // mask of unique active channels
        /** Return list of active tx channels */
        std::vector<uint32_t> getTxMaskUnique();
        /** Return list of active rx channels */
        std::vector<uint32_t> getRxMaskUnique();

        /** Set time over threshold target value */
        void setTargetTot(int v) {target_tot = v;}
        /** Retrieve time over threshold target value */
        int getTargetTot() const {return target_tot;}
        
        /** Set target charge value */
        void setTargetCharge(int v) {target_charge = v;}
        /** Retrieve target charge value */
        int getTargetCharge() const {return target_charge;}

        /** Retrieve global frontend */
        template<typename T> T* globalFe() {return dynamic_cast<T*>(g_fe.get());}
        // TODO make private, not nice like that
        TxCore *tx;
        RxCore *rx;

        /** Lookup ID for FrontEnd */
        unsigned getId(FrontEnd *fe);
        /** Get BookEntry for FrontEnd ID */
        BookEntry &getEntry(unsigned id);
        /** Return count of known FrontEnds */
        unsigned getNumOfEntries() {return bookEntries.size();};

        /** Return FrontEnd IDs associated with rx channel. */
        std::vector<unsigned> &getRxToId(unsigned rx);

        /** Set the trigger action (used for communicating expected event counts). */
        void setTriggerAction(std::shared_ptr<StdTriggerAction> trigLoop) {m_trigLoop = trigLoop;};

        /** Get the trigger action (used for communicating expected event counts). */
        std::shared_ptr<StdTriggerAction> getTriggerAction() {return m_trigLoop;};

        /** Start the clipboard monitor thread. */
        void startFeClipboardMonitor();
        /** Finish the clipboard monitor thread. */
        void joinFeClipboardMonitor();
        /** Configure the clipboard monitor thread. */
        void setFeClipboardMonitorRefreshTime(unsigned arg_clipboardMonitorRefreshTime);
        /** Add FrontEnd to clipboard monitor thread. */
        void addFeClipboardMonitor(unsigned arg_id, const std::string& arg_name);

        ClipBoard<HistogramBase> &getLoopHistograms();

    private:
        void feClipboardMonitor();

        std::unique_ptr<FrontEnd> g_fe;

        // Index of vector is UID 
        std::vector<BookEntry> bookEntries;
        std::map<FrontEnd* , unsigned> idMap;
        std::map<unsigned, std::vector<unsigned>> rxToIdMap;
        std::shared_ptr<StdTriggerAction> m_trigLoop = nullptr;

        int target_tot = 0;
        int target_threshold = 0;
        int target_charge = 0;

        // Clipboard monitoring thread/variables
        unsigned clipboardMonitorRefreshTime = 0;
        bool runClipboardMonitor = false;

        std::unique_ptr<ClipBoard<HistogramBase>> loop_histograms;
        std::unique_ptr<std::thread> clipboardMonitorThread_ptr;
        std::vector<unsigned> clipboardMonitorFeIDs;
        std::vector<std::string> clipboardMonitorFeNames;
};

#endif
