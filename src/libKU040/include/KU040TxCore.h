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
		~KU040TxCore() override;

        // Write to FE interface
        void writeFifo(uint32_t) override;
        void setCmdEnable(uint32_t) override;
        void setCmdEnable(std::vector<uint32_t> channels) override;
        void disableCmd() override;
        uint32_t getCmdEnable() override;
        bool isCmdEmpty() override;
        void releaseFifo() override;

        // Word repeater TODO: move to seperate class?
        void setTrigEnable(uint32_t value) override;
        uint32_t getTrigEnable() override;
        void maskTrigEnable(uint32_t value, uint32_t mask) override;
        bool isTrigDone() override;


        void setTrigConfig(enum TRIG_CONF_VALUE cfg) override;
        void setTrigFreq(double freq) override; // in Hz
        void setTrigCnt(uint32_t count) override;
        void setTrigTime(double time) override; // in s
        void setTrigWordLength(uint32_t length) override; // From Msb
        void setTrigWord(uint32_t *word, uint32_t length) override; // 4 words, start at Msb
        void toggleTrigAbort() override;

        // Trigger interface
        void setTriggerLogicMask(uint32_t mask) override;
        void setTriggerLogicMode(enum TRIG_LOGIC_MODE_VALUE mode) override;
        void resetTriggerLogic() override;
        uint32_t getTrigInCount() override;

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
