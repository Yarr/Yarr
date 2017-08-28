#ifndef SCANBASE_H
#define SCANBASE_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Scan Base Class
// # Comment:
// ################################

#include <vector>
#include <memory>
#include <map>
#include <typeindex>

#include "TxCore.h"
#include "RxCore.h"
#include "LoopEngine.h"
#include "LoopActionBase.h"
#include "ClipBoard.h"
#include "RawData.h"
#include "json.hpp"

#include "Bookkeeper.h"

using json=nlohmann::basic_json<std::map, std::vector, std::string, bool, std::int32_t, std::uint32_t, float>;

class ScanBase {
    public:
        ScanBase(Bookkeeper *k);
        virtual ~ScanBase() {}

        virtual void init() {}
        virtual void preScan() {}
        virtual void postScan() {}
        void run();

        std::shared_ptr<LoopActionBase> getLoop(unsigned n);
        std::shared_ptr<LoopActionBase> operator[](unsigned n);
        std::shared_ptr<LoopActionBase> operator[](std::type_index t);
        unsigned size();
        
        virtual void loadConfig(json &cfg) {}

    protected:
        LoopEngine engine;
        void addLoop(std::shared_ptr<LoopActionBase> l);
        TxCore *g_tx;
        RxCore *g_rx;
        Bookkeeper *g_bk;
        ClipBoard<RawDataContainer> *g_data;

    private:
        std::vector<std::shared_ptr<LoopActionBase> > loops;
        std::map<std::type_index, std::shared_ptr<LoopActionBase> > loopMap;
};

#endif
