#include "Fe65p2GlobalCfg.h"

Fe65p2GlobalCfg::Fe65p2GlobalCfg() {
    this->init();
}

Fe65p2GlobalCfg::~Fe65p2GlobalCfg() {

}

void Fe65p2GlobalCfg::init() {
        for(unsigned i=0; i<numRegs; i++)
            cfg[i] = 0x0;
        
        TestHit.initReg(cfg, 0x00, 0);
        SignLd.initReg(cfg, 0x01, 0);
        InjEnLd.initReg(cfg, 0x02, 0);
        TDacLd.initReg(cfg, 0x03, 0);
        PixConfLd.initReg(cfg, 0x04, 0);
        SPARE_0.initReg(cfg, 0x05, 0);

        OneSr.initReg(cfg, 0x06, 0);
        HitDisc.initReg(cfg, 0x07, 0);
        Latency.initReg(cfg, 0x08, 30);
        ColEn.initReg(cfg, 0x09, 0xFFFF);
        ColSrEn.initReg(cfg, 0x0a, 0xFFFF);
        ColSrOut.initReg(cfg, 0x0b, 15);
        SPARE_1.initReg(cfg, 0x0c, 0);
        PrmpVbpDac.initReg(cfg, 0x0d, 36);
        Vthin1Dac.initReg(cfg, 0x0e, 255);
        Vthin2Dac.initReg(cfg, 0x0f, 0);
        VffDac.initReg(cfg, 0x10, 50);
        VctrCF0Dac.initReg(cfg, 0x11, 0);
        VctrCF1Dac.initReg(cfg, 0x12, 0);
        PrmpVbnFolDac.initReg(cfg, 0x13, 50);
        VbnLccDac.initReg(cfg, 0x14, 1);
        CompVbnDacConf.initReg(cfg, 0x15, 25);
        PreCompVbnDac.initReg(cfg, 0x16, 50);
}
