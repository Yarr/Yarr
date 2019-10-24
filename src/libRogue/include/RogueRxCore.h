#ifndef __ROGUE_RXCORE_H__
#define __ROGUE_RXCORE_H__

#include <memory>
#include <iostream>
#include "RxCore.h"
#include "RogueCom.h"

class RogueRxCore : virtual public RxCore {
	public:
		RogueRxCore();
		~RogueRxCore();

		void setRxEnable(uint32_t val) {}
		void setRxEnable(std::vector<uint32_t>)  {}
		void maskRxEnable(uint32_t val, uint32_t mask) {}

		RawData* readData();

		uint32_t getDataRate() {return 0;}
		uint32_t getCurCount() {m_com->setRxChannel(m_rxChannel);return m_com->getCurSize();}
		bool isBridgeEmpty() {m_com->setRxChannel(m_rxChannel);return m_com->isEmpty();}
		void setRxChannel(uint32_t rxchannel){m_rxChannel=rxchannel;} 
		uint32_t getRxChannel(){return m_rxChannel;} 

		void flushBuffer(){
			//std::cout << __PRETTY_FUNCTION__ << std::endl;
			m_com->flushBuffer();
		}

	private:
		std::shared_ptr<RogueCom> m_com;
		uint32_t m_rxChannel;
};

#endif
