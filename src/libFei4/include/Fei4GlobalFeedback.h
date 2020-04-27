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
        ChannelInfo &chan = chanInfo[channel];
        // Calculate new step and val
        if (sign != chan.oldSign) {
            chan.oldSign = 0;
            chan.localStep = chan.localStep/2;
        }
        int val = (chan.values+(chan.localStep*sign));
        if (val > (int)max) val = max;
        if (val < 0) val = 0;
        chan.values = val;
        doneMap[channel] |= last;

        if (chan.localStep == 1) {
            doneMap[channel] = true;
        }

        // Abort if we are getting to low
        if (val < 50) {
            doneMap[channel] = true;
        }
        // Unlock the mutex to let the scan proceed
        chan.fbMutex.unlock();
    }

    // Binary search feedback algorithm
    void feedbackBinary(unsigned channel, double sign, bool last = false) {
        ChannelInfo &chan = chanInfo[channel];
        // Calculate new step and value
        int val = (chan.values+(chan.localStep*sign));
        if (val < 0) val = 0;
        chan.values = val;
        chan.localStep  = chan.localStep/2;
        doneMap[channel] |= last;

        if (chan.localStep == 1) {
            doneMap[channel] = true;
        }

        // Unlock the mutex to let the scan proceed
        chan.fbMutex.unlock();
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
                ChannelInfo &info = chanInfo[ch];
                info.localStep = step;
                info.values = max;
                info.oldSign = -1;
                doneMap[ch] = false;
            }
        }
        this->writePar();
    }

    void end() {
        for(unsigned int k=0; k<keeper->feList.size(); k++) {
            if(keeper->feList[k]->getActive()) {	
                unsigned ch = dynamic_cast<FrontEndCfg*>(keeper->feList[k])->getRxChannel();
                logger().info(" --> Final parameter of Fe {} is {}", ch, chanInfo[ch].values);
            }
        }
    }

    void execPart1() {
        g_stat->set(this, cur);
        // Lock all mutexes if open
        for(unsigned int k=0; k<keeper->feList.size(); k++) {
            if(keeper->feList[k]->getActive()) {
                unsigned ch = dynamic_cast<FrontEndCfg*>(keeper->feList[k])->getRxChannel();
                ChannelInfo &info = chanInfo[ch];
                info.fbMutex.try_lock();
            }
        }
        m_done = allDone();
    }

    void execPart2() {
        // Wait for mutexes to be unlocked by feedback
        for(unsigned int k=0; k<keeper->feList.size(); k++) {
            if(keeper->feList[k]->getActive()) {
                unsigned ch = dynamic_cast<FrontEndCfg*>(keeper->feList[k])->getRxChannel();
                ChannelInfo &info = chanInfo[ch];
                info.fbMutex.lock();
                logger().info(" --> Received Feedback on Channel {} with value: {}",
                        dynamic_cast<FrontEndCfg*>(keeper->feList[k])->getRxChannel(),
                        chanInfo[dynamic_cast<FrontEndCfg*>(keeper->feList[k])->getRxChannel()].values);
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
                fe->writeRegister(parPtr, chanInfo[fe_cfg->getRxChannel()].values);
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

    struct ChannelInfo {
      std::mutex fbMutex;
      unsigned values;
      unsigned localStep;
      unsigned oldSign;
    };

    std::map<unsigned, ChannelInfo> chanInfo;
    unsigned cur;

    // Somehow we need to register logger at static init time
    friend void logger_static_init_fei4();
};

#endif
