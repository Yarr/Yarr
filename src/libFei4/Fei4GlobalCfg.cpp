// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: FE-I4 cpp Library
// # Comment: FE-I4 Global Register container
// ################################

#include "Fei4GlobalCfg.h"

#include <fstream>

Fei4GlobalCfg::Fei4GlobalCfg() {
    this->init();
}


void Fei4GlobalCfg::init() {
    // Set memory to 0
    for (unsigned int i=0; i<numRegs; i++)
        cfg[i] = 0;

    // Initialize fields with default config
    // 1
    SME.initReg(cfg, 0, 1, 0x8, 1); regMap["SME"] = &SME;
    EventLimit.initReg(cfg, 0, 1 , 0x0, 8, true); regMap["EventLimit"] = &EventLimit;
    // 2
    Trig_Count.initReg(cfg, 1, 2, 0xC, 4); regMap["Trig_Count"] = &Trig_Count;
    Conf_AddrEnable.initReg(cfg, 1, 2, 0xB, 1); regMap["Conf_AddrEnable"] = &Conf_AddrEnable;
    // 3
    ErrorMask_0.initReg(cfg, 0x4600, 3, 0x0, 16); regMap["ErrorMask_0"] = &ErrorMask_0;
    // 4
    ErrorMask_1.initReg(cfg, 0x0040, 4, 0x0, 16); regMap["ErrorMask_1"] = &ErrorMask_1;
    // 5
    PrmpVbp_R.initReg(cfg, 43, 5, 0x8, 8, true); regMap["PrmpVbp_R"] = &PrmpVbp_R;
    BufVgOpAmp.initReg(cfg, 160, 5, 0x0, 8, true); regMap["BufVgOpAmp"] = &BufVgOpAmp;
    // 6
    PrmpVbp.initReg(cfg, 43, 6, 0x0 , 8, true); regMap["PrmpVbp"] = &PrmpVbp;
    // 7
    TDACVbp.initReg(cfg, 150, 7, 0x8, 8, true); regMap["TDACVbp"] = &TDACVbp;
    DisVbn.initReg(cfg, 40, 7, 0x0, 8, true); regMap["DisVbn"] = &DisVbn;
    // 8
    Amp2Vbn.initReg(cfg, 79, 8, 0x8, 8, true); regMap["Amp2Vbn"] = &Amp2Vbn;
    Amp2VbpFol.initReg(cfg, 26, 8, 0x0, 8, true); regMap["Amp2VbpFol"] = &Amp2VbpFol;
    // 9
    Amp2Vbp.initReg(cfg, 85, 9, 0x0, 8, true); regMap["Amp2Vbp"] = &Amp2Vbp;
    // 10
    FDACVbn.initReg(cfg, 30, 10, 0x8, 8, true); regMap["FDACVbn"] = &FDACVbn;
    Amp2Vbpff.initReg(cfg, 50, 10, 0x0, 8, true); regMap["Amp2Vbpff"] = &Amp2Vbpff;
    // 11
    PrmpVbnFol.initReg(cfg, 106, 11, 0x8, 8, true); regMap["PrmpVbnFol"] = &PrmpVbnFol; 
    PrmpVbp_L.initReg(cfg, 43, 11, 0x0, 8, true); regMap["PrmpVbp_L"] = &PrmpVbp_L;
    // 12
    PrmpVbpf.initReg(cfg, 40, 12, 0x8, 8, true); regMap["PrmpVbpf"] = &PrmpVbpf;
    PrmpVbnLCC.initReg(cfg, 0, 12, 0x0, 8, true); regMap["PrmpVbnLCC"] = &PrmpVbnLCC;
    // 13
    S1.initReg(cfg, 0, 13, 0xF, 1); regMap["S1"] = &S1;
    S0.initReg(cfg, 0, 13, 0xE, 1); regMap["S0"] = &S0;
    Pixel_latch_strobe.initReg(cfg, 0, 13, 0x1, 13, true); regMap["Pixel_latch_strobe"] = &Pixel_latch_strobe;
    // 14
    LVDSDrvIref.initReg(cfg, 171, 14, 0x8, 8, true); regMap["LVDSDrvIref"] = &LVDSDrvIref;
    GADCCompBias.initReg(cfg, 100, 14, 0x0, 8, true); regMap["GADCCompBias"] = &GADCCompBias;
    // 15
    PllIbias.initReg(cfg, 88, 15, 0x8, 8 , true); regMap["PllIbias"] = &PllIbias;
    LVDSDrvVos.initReg(cfg, 105, 15, 0x0, 8, true); regMap["LVDSDrvVos"] = &LVDSDrvVos;
    // 16
    TempSensIbias.initReg(cfg, 0, 16, 0x8, 8, true); regMap["TempSensIbias"] = &TempSensIbias;
    PllIcp.initReg(cfg, 28, 16, 0x0, 8, true); regMap["PllIcp"] = &PllIcp;
    // 17
    PlsrIDACRamp.initReg(cfg, 213, 17, 0x0, 8, true); regMap["PlsrIDACRamp"] = &PlsrIDACRamp;
    // 18
    VrefDigTune.initReg(cfg, 110, 18, 0x8, 8, true); regMap["VrefDigTune"] = &VrefDigTune;
    PlsrVgOpAmp.initReg(cfg, 255, 18, 0x0, 8, true); regMap["PlsrVgOpAmp"] = &PlsrVgOpAmp;
    // 19
    PlsrDACbias.initReg(cfg, 96, 19, 0x8, 8, true); regMap["PlsrDACbias"] = &PlsrDACbias;
    VrefAnTune.initReg(cfg, 50, 19, 0x0, 8, true); regMap["VrefAnTune"] = &VrefAnTune;
    // 20
    Vthin_Coarse.initReg(cfg, 0, 20, 0x8, 8, true); regMap["Vthin_Coarse"] = &Vthin_Coarse;
    Vthin_Fine.initReg(cfg, 150, 20, 0x0, 8, true); regMap["Vthin_Fine"] = &Vthin_Fine;
    // 21
    HitLD.initReg(cfg, 0, 21, 0xC, 1); regMap["HitLD"] = &HitLD;
    DJO.initReg(cfg, 0, 21, 0xB, 1); regMap["DJO"] = &DJO;
    DigHitIn_Sel.initReg(cfg, 0, 21, 0xA, 1); regMap["DigHitIn_Sel"] = &DigHitIn_Sel;
    PlsrDAC.initReg(cfg, 54, 21, 0x0, 10, true); regMap["PlsrDAC"] = &PlsrDAC;
    // 22
    Colpr_Mode.initReg(cfg, 0, 22, 0x8, 2, true); regMap["Colpr_Mode"] = &Colpr_Mode;
    Colpr_Addr.initReg(cfg, 0, 22, 0x2, 6, true); regMap["Colpr_Addr"] = &Colpr_Addr;
    // 23
    DisableColCnfg0.initReg(cfg, 0, 23, 0x0, 16); regMap["DisableColCnfg0"] = &DisableColCnfg0;
    // 24
    DisableColCnfg1.initReg(cfg, 0, 24, 0x0, 16); regMap["DisableColCnfg1"] = &DisableColCnfg1;
    // 25
    Trig_Lat.initReg(cfg, 210, 25, 0x8, 8); regMap["Trig_Lat"] = &Trig_Lat;
    DisableColCnfg2.initReg(cfg, 0, 25, 0x0, 8); regMap["DisableColCnfg2"] = &DisableColCnfg2;
    // 26
    CMDcnt12.initReg(cfg, 0, 26, 0x3, 13); regMap["CMDcnt12"] = &CMDcnt12;
    CalPulseWidth.initReg(cfg, 10, 26, 0x3, 8); regMap["CalPulseWidth"] = &CalPulseWidth;
    CalPulseDelay.initReg(cfg, 0, 26, 0xB, 5); regMap["CalPulseDelay"] = &CalPulseDelay;
    StopModeConfig.initReg(cfg, 0, 26, 0x2, 1); regMap["StopModeConfig"] = &StopModeConfig;
    HitDiscCnfg.initReg(cfg, 0, 26, 0x0, 2); regMap["HitDiscCnfg"] = &HitDiscCnfg;
    // 27
    PLL_Enable.initReg(cfg, 1, 27, 0xF, 1); regMap["PLL_Enable"] = &PLL_Enable;
    EFS.initReg(cfg, 0, 27, 0xE, 1); regMap["EFS"] = &EFS;
    StopClkPulse.initReg(cfg, 0, 27, 0xD, 1); regMap["StopClkPulse"] = &StopClkPulse;
    ReadErrorReq.initReg(cfg, 0, 27, 0xC, 1); regMap["ReadErrorReq"] = &ReadErrorReq;
    GADC_En.initReg(cfg, 0, 27, 0xA, 1); regMap["GADC_En"] = &GADC_En;
    SRRead.initReg(cfg, 0, 27, 0x9, 1); regMap["SRRead"] = &SRRead;
    HitOr.initReg(cfg, 0, 27, 0x5, 1); regMap["HitOr"] = &HitOr;
    CalEn.initReg(cfg, 0, 27, 0x4, 1); regMap["CalEn"] = &CalEn;
    SRClr.initReg(cfg, 0, 27, 0x3, 1); regMap["SRClr"] = &SRClr;
    Latch_Enable.initReg(cfg, 0, 25, 0x2, 1); regMap["Latch_Enable"] = &Latch_Enable;
    SR_Clock.initReg(cfg, 0, 25, 0x1, 1); regMap["SR_Clock"] = &SR_Clock;
    M13.initReg(cfg, 0, 25, 0x0, 1); regMap["M13"] = &M13;
    // 28
    LVDSDrvSet06.initReg(cfg, 1, 28, 0xF, 1); regMap["LVDSDrvSet06"] = &LVDSDrvSet06;
    EN_40M.initReg(cfg, 1, 28, 0x9, 1); regMap["EN_40M"] = &EN_40M;
    EN_80M.initReg(cfg, 0, 28, 0x8, 1); regMap["EN_80M"] = &EN_80M;
    CLK1_S0.initReg(cfg, 0, 28, 0x7, 1); regMap["CLK1_S0"] = &CLK1_S0;
    CLK1_S1.initReg(cfg, 0, 28, 0x6, 1); regMap["CLK1_S1"] = &CLK1_S1;
    CLK1_S2.initReg(cfg, 0, 28, 0x5, 1); regMap["CLK1_S2"] = &CLK1_S2;
    CLK0_S0.initReg(cfg, 0, 28, 0x4, 1); regMap["CLK0_S0"] = &CLK0_S0;
    CLK0_S1.initReg(cfg, 0, 28, 0x3, 1); regMap["CLK0_S1"] = &CLK0_S1;
    CLK0_S2.initReg(cfg, 1, 28, 0x2, 1); regMap["CLK0_S2"] = &CLK0_S2;
    EN_160.initReg(cfg, 1, 28, 0x1, 1); regMap["EN_160"] = &EN_160;
    EN_320.initReg(cfg, 0, 28, 0x0, 1); regMap["EN_320"] = &EN_320;
    // 29
    No8b10b.initReg(cfg, 0, 29, 0xD, 1); regMap["No8b10b"] = &No8b10b;
    Clk2Out.initReg(cfg, 0, 29, 0xC, 1); regMap["Clk2Out"] = &Clk2Out;
    EmptyRecordCnfg.initReg(cfg, 0, 29, 0x4, 8); regMap["EmptyRecordCnfg"] = &EmptyRecordCnfg;
    LVDSDrvEn.initReg(cfg, 1, 29, 0x2, 1); regMap["LVDSDrvEn"] = &LVDSDrvEn;
    LVDSDrvSet30.initReg(cfg, 1, 29, 0x1, 1); regMap["LVDSDrvSet30"] = &LVDSDrvSet30;
    LVDSDrvSet12.initReg(cfg, 1, 29, 0x0, 1); regMap["LVDSDrvSet12"] = &LVDSDrvSet12;
    // 30
    TmpSensDiodeSel.initReg(cfg, 0, 30, 0xE, 2); regMap["TmpSensDiodeSel"] = &TmpSensDiodeSel;
    TmpSensDisable.initReg(cfg, 0, 30, 0xD, 1); regMap["TmpSensDisable"] = &TmpSensDisable;
    IleakRange.initReg(cfg, 0, 30, 0xC, 1); regMap["IleakRange"] = &IleakRange;
    // 31
    PlsrRiseUpTau.initReg(cfg, 7, 31, 0xD, 3); regMap["PlsrRiseUpTau"] = &PlsrRiseUpTau;
    PlsrPwr.initReg(cfg, 1, 31, 0xC, 1); regMap["PlsrPwr"] = &PlsrPwr;
    PlsrDelay.initReg(cfg, 2, 31, 0x6, 6, true); regMap["PlsrDelay"] = &PlsrDelay;
    ExtDigCalSW.initReg(cfg, 0, 31, 0x5, 1); regMap["ExtDigCalSW"] = &ExtDigCalSW;
    ExtAnaCalSW.initReg(cfg, 0, 31, 0x4, 1); regMap["ExtAnaCalSW"] = &ExtAnaCalSW;
    GADCSel.initReg(cfg, 0, 31, 0x0, 3); regMap["GADCSel"] = &GADCSel;
    // 32
    SELB0.initReg(cfg, 0, 32, 0x0, 16); regMap["SELB0"] = &SELB0;
    // 33
    SELB1.initReg(cfg, 0, 33, 0x0, 16); regMap["SELB1"] = &SELB1;
    // 34
    SELB2.initReg(cfg, 0, 34, 0x8, 8); regMap["SELB2"] = &SELB2;
    PrmpVbpMsbEn.initReg(cfg, 0, 34, 0x4, 1); regMap["PrmpVbpMsbEn"] = &PrmpVbpMsbEn;
    //35
    EFUSE.initReg(cfg, 0, 35, 0x0, 16);
}


