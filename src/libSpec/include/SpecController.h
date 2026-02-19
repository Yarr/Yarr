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
        SpecController() {
            m_waitTime = std::chrono::microseconds(1000);
            m_timeoutTime = std::chrono::microseconds(50000);
        }

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

            if (j.contains("rxWaitTime")) {
                m_waitTime = std::chrono::microseconds(j["rxWaitTime"]);
            }
            
            if (j.contains("rxTimeoutTime")) {
                m_timeoutTime = std::chrono::microseconds(j["rxTimeoutTime"]);
            }
            
            // Configure trigger logic (disable trigger encoder by default)
            if (j.contains("trigConfig")) {
                auto &tc = j["trigConfig"];
                if(tc.contains("mask"))
                    this->setTriggerLogicMask(tc["mask"]);
                
                if(tc.contains("mode"))
                    this->setTriggerLogicMode(tc["mode"]);
                
                if(tc.contains("config"))
                    this->setTriggerLogicConfig(tc["config"]);
                
                if(tc.contains("edge"))
                    this->setTriggerEdge(tc["edge"]);
                
                if(tc.contains("delay")) {
                    if(tc["delay"].size() == 4) {
                        for (unsigned i=0; i<4; i++) {
                            this->setTriggerDelay(i, tc["delay"][i]);
                        }
                    }
                }
                if(tc.contains("deadtime"))
                    this->setTriggerDeadtime(tc["deadtime"]);

                if(tc.contains("triggerEncoderMultiplier"))
                    this->setTriggerEncoderMultiplier(tc["triggerEncoderMultiplier"]);
                else
                    this->setTriggerEncoderMultiplier(0);
                    
                if(tc.contains("triggerEncoderEnable"))
                    this->setTriggerEncoderEnable(tc["triggerEncoderEnable"]);
                else
                    this->setTriggerEncoderEnable(0);

                // Configure trigger pulse extension (default 1BC, no extension)
                if(tc.contains("triggerPulseExtension"))
                    this->setTriggerPulseExtension(tc["triggerPulseExtension"]);
                else
                    this->setTriggerPulseExtension(1);

                // configure eudet simple mode
                if(tc.contains("eudetSimpleModeEnable"))
                    this->setEudetSimpleMode(tc["eudetSimpleModeEnable"]);

            }
            else {
                // Turn off trigger encoder by default
                this->setTriggerEncoderEnable(0);
                this->setTriggerEncoderMultiplier(0);
            }

            // Configure pulse logic
            if (j.contains("pulse")) {
                auto &jp = j["pulse"];
                if (jp.contains("word")) {
                    this->setPulseWord(jp["word"]);
                    m_pulseWord = jp["word"];
                }
                if (jp.contains("interval")) {
                    this->setPulseInterval(jp["interval"]);
                    m_pulseInterval = jp["interval"];
                }
            }
            
            // Configure sync logic
            if (j.contains("sync")) {
                auto &js = j["sync"];
                if (js.contains("word")) {
                    this->setSyncWord(js["word"]);
                    m_syncWord = js["word"];
                }
                if (js.contains("interval")) {
                    this->setSyncInterval(js["interval"]);
                    m_syncInterval = js["interval"];
                }
            }

            // Configure sync logic
            if (j.contains("idle")) {
                auto &ji = j["idle"];
                if (ji.contains("word")) {
                    this->setIdleWord(ji["word"]);
                    m_idleWord = ji["word"];
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
 
            // Configure RX delay
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

            // Configure BRAM busy enable (default on)
            if(j.contains("BRAMBusyEnable"))
                this->setBRAMBusyEnable(j["BRAMBusyEnable"]);
            else
                this->setBRAMBusyEnable(1);

            // Configure BRAM full threshold (default 80% fill)
            if(j.contains("BRAMFullThreshold"))
                this->setBRAMFullThreshold(j["BRAMFullThreshold"]);
            else
                this->setBRAMFullThreshold((int)(2048*64*0.8));

            // Configure BRAM empty threshold (default 40% fill)
            if(j.contains("BRAMEmptyThreshold"))
                this->setBRAMEmptyThreshold(j["BRAMEmptyThreshold"]);
            else
                this->setBRAMEmptyThreshold((int)(2048*64*0.4));
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
