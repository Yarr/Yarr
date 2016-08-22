#include "Fe65p2GlobalCfg.h"

Fe65p2GlobalCfg::Fe65p2GlobalCfg() {
    this->init();
}

Fe65p2GlobalCfg::~Fe65p2GlobalCfg() {

}

void Fe65p2GlobalCfg::init() {
        for(unsigned i=0; i<numRegs; i++)
            cfg[i] = 0x0;
        
        TestHit.initReg(cfg, 0x00, 0); regMap["TestHit"] = &TestHit;
        SignLd.initReg(cfg, 0x01, 0); regMap["SignLd"] = &SignLd;
        InjEnLd.initReg(cfg, 0x02, 0); regMap["InjEnLd"] = &InjEnLd;
        TDacLd.initReg(cfg, 0x03, 0); regMap["TDacLd"] = &TDacLd;
        PixConfLd.initReg(cfg, 0x04, 0); regMap["PixConfLd"] = &PixConfLd;
        SPARE_0.initReg(cfg, 0x05, 0); // unused

        OneSr.initReg(cfg, 0x06, 0); regMap["OneSr"] = &OneSr;
        HitDisc.initReg(cfg, 0x07, 0); // not connected
        Latency.initReg(cfg, 0x08, 30); regMap["Latency"] = &Latency;
        ColEn.initReg(cfg, 0x09, 0xFFFF); regMap["ColEn"] = &ColEn;
        ColSrEn.initReg(cfg, 0x0a, 0xFFFF); regMap["ColSrEn"] = &ColSrEn;
        ColSrOut.initReg(cfg, 0x0b, 15); regMap["ColSrOut"] = &ColSrOut;
        SPARE_1.initReg(cfg, 0x0c, 0); // unused
        PrmpVbpDac.initReg(cfg, 0x0d, 40); regMap["PrmpVbpDac"] = &PrmpVbpDac;
        Vthin1Dac.initReg(cfg, 0x0e, 100); regMap["Vthin1Dac"] = &Vthin1Dac;
        Vthin2Dac.initReg(cfg, 0x0f, 30); regMap["Vthin2Dac"] = &Vthin2Dac;
        VffDac.initReg(cfg, 0x10, 25); regMap["VffDac"] = &VffDac;
        VctrCF0Dac.initReg(cfg, 0x11, 0); // not connected
        VctrCF1Dac.initReg(cfg, 0x12, 0); // not connected
        PrmpVbnFolDac.initReg(cfg, 0x13, 50); regMap["PrmpVbnFolDac"] = &PrmpVbnFolDac;
        VbnLccDac.initReg(cfg, 0x14, 1); regMap["VbnLccDac"] = &VbnLccDac;
        CompVbnDac.initReg(cfg, 0x15, 25); regMap["CompVbnDac"] = &CompVbnDac;
        PreCompVbnDac.initReg(cfg, 0x16, 50); regMap["PreCompVbnDac"] = &PreCompVbnDac;
        
        // Only in firmware
        PlsrDac.initReg(&plsrDacReg, 0x0, 0); regMap["PlsrDac"] = &PlsrDac;
        PlsrDelay.initReg(&plsrDelayReg, 0x0, 0); regMap["PlsrDelay"] = &PlsrDelay;
        TrigCount.initReg(&trigCountReg, 0x0, 10); regMap["TrigCount"] = &TrigCount;
}

void Fe65p2GlobalCfg::toFileJson(json &j) {
    typedef std::map<std::string, Fe65p2GlobalReg*>::iterator it_type;
    for(it_type iterator = regMap.begin(); iterator != regMap.end(); iterator++) {
         j["FE65-P2"]["GlobalConfig"][iterator->first] = iterator->second->read();
    }

}

void Fe65p2GlobalCfg::fromFileJson(json &j) {
    typedef std::map<std::string, Fe65p2GlobalReg*>::iterator it_type;
    for(it_type iterator = regMap.begin(); iterator != regMap.end(); iterator++) {
        if (!j["FE65-P2"]["GlobalConfig"][iterator->first].empty()) {
            iterator->second->write((uint16_t) j["FE65-P2"]["GlobalConfig"][iterator->first]);
        } else {
            std::cout << "Fe65p2GlobalCfg: Could not find register \"" << iterator->first << "\" using default value!" << std::endl;
        }
    }

}
