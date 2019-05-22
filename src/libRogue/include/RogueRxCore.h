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
        void maskRxEnable(uint32_t val, uint32_t mask) {}

        RawData* readData();
        
        uint32_t getDataRate() {return 0;}
        uint32_t getCurCount() {return m_com->getCurSize();}
        bool isBridgeEmpty() {return m_com->isEmpty();}

    private:
	std::shared_ptr<RogueCom> m_com;
};

#endif
