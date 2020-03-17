/*
 * Authors: T. Heim <timon.heim@cern.ch>,
 * Date: 2014-Oct-16
 */

#ifndef FEI4GLOBALFEEDBACK_H
#define FEI4GLOBALFEEDBACK_H

#include <queue>
#include <mutex>
#include "Fei4.h"
#include "LoopActionBase.h"
#include "FeedbackBase.h"

#include "logging.h"

class Fei4GlobalFeedback : public LoopActionBase, public GlobalFeedbackBase {
    static logging::Logger &logger() {
        static logging::LoggerStore instance = logging::make_log("Fei4GlobalFeedback");
        return *instance;
    }

    public:
    Fei4GlobalFeedback() {
        loopType = typeid(this);
    };

    Fei4GlobalFeedback(Fei4Register Fei4GlobalCfg::*ref) :parPtr(ref) { 
        loopType = typeid(this);
    };

    // Step down feedback algorithm
    void feedback(unsigned channel, double sign, bool last = false) {
        // Calculate new step and val
        if (sign != oldSign[channel]) {
            oldSign[channel] = 0;
            localStep[channel] = localStep[channel]/2;
        }
        int val = (values[channel]+(localStep[channel]*sign));
        if (val > (int)max) val = max;
        if (val < 0) val = 0;
        values[channel] = val;
        doneMap[channel] |= last;

        if (localStep[channel] == 1) {
            doneMap[channel] = true;
        }

        // Abort if we are getting to low
        if (val < 50) {
            doneMap[channel] = true;
        }
        // Unlock the mutex to let the scan proceed
        keeper->mutexMap[channel].unlock();
    }

    // Binary search feedback algorithm
    void feedbackBinary(unsigned channel, double sign, bool last = false) {
        // Calculate new step and value
        int val = (values[channel]+(localStep[channel]*sign));
        if (val < 0) val = 0;
        values[channel] = val;
        localStep[channel]  = localStep[channel]/2;
        doneMap[channel] |= last;

        if (localStep[channel] == 1) {
            doneMap[channel] = true;
        }

        // Unlock the mutex to let the scan proceed
        keeper->mutexMap[channel].unlock();
    }
    void writeConfig(json &config);
    void loadConfig(json &config);
    private:
    std::string parName = "";
    void init() {
        m_done = false;
        cur = 0;
        // Init all maps:
        for(unsigned int k=0; k<keeper->feList.size(); k++) {
            if(keeper->feList[k]->getActive()) {
                unsigned ch = dynamic_cast<FrontEndCfg*>(keeper->feList[k])->getRxChannel();
                localStep[ch] = step;
                values[ch] = max;
                oldSign[ch] = -1;
                doneMap[ch] = false;
            }
        }
        this->writePar();
    }

    void end() {
        for(unsigned int k=0; k<keeper->feList.size(); k++) {
            if(keeper->feList[k]->getActive()) {	
                unsigned ch = dynamic_cast<FrontEndCfg*>(keeper->feList[k])->getRxChannel();
                logger().info(" --> Final parameter of Fe {} is {}", ch, values[ch]);
            }
        }
    }

    void execPart1() {
        g_stat->set(this, cur);
        // Lock all mutexes if open
        for(unsigned int k=0; k<keeper->feList.size(); k++) {
            if(keeper->feList[k]->getActive()) {	
                keeper->mutexMap[dynamic_cast<FrontEndCfg*>(keeper->feList[k])->getRxChannel()].try_lock();
            }
        }
        m_done = allDone();
    }

    void execPart2() {
        // Wait for mutexes to be unlocked by feedback
        for(unsigned int k=0; k<keeper->feList.size(); k++) {
            if(keeper->feList[k]->getActive()) {
                auto channel = dynamic_cast<FrontEndCfg*>(keeper->feList[k])->getRxChannel();
                keeper->mutexMap[channel].lock();
                logger().info(" --> Received Feedback on Channel {} with value: {}",
                              channel, values[channel]);
            }
        }
        cur++;
        this->writePar();
    }

    void writePar() {
        if(parName!=""){
            parPtr = keeper->globalFe<Fei4>()->regMap[parName];
        }
        for(unsigned int k=0; k<keeper->feList.size(); k++) {
            if(keeper->feList[k]->getActive()) {
                auto fe = dynamic_cast<Fei4*>(keeper->feList[k]);
                auto fe_cfg = dynamic_cast<FrontEndCfg*>(fe);
                auto tx_channel = fe_cfg->getTxChannel();
                auto rx_channel = fe_cfg->getRxChannel();

                g_tx->setCmdEnable(tx_channel);
                fe->writeRegister(parPtr, values[rx_channel]);
                while(!g_tx->isCmdEmpty());
            }
        }
        g_tx->setCmdEnable(keeper->getTxMask());
    }

    bool allDone() {
        for(unsigned int k=0; k<keeper->feList.size(); k++) {
            if(keeper->feList[k]->getActive()) {
                unsigned ch = dynamic_cast<FrontEndCfg*>(keeper->feList[k])->getRxChannel();
                if (!doneMap[ch])
                    return false;
            }
        }
        return true;
    }

    Fei4Register Fei4GlobalCfg::*parPtr;

    protected:
    std::mutex fbMutex;
    std::map<unsigned, unsigned> values;
    std::map<unsigned, unsigned> localStep;
    std::map<unsigned, double> oldSign;
    unsigned cur;

    // Somehow we need to register logger at static init time
    friend void logger_static_init_fei4();
};

#endif
