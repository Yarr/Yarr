#ifndef SPECCONTROLLER_H
#define SPECCONTROLLER_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Spec Controller class
// # Comment:
// # Data: Feb 2017
// ################################

#include "HwController.h"
#include "SpecTxCore.h"
#include "SpecRxCore.h"


#include "storage.hpp"

class SpecController : public HwController, public SpecTxCore, public SpecRxCore {
    public:

        const json getStatus() override {
            return this->SpecCom::getStatus();
        }

        void loadConfig(const json &j) override {
            if (j.contains("specNum"))
                this->SpecCom::init(j["specNum"]);
            
            // Set direction of LVDS lines
            if (j.contains("spiConfig")) {
                // TODO make proper function
                this->writeSingle(0x6<<14 | 0x0, (uint32_t)j["spiConfig"]);
                this->writeSingle(0x6<<14 | 0x1, 0xF);
            }

            if (j.contains("rxPolarity")) {
                this->setRxPolarity(j["rxPolarity"]);
            }

            if (j.contains("txPolarity")) {
                this->setTxPolarity(j["txPolarity"]);
            }
            
            // Configure trigger logic
            if (j.contains("trigConfig")) {
                if(j.contains({"trigConfig","mask"}))
                    this->setTriggerLogicMask(j["trigConfig"]["mask"]);
                
                if(j.contains({"trigConfig","mode"}))
                    this->setTriggerLogicMode(j["trigConfig"]["mode"]);
                
                if(j.contains({"trigConfig","config"}))
                    this->setTriggerLogicConfig(j["trigConfig"]["config"]);
                
                if(j.contains({"trigConfig","edge"}))
                    this->setTriggerEdge(j["trigConfig"]["edge"]);
                
                if(j.contains({"trigConfig","delay"})) {
                    if(j["trigConfig"]["delay"].size() == 4) {
                        for (unsigned i=0; i<4; i++) {
                            this->setTriggerDelay(i, j["trigConfig"]["delay"][i]);
                        }
                    }
                }
                if(j.contains({"trigConfig","deadtime"}))
                    this->setTriggerDeadtime(j["trigConfig"]["deadtime"]);
                if(j.contains({"trigConfig","triggerEncoderMultiplier"}))
                    this->setTriggerEncoderMultiplier(j["trigConfig"]["triggerEncoderMultiplier"]);
                if(j.contains({"trigConfig","triggerEncoderEnable"}))
                    this->setTriggerEncoderEnable(j["trigConfig"]["triggerEncoderEnable"]);
            }

            // Configure pulse logic
            if (j.contains("pulse")) {
                if (j.contains({"pulse","word"})) {
                    this->setPulseWord(j["pulse"]["word"]);
                    m_pulseWord = j["pulse"]["word"];
                }
                if (j.contains({"pulse","interval"})) {
                    this->setPulseInterval(j["pulse"]["interval"]);
                    m_pulseInterval = j["pulse"]["interval"];
                }
            }
            
            // Configure sync logic
            if (j.contains("sync")) {
                if (j.contains({"sync","word"})) {
                    this->setSyncWord(j["sync"]["word"]);
                    m_syncWord = j["sync"]["word"];
                }
                if (j.contains({"sync","interval"})) {
                    this->setSyncInterval(j["sync"]["interval"]);
                    m_syncInterval = j["sync"]["interval"];
                }
            }

            // Configure sync logic
            if (j.contains("idle")) {
                if (j.contains({"idle","word"})) {
                    this->setIdleWord(j["idle"]["word"]);
                    m_idleWord = j["idle"]["word"];
                }
            }

            // Configure Tx speed
            if (j.contains("cmdPeriod")) {
                SpecTxCore::m_clk_period = (float)j["cmdPeriod"]; //fix for variant 
            }
            
            // Set number of active lanes
            if (j.contains("rxActiveLanes")) {
                this->setRxActiveLanes(j["rxActiveLanes"]);
                SpecRxCore::m_rxActiveLanes = j["rxActiveLanes"];
            }

            // Set number of active lanes
            if (j.contains("rxDelayOffset")) {
                SpecRxCore::m_rxDelayOffset = j["rxDelayOffset"];
            }
 
            SpecCom::writeSingle(RX_ADDR | RX_MANUAL_DELAY, 0xFFFF); 
            if (j.contains("delay")) {
                SpecRxCore::m_delay.clear();
                unsigned n = 0;
                for(auto i: j["delay"]) {
                    SpecRxCore::m_delay.push_back(i);
                    SpecRxCore::setRxDelay(n, i);
                    n++;
                }
    	    }


        }

        void setupMode() final{
            this->setPulseWord(0x0);
        }

        void runMode() override {
            this->setPulseWord(m_pulseWord);
        }

    private:
        uint32_t m_pulseWord;
        uint32_t m_pulseInterval;

        uint32_t m_syncWord;
        uint32_t m_syncInterval;

        uint32_t m_idleWord;

};

#endif
