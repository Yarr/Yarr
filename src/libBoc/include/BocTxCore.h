#ifndef BOCTXCORE_H
#define BOCTXCORE_H

#include <iostream>
#include <stdint.h>
#include <thread>
#include <chrono>
#include <queue>

#include "BocCom.h"
#include "TxCore.h"

class BocTxCore : virtual public TxCore, virtual public BocCom {
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
        void setTrigWord(uint32_t *word); // 4 words, start at Msb
        void toggleTrigAbort();

        // Trigger interface
        void setTriggerLogicMask(uint32_t mask);
        void setTriggerLogicMode(enum TRIG_LOGIC_MODE_VALUE mode);
        void resetTriggerLogic();
        uint32_t getTrigInCount();

    private:
    	std::thread *m_trigThread;
    	void trigThreadProc();
    	volatile bool m_trigThreadRunning;

    	// trigger configuration
    	uint32_t m_trigWord[4];
    	uint32_t m_trigCount;
    	double m_trigTime;
    	double m_trigFreq;
    	uint32_t m_trigMask;

    	// enable mask and command sending
    	uint32_t m_enableMask;
    	bool m_isSending;

        // intermediate FIFO for storage
        std::queue<uint8_t> cmdFifo;
};

#endif // BOCTXCORE_H
