#ifndef RD53ADATAPROCESSOR_H
#define RD53ADATAPROCESSOR_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: RD53A Data Processor
// # Date: Apr 2018
// ################################

#include <vector>
#include <array>
#include <map>

#include "FeDataProcessor.h"
#include "ClipBoard.h"
#include "RawData.h"
#include "Rd53a.h"

class Rd53aDataProcessor : public FeDataProcessor {
    public:
        Rd53aDataProcessor();
        ~Rd53aDataProcessor() override;

        void connect(FrontEndCfg *feCfg, ClipBoard<RawDataContainer> *input, ClipBoard<EventDataBase> *output) override {
            m_input = input;
            m_output = output;
        }

        void init()    override;
        void run()     override;
        void join()    override;
        void process() override;

    private:
        std::unique_ptr<std::thread> thread_ptr;
        ClipBoard<RawDataContainer> *m_input;
        ClipBoard<EventDataBase> *m_output;
        
        unsigned tag;
        unsigned l1id;
        unsigned bcid;
        unsigned wordCount;
        unsigned hits;

        void process_core();
    
};

#endif
