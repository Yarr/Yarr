#ifndef __RD53A_EMU_H__
#define __RD53A_EMU_H__

#define HEXF(x,y) std::hex << "0x" << std::hex << std::setw(x) << std::setfill('0') << static_cast<int>(y) << std::dec

#include "Rd53aCfg.h"
#include "EmuShm.h"
#include "RingBuffer.h"
#include "Gauss.h"
#include "PixelModel.h"
#include "FrontEndGeometry.h"
#include "json.hpp"

#include "RingBuffer.h"

#include <cstdint>
#include <memory>

class Rd53aEmu {
    public:
        Rd53aEmu(RingBuffer * rx, RingBuffer * tx);
        ~Rd53aEmu();

        // the main loop which recieves commands from yarr
        void executeLoop();

        // functions for dealing with sending data to yarr
        void pushOutput(uint32_t value);

        RingBuffer * m_txRingBuffer;
        RingBuffer * m_rxRingBuffer;
        std::shared_ptr<Rd53aCfg> m_feCfg;

        uint32_t m_header;
        uint32_t m_id_address_some_data;
        uint32_t m_small_data;

        volatile bool run;
};

#endif //__RD53A_EMU_H__
