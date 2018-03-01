#ifndef KU040TXCORE_H
#define KU040TXCORE_H

#include <iostream>
#include <stdint.h>
#include <thread>
#include <chrono>
#include <queue>

#include "IPbus.h"
#include "TxCore.h"

class KU040TxCore : virtual public TxCore {
	public:
		// constructor/destructor
		KU040TxCore();
		~KU040TxCore();

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

		void DumpTxCounters();

        void setCom(IPbus *com) {
            m_com = com;
        }

        IPbus *getCom() {
            return m_com;
        }

    private:
    	// trigger configuration
    	uint32_t m_trigMask;
        enum TRIG_CONF_VALUE m_trigCfg;

    	// enable mask and command sending
    	uint32_t m_enableMask;
    	bool m_isSending;

        // intermediate FIFO for storage
        std::queue<uint32_t> cmdFifo;
        IPbus *m_com;
};

#endif // BOCTXCORE_H
