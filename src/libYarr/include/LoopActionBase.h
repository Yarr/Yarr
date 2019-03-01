/*
 * Authors: K. Potamianos <karolos.potamianos@cern.ch>,
 *          T. Heim <timon.heim@cern.ch>
 * Date: 2013-Oct-22
 */

#ifndef LOOPACTIONBASE_H
#define LOOPACTIONBASE_H

#include <memory>
#include <typeinfo>
#include <typeindex>
#include <string>

#include "TxCore.h"
#include "RxCore.h"
#include "LoopStatus.h"
#include "Bookkeeper.h"
#include "json.hpp"

using json=nlohmann::basic_json<std::map, std::vector, std::string, bool, std::int32_t, std::uint32_t, float>;

using std::shared_ptr;

class LoopActionBase {
    public:
        LoopActionBase();
        virtual ~LoopActionBase() {}

        void setVerbose(bool v=true);
        void setup(LoopStatus *stat, Bookkeeper *k);
        void setNext(shared_ptr<LoopActionBase>& ptr);
        void execute();

        std::type_index type() {
            return loopType;
        }

        unsigned getMin();
        unsigned getMax();
        unsigned getStep();
        void setMin(unsigned v);
        void setMax(unsigned v);
        void setStep(unsigned v);

        virtual void loadConfig(json &config) {}
        virtual void writeConfig(json &config) {}
		
        bool checkGlobalDone();

    protected:
        virtual void init() {}
        virtual void end() {}
        virtual void execPart1() {}
        virtual void execPart2() {}
        virtual bool done();

        bool m_done;
		bool g_done;
        bool verbose;

        int min;
        int max;
        unsigned step;

        double progress;

        LoopStatus *g_stat;
        FrontEnd *g_fe;
        TxCore *g_tx;
        RxCore *g_rx;
		Bookkeeper *keeper;
		std::map<unsigned, bool> doneMap;

        std::type_index loopType;
    private:
        void execStep();
        void run();

        shared_ptr<LoopActionBase> m_inner;
};

#endif
