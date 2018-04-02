#ifndef BOCTXCORE_H
#define BOCTXCORE_H

#include <iostream>
#include <stdint.h>
#include <thread>
#include <chrono>
#include <queue>

#include "BocCom.h"
#include "TxCore.h"

class BocTxCore : virtual public TxCore {
	public:
		// constructor/destructor
		BocTxCore();
		~BocTxCore();

        // Write to FE interface
        void writeFifo(uint32_t);
        void setCmdEnable(uint32_t);
        uint32_t getCmdEnable();
        bool isCmdEmpty();
        void releaseFifo();

        // Word repeater TODO: move to seperate class?
        void setTrigEnable(uint32_t value);
        uint32_t getTrigEnable();
        void maskTrigEnable(uint32_t value, uint32_t mask);
        bool isTrigDone();


        void setTrigConfig(enum TRIG_CONF_VALUE cfg);
        void setTrigFreq(double freq); // in Hz
        void setTrigCnt(uint32_t count);
        void setTrigTime(double time); // in s
        void setTrigWordLength(uint32_t length); // From Msb
        void setTrigWord(uint32_t *word, uint32_t length); // 4 words, start at Msb
        void toggleTrigAbort();

        // Trigger interface
        void setTriggerLogicMask(uint32_t mask);
        void setTriggerLogicMode(enum TRIG_LOGIC_MODE_VALUE mode);
        void resetTriggerLogic();
        uint32_t getTrigInCount();

        void setCom(BocCom *com) {
            m_com = com;
        }

        BocCom *getCom() {
            return m_com;
        }

    private:
    	// trigger configuration
    	//uint32_t m_trigWord[4];
    	uint32_t m_trigCount;
    	double m_trigTime;
    	double m_trigFreq;
    	uint32_t m_trigMask;
        enum TRIG_CONF_VALUE m_trigCfg;

    	// enable mask and command sending
    	uint32_t m_enableMask;
    	bool m_isSending;

        // intermediate FIFO for storage
        std::queue<uint8_t> cmdFifo;
        BocCom *m_com;
};

#endif // BOCTXCORE_H