void Fei4GlobalCfg::toFilePlain(std::string filename) {
    std::fstream file(filename, std::fstream::out | std::fstream::trunc);
    
    file << "# FEI4B-global-config" << std::endl;
    file << "# ###################" << std::endl;
    
    file << "SME              " << SME.value() << std::endl;
    file << "EventLimit       " << EventLimit.value() << std::endl;
    file << "Trig_Count       " << Trig_Count.value() << std::endl;
    file << "Conf_AddrEnable  " << Conf_AddrEnable.value() << std::endl;
    file << "ErrorMask_0      " << ErrorMask_0.value() << std::endl;
    file << "ErrorMask_1      " << ErrorMask_1.value() << std::endl;
    file << "PrmpVbp_R        " << PrmpVbp_R.value() << std::endl;
    
    file.close();

}

void Fei4GlobalCfg::toFileXml(tinyxml2::XMLDocument *doc, tinyxml2::XMLElement *node) {
    tinyxml2::XMLElement *gcfg = doc->NewElement("GlobalConfig");
    
    tinyxml2::XMLElement *reg = NULL;
    typedef std::map<std::string, Fei4Register*>::iterator it_type;
    for(it_type iterator = regMap.begin(); iterator != regMap.end(); iterator++) {
        reg = doc->NewElement(iterator->first.c_str());
        reg->SetAttribute("value", iterator->second->value());
        reg->SetAttribute("type", "dec");
        gcfg->LinkEndChild(reg);
    }

    node->LinkEndChild(gcfg);
}

void Fei4GlobalCfg::toFileJson(json &j) {
    typedef std::map<std::string, Fei4Register*>::iterator it_type;
    for(it_type iterator = regMap.begin(); iterator != regMap.end(); iterator++) {
         j["FE-I4B"]["GlobalConfig"][iterator->first] = iterator->second->value();
    }

}

void Fei4GlobalCfg::fromFileJson(json &j) {
    typedef std::map<std::string, Fei4Register*>::iterator it_type;
    for(it_type iterator = regMap.begin(); iterator != regMap.end(); iterator++) {
        if (!j["FE-I4B"]["GlobalConfig"][iterator->first].empty())
            iterator->second->write((uint16_t) j["FE-I4B"]["GlobalConfig"][iterator->first]);
    }

}
