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
#include "json.hpp"

using json=nlohmann::basic_json<std::map, std::vector, std::string, bool, std::int32_t, std::uint32_t, float>;

class SpecController : public HwController, public SpecTxCore, public SpecRxCore {
    public:

        void loadConfig(json &j) override {
            if (!j["specNum"].empty())
                this->SpecCom::init(j["specNum"]);
            
            // Set direction of LVDS lines
            if (!j["spiConfig"].empty()) {
                // TODO make proper function
                this->writeSingle(0x6<<14 | 0x0, (uint32_t)j["spiConfig"]);
                this->writeSingle(0x6<<14 | 0x1, 0xF);
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

            // Configure auto-zero logic
            if (!j["autoZero"].empty()) {
                if (!j["autoZero"]["word"].empty()) {
                    this->setAzWord(j["autoZero"]["word"]);
                    m_azWord = j["autoZero"]["word"];
                }
                if (!j["autoZero"]["interval"].empty()) {
                    this->setAzInterval(j["autoZero"]["interval"]);
                    m_azInterval = j["autoZero"]["interval"];
                }
            }

            // Configure Tx speed
            if (!j["cmdPeriod"].empty()) {
                SpecTxCore::m_clk_period = j["cmdPeriod"];
            }
        }

        void setupMode() override final{
            this->setAzWord(0x0);
        }

        void runMode() override final {
            this->setAzWord(m_azWord);
        }

    private:
        uint32_t m_azWord;
        uint32_t m_azInterval;
};

#endif
