#ifndef __ROGUE_RXCORE_H__
#define __ROGUE_RXCORE_H__

#include <memory>
#include <iostream>
#include "RxCore.h"
#include "RogueCom.h"

class RogueRxCore : virtual public RxCore {
	public:
		RogueRxCore();
		~RogueRxCore() override;

		void setRxEnable(uint32_t val) override {}
		void setRxEnable(std::vector<uint32_t>) override  {}
        void disableRx() override {}
		void maskRxEnable(uint32_t val, uint32_t mask) override {}

        std::vector<RawDataPtr> readData() override;

		uint32_t getDataRate() override {return 0;}
		uint32_t getCurCount() override {m_com->setRxChannel(m_rxChannel);return m_com->getCurSize();}
		bool isBridgeEmpty() override {m_com->setRxChannel(m_rxChannel);return m_com->isEmpty();}
		void setRxChannel(uint32_t rxchannel){m_rxChannel=rxchannel;} 
		uint32_t getRxChannel() const{return m_rxChannel;}

		void flushBuffer() override{
			//std::cout << __PRETTY_FUNCTION__ << std::endl;
			m_com->flushBuffer();
		}

	private:
		std::shared_ptr<RogueCom> m_com;
		uint32_t m_rxChannel;
};

#endif
