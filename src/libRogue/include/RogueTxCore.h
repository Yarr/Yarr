#ifndef __ROGUE_TXCORE_H__
#define __ROGUE_TXCORE_H__



#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <memory>

#include "TxCore.h"
#include "RogueCom.h"

class RogueTxCore : virtual public TxCore {
    public:
        RogueTxCore();
        ~RogueTxCore() override;

        void setCom(std::shared_ptr<RogueCom> com) {m_com = com;}
	std::shared_ptr<RogueCom> getCom() {return m_com;}

        void writeFifo(uint32_t value) override;
        void releaseFifo() override {m_com->setTxChannel(m_txChannel);m_com->releaseFifo();} // Add some padding
		void setForceRelaseTxfifo(bool enable=true) { m_com->setTxChannel(m_txChannel);m_com->setForceRelaseTxfifo(enable);}
        
        void setCmdEnable(uint32_t value) override { }
        void setCmdEnable(std::vector<uint32_t>) override {}
        void disableCmd() override {}
        uint32_t getCmdEnable() override {return 0x0;}
        void maskCmdEnable(uint32_t value, uint32_t mask) {}

        void setTrigEnable(uint32_t value) override;
        uint32_t getTrigEnable() override {m_com->setTxChannel(m_txChannel);return  trigProcRunning || !m_com->isEmpty();}
        void maskTrigEnable(uint32_t value, uint32_t mask) override {}

        void setTrigConfig(enum TRIG_CONF_VALUE cfg) override {m_trigCfg = cfg;} 
        void setTrigFreq(double freq) override {m_trigFreq = freq;}
        void setTrigCnt(uint32_t count) override;
        void setTrigTime(double time) override;
        //void setTrigTime(double time) {m_trigTime = time;}
        void setTrigWordLength(uint32_t length) override; // TODO length here is bits, should be words
        void setTrigWord(uint32_t *word, uint32_t length) override {
	  for(unsigned i=0; i<length; i++) {
	    m_trigWord[i] = word[i];	  
	    //	    printf("%08x ",word[i]);
	    //  printf("\n");
	  }
	} 

        void toggleTrigAbort() override {
			trigProcRunning =false;
		}

        bool isCmdEmpty() override {
			m_com->setTxChannel(m_txChannel);
            bool rtn = m_com->isEmpty();
            return rtn;
        }
        bool isTrigDone() override {
			m_com->setTxChannel(m_txChannel);
	  //	  bool rtn = triggerProc.joinable() && m_com->isEmpty();
	  bool rtn = !trigProcRunning &&  m_com->isEmpty();
	  return rtn;
        }

        uint32_t getTrigInCount() override {return 0x0;}
        
        void setTriggerLogicMask(uint32_t mask) override {}
        void setTriggerLogicMode(enum TRIG_LOGIC_MODE_VALUE mode) override {}
        void resetTriggerLogic() override {}
		void setTxChannel(uint32_t txChannel){m_txChannel=txChannel;} 

    private:
		uint32_t m_txChannel;
	std::shared_ptr<RogueCom> m_com;

        std::mutex accMutex;
        std::thread triggerProc;
	void setChannelInMask(unsigned);
	void setChannelOutMask(unsigned);
	
        bool trigProcRunning;
        
        void doTriggerCnt();
        void doTriggerTime();

        enum TRIG_CONF_VALUE m_trigCfg;
        unsigned m_trigCnt;
	unsigned m_trigWordLength;
        uint32_t m_trigWord[32]; // Repeated bitstream max length is 512 bits, in software there is not really a limit
        double m_trigFreq;
        //double m_trigTime;
};

#endif
