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

#include "DataProcessor.h"
#include "ClipBoard.h"
#include "RawData.h"
#include "Fei4EventData.h"
#include "Rd53a.h"

class Rd53aDataProcessor : public DataProcessor {
    public:
        Rd53aDataProcessor();
        ~Rd53aDataProcessor();

        void connect(ClipBoard<RawDataContainer> *input, std::map<unsigned, ClipBoard<EventDataBase> > *outMap) override final {
            m_input = input;
            m_outMap = outMap;
        }

        void init()    override final;
        void run()     override final;
        void join()    override final; 
        void process() override final;

    private:
        std::vector<std::unique_ptr<std::thread>> thread_ptrs;
        ClipBoard<RawDataContainer> *m_input;
        std::map<unsigned, ClipBoard<EventDataBase>> *m_outMap;
        std::vector<unsigned> activeChannels;
        
        std::map<unsigned, unsigned> tag;
        std::map<unsigned, unsigned> l1id;
        std::map<unsigned, unsigned> bcid;
        std::map<unsigned, unsigned> wordCount;
        std::map<unsigned, int> hits;

        void process_core();
        
        unsigned long dataWords = 0;
        unsigned long headerWords = 0;
        unsigned long hitWords = 0;
        unsigned long invalidWords = 0; //invalid Hit words
        unsigned long ffffWords = 0;
        unsigned long noWayWords = 0;

        unsigned long dataTotal = 0;
        unsigned long headerTotal = 0;
        unsigned long hitTotal = 0;
        unsigned long invalidTotal = 0; //invalid Hit words
        unsigned long ffffTotal = 0;
        unsigned long noWayTotal = 0;

        void printLocalStats();
        void printGlobalStats();    
};

#endif
