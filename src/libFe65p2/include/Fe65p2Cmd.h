#ifndef FE65P2CMD_H
#define FE65P2CMD_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: FE-I4 Library
// # Comment: Collection of FE-I4 commands
// ################################

#include <stdint.h>
#include <unistd.h>

#include "TxCore.h"
#include "Fe65p2GlobalCfg.h"
#include "Fe65p2PixelCfg.h"

#define MOJO_HEADER 0x80000000

#define GLOBAL_REG_BASE 0x0
#define PIXEL_REG_BASE 0x20
#define STATIC_REG 0x30
#define PULSE_REG 0x31
#define LAT_REG 0x32
#define DAC_REG 0x33
#define TRIGCNT_REG 0x34
#define DELAY_REG 0x35

#define PULSE_SHIFT_GLOBAL 0x1
#define PULSE_INJECT 0x2
#define PULSE_SHIFT_PIXEL 0x4
#define PULSE_LOAD 0x8
#define PULSE_SHIFTBYONE 0x10 //untestead
#define PULSE_LOADDAC 0x20
#define PULSE_SWITCHPLSR 0x40
#define PULSE_TRIGGER 0x80

#define STATIC_DATA_CLK 0x1
#define STATIC_BX_CLK 0x2
#define STATIC_PIXDCONF 0x4
#define STATIC_ANINJ 0x8
#define STATIC_RST0 0x10
#define STATIC_RST1 0x20

class Fe65p2Cmd {
    public:
        Fe65p2Cmd();
        Fe65p2Cmd(TxCore *arg_core);
        ~Fe65p2Cmd();
        
        void setCore(TxCore *arg_core);

        void writeGlobal(uint16_t *cfg);
        void writePixel(uint16_t *bitstream);
        void writePixel(uint16_t mask);
        
        void setLatency(uint16_t lat);
        void injectAndTrigger();
        void setPlsrDac(unsigned setting);
        void setTrigCount(uint32_t setting);
        void setPulserDelay(uint32_t setting);

        void reset();
        void clocksOn();
        void clocksOff();

        void enAnaInj();
        void enDigInj();

    private:
        TxCore *core;
        uint32_t static_reg;

        void setStaticReg(uint32_t bit);
        void unsetStaticReg(uint32_t bit);
        void writeStaticReg();
};

#endif
