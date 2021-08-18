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

        void loadConfig(json &j) override {
            if (!j["specNum"].empty())
                this->SpecCom::init(j["specNum"]);
            
            // Set direction of LVDS lines
            if (!j["spiConfig"].empty()) {
                // TODO make proper function
                this->writeSingle(0x6<<14 | 0x0, (uint32_t)j["spiConfig"]);
                this->writeSingle(0x6<<14 | 0x1, 0xF);
            }

            if (!j["rxPolarity"].empty()) {
                this->setRxPolarity(j["rxPolarity"]);
            }

            if (!j["txPolarity"].empty()) {
                this->setTxPolarity(j["txPolarity"]);
            }
            
            // Configure trigger logic
            if (!j["trigConfig"].empty()) {
                if(!j["trigConfig"]["mask"].empty())
                    this->setTriggerLogicMask(j["trigConfig"]["mask"]);
                
                if(!j["trigConfig"]["mode"].empty())
                    this->setTriggerLogicMode(j["trigConfig"]["mode"]);
                
                if(!j["trigConfig"]["config"].empty())
                    this->setTriggerLogicConfig(j["trigConfig"]["config"]);
                
                if(!j["trigConfig"]["edge"].empty())
                    this->setTriggerEdge(j["trigConfig"]["edge"]);
                
                if(!j["trigConfig"]["delay"].empty()) {
                    if(j["trigConfig"]["delay"].size() == 4) {
                        for (unsigned i=0; i<4; i++) {
                            this->setTriggerDelay(i, j["trigConfig"]["delay"][i]);
                        }
                    }
                }
                if(!j["trigConfig"]["deadtime"].empty())
                    this->setTriggerDeadtime(j["trigConfig"]["deadtime"]);
            }

            // Configure pulse logic
            if (!j["pulse"].empty()) {
                if (!j["pulse"]["word"].empty()) {
                    this->setPulseWord(j["pulse"]["word"]);
                    m_pulseWord = j["pulse"]["word"];
                }
                if (!j["pulse"]["interval"].empty()) {
                    this->setPulseInterval(j["pulse"]["interval"]);
                    m_pulseInterval = j["pulse"]["interval"];
                }
            }
            
            // Configure sync logic
            if (!j["sync"].empty()) {
                if (!j["sync"]["word"].empty()) {
                    this->setSyncWord(j["sync"]["word"]);
                    m_syncWord = j["sync"]["word"];
                }
                if (!j["sync"]["interval"].empty()) {
                    this->setSyncInterval(j["sync"]["interval"]);
                    m_syncInterval = j["sync"]["interval"];
                }
            }

            // Configure sync logic
            if (!j["idle"].empty()) {
                if (!j["idle"]["word"].empty()) {
                    this->setIdleWord(j["idle"]["word"]);
                    m_idleWord = j["idle"]["word"];
                }
            }

            // Configure Tx speed
            if (!j["cmdPeriod"].empty()) {
                SpecTxCore::m_clk_period = (float)j["cmdPeriod"]; //fix for variant 
            }
            
            // Set number of active lanes
            if (!j["rxActiveLanes"].empty()) {
                this->setRxActiveLanes(j["rxActiveLanes"]);
                SpecRxCore::m_rxActiveLanes = j["rxActiveLanes"];
            }
        }

        void setupMode() override final{
            this->setPulseWord(0x0);
        }

        void runMode() override final {
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
