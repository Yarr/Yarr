/*
 * Authors: T. Heim <timon.heim@cern.ch>,
 * Date: 2014-Oct-16
 */

#ifndef FEI4GLOBALFEEDBACK_H
#define FEI4GLOBALFEEDBACK_H

#include <queue>
#include "Fei4.h"
#include "LoopActionBase.h"
#include "FeedbackBase.h"

#include "logging.h"

class Fei4GlobalFeedback : public LoopActionBase, public GlobalFeedbackReceiver {
    static logging::Logger &logger() {
        static logging::LoggerStore instance = logging::make_log("Fei4GlobalFeedback");
        return *instance;
    }

    public:
    Fei4GlobalFeedback() : LoopActionBase(LOOP_STYLE_GLOBAL_FEEDBACK) {
        loopType = typeid(this);
    };

    Fei4GlobalFeedback(Fei4Register Fei4GlobalCfg::*ref) : LoopActionBase(LOOP_STYLE_GLOBAL_FEEDBACK), parPtr(ref) {
        loopType = typeid(this);
    };

    // Step down feedback algorithm
    void feedback(unsigned channel, double sign, bool last = false) override {
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
        fbDoneMap[channel] |= last;

        if (chan.localStep == 1) {
            fbDoneMap[channel] = true;
        }

        // Abort if we are getting to low
        if (val < 50) {
            fbDoneMap[channel] = true;
        }
    }

    // Binary search feedback algorithm
    void feedbackBinary(unsigned channel, double sign, bool last = false) override {
        ChannelInfo &chan = chanInfo[channel];
        // Calculate new step and value
        int val = (chan.values+(chan.localStep*sign));
        if (val < 0) val = 0;
        chan.values = val;
        chan.localStep  = chan.localStep/2;
        fbDoneMap[channel] |= last;

        if (chan.localStep == 1) {
            fbDoneMap[channel] = true;
        }
    }
    void writeConfig(json &config) override;
    void loadConfig(const json &config) override;
    private:
    std::string parName = "";
    void init() override {
        m_done = false;
        cur = 0;
        // Init all maps:
        for(unsigned id=0; id<keeper->getNumOfEntries(); id++) {
            auto fe = keeper->getFe(id);
            if(fe->getActive()) {
                ChannelInfo &info = chanInfo[id];
                info.localStep = step;
                info.values = max;
                info.oldSign = -1;
                fbDoneMap[id] = false;
            }
        }
        this->writePar();
    }

    void end() override {
        for(unsigned id=0; id<keeper->getNumOfEntries(); id++) {
            auto fe = keeper->getFe(id);
            if(fe->getActive()) {	
                logger().info(" --> Final parameter of Fe {} is {}", id, chanInfo[id].values);
            }
        }
    }

    void execPart1() override {
        g_stat->set(this, cur);
        m_done = isFeedbackDone();
    }

    void execPart2() override {
        // Wait for mutexes to be unlocked by feedback
        for(unsigned id=0; id<keeper->getNumOfEntries(); id++) {
            auto fe = keeper->getFe(id);
            if(fe->getActive()) {
                waitForFeedback(id);
                logger().info(" --> Received Feedback on ID {} with value: {}",
                        id,
                        chanInfo[id].values);
            }
        }
        cur++;
        this->writePar();
    }

    void writePar() {
        if(parName!=""){
            parPtr = keeper->globalFe<Fei4>()->regMap[parName];
        }
        for(unsigned id=0; id<keeper->getNumOfEntries(); id++) {
            Fei4 *fe = dynamic_cast<Fei4*>(keeper->getFe(id));
            if(fe->getActive()) {
                auto fe_cfg = dynamic_cast<FrontEndCfg*>(fe);
                auto tx_channel = fe_cfg->getTxChannel();
                auto rx_channel = fe_cfg->getRxChannel();
                
                g_tx->setCmdEnable(tx_channel);
                fe->writeRegister(parPtr, chanInfo[id].values);
                while(!g_tx->isCmdEmpty());
            }
        }
        g_tx->setCmdEnable(keeper->getTxMask());
    }

    Fei4Register Fei4GlobalCfg::*parPtr;

    protected:

    struct ChannelInfo{
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
