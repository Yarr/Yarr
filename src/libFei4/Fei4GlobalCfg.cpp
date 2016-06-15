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
    SME.initField(cfg, 0); fieldMap["SME"] = &SME;
    EventLimit.initField(cfg, 0); fieldMap["EventLimit"] = &EventLimit;
    // 2
    Trig_Count.initField(cfg, 1); fieldMap["Trig_Count"] = &Trig_Count;
    Conf_AddrEnable.initField(cfg, 1); fieldMap["Conf_AddrEnable"] = &Conf_AddrEnable;
    // 3
    ErrorMask_0.initField(cfg, 0x4600); fieldMap["ErrorMask_0"] = &ErrorMask_0;
    // 4
    ErrorMask_1.initField(cfg, 0x0040); fieldMap["ErrorMask_1"] = &ErrorMask_1;
    // 5
    PrmpVbp_R.initField(cfg, 43); fieldMap["PrmpVbp_R"] = &PrmpVbp_R;
    BufVgOpAmp.initField(cfg, 160); fieldMap["BufVgOpAmp"] = &BufVgOpAmp;
    // 6
    PrmpVbp.initField(cfg, 43); fieldMap["PrmpVbp"] = &PrmpVbp;
    // 7
    TDACVbp.initField(cfg, 150); fieldMap["TDACVbp"] = &TDACVbp;
    DisVbn.initField(cfg, 40); fieldMap["DisVbn"] = &DisVbn;
    // 8
    Amp2Vbn.initField(cfg, 79); fieldMap["Amp2Vbn"] = &Amp2Vbn;
    Amp2VbpFol.initField(cfg, 26); fieldMap["Amp2VbpFol"] = &Amp2VbpFol;
    // 9
    Amp2Vbp.initField(cfg, 85); fieldMap["Amp2Vbp"] = &Amp2Vbp;
    // 10
    FDACVbn.initField(cfg, 30); fieldMap["FDACVbn"] = &FDACVbn;
    Amp2Vbpff.initField(cfg, 50); fieldMap["Amp2Vbpff"] = &Amp2Vbpff;
    // 11
    PrmpVbnFol.initField(cfg, 106); fieldMap["PrmpVbnFol"] = &PrmpVbnFol; 
    PrmpVbp_L.initField(cfg, 43); fieldMap["PrmpVbp_L"] = &PrmpVbp_L;
    // 12
    PrmpVbpf.initField(cfg, 40); fieldMap["PrmpVbpf"] = &PrmpVbpf;
    PrmpVbnLCC.initField(cfg, 0); fieldMap["PrmpVbnLCC"] = &PrmpVbnLCC;
    // 13
    S1.initField(cfg, 0); fieldMap["S1"] = &S1;
    S0.initField(cfg, 0); fieldMap["S0"] = &S0;
    Pixel_latch_strobe.initField(cfg, 0); fieldMap["Pixel_latch_strobe"] = &Pixel_latch_strobe;
    // 14
    LVDSDrvIref.initField(cfg, 171); fieldMap["LVDSDrvIref"] = &LVDSDrvIref;
    GADCCompBias.initField(cfg, 100); fieldMap["GADCCompBias"] = &GADCCompBias;
    // 15
    PllIbias.initField(cfg, 88); fieldMap["PllIbias"] = &PllIbias;
    LVDSDrvVos.initField(cfg, 105); fieldMap["LVDSDrvVos"] = &LVDSDrvVos;
    // 16
    TempSensIbias.initField(cfg, 0); fieldMap["TempSensIbias"] = &TempSensIbias;
    PllIcp.initField(cfg, 28); fieldMap["PllIcp"] = &PllIcp;
    // 17
    PlsrIDACRamp.initField(cfg, 213); fieldMap["PlsrIDACRamp"] = &PlsrIDACRamp;
    // 18
    VrefDigTune.initField(cfg, 110); fieldMap["VrefDigTune"] = &VrefDigTune;
    PlsrVgOpAmp.initField(cfg, 255); fieldMap["PlsrVgOpAmp"] = &PlsrVgOpAmp;
    // 19
    PlsrDACbias.initField(cfg, 96); fieldMap["PlsrDACbias"] = &PlsrDACbias;
    VrefAnTune.initField(cfg, 50); fieldMap["VrefAnTune"] = &VrefAnTune;
    // 20
    Vthin_Coarse.initField(cfg, 0); fieldMap["Vthin_Coarse"] = &Vthin_Coarse;
    Vthin_Fine.initField(cfg, 150); fieldMap["Vthin_Fine"] = &Vthin_Fine;
    // 21
    HitLD.initField(cfg, 0); fieldMap["HitLD"] = &HitLD;
    DJO.initField(cfg, 0); fieldMap["DJO"] = &DJO;
    DigHitIn_Sel.initField(cfg, 0); fieldMap["DigHitIn_Sel"] = &DigHitIn_Sel;
    PlsrDAC.initField(cfg, 54); fieldMap["PlsrDAC"] = &PlsrDAC;
    // 22
    Colpr_Mode.initField(cfg, 0); fieldMap["Colpr_Mode"] = &Colpr_Mode;
    Colpr_Addr.initField(cfg, 0); fieldMap["Colpr_Addr"] = &Colpr_Addr;
    // 23
    DisableColCnfg0.initField(cfg, 0); fieldMap["DisableColCnfg0"] = &DisableColCnfg0;
    // 24
    DisableColCnfg1.initField(cfg, 0); fieldMap["DisableColCnfg1"] = &DisableColCnfg1;
    // 25
    DisableColCnfg2.initField(cfg, 0); fieldMap["DisableColCnfg2"] = &DisableColCnfg2;
    Trig_Lat.initField(cfg, 210); fieldMap["Trig_Lat"] = &Trig_Lat;
    // 26
    CalPulseWidth.initField(cfg, 10); fieldMap["CalPulseWidth"] = &CalPulseWidth;
    CalPulseDelay.initField(cfg, 0); fieldMap["CalPulseDelay"] = &CalPulseDelay;
    StopModeConfig.initField(cfg, 0); fieldMap["StopModeConfig"] = &StopModeConfig;
    HitDiscCnfg.initField(cfg, 0); fieldMap["HitDiscCnfg"] = &HitDiscCnfg;
    // 27
    PLL_Enable.initField(cfg, 1); fieldMap["PLL_Enable"] = &PLL_Enable;
    EFS.initField(cfg, 0); fieldMap["EFS"] = &EFS;
    StopClkPulse.initField(cfg, 0); fieldMap["StopClkPulse"] = &StopClkPulse;
    ReadErrorReq.initField(cfg, 0); fieldMap["ReadErrorReq"] = &ReadErrorReq;
    GADC_En.initField(cfg, 0); fieldMap["GADC_En"] = &GADC_En;
    SRRead.initField(cfg, 0); fieldMap["SRRead"] = &SRRead;
    HitOr.initField(cfg, 0); fieldMap["HitOr"] = &HitOr;
    CalEn.initField(cfg, 0); fieldMap["CalEn"] = &CalEn;
    SRClr.initField(cfg, 0); fieldMap["SRClr"] = &SRClr;
    Latch_Enable.initField(cfg, 0); fieldMap["Latch_Enable"] = &Latch_Enable;
    SR_Clock.initField(cfg, 0); fieldMap["SR_Clock"] = &SR_Clock;
    // 28
    LVDSDrvSet06.initField(cfg, 1); fieldMap["LVDSDrvSet06"] = &LVDSDrvSet06;
    EN_40M.initField(cfg, 1); fieldMap["EN_40M"] = &EN_40M;
    EN_80M.initField(cfg, 0); fieldMap["EN_80M"] = &EN_80M;
    CLK1_S0.initField(cfg, 0); fieldMap["CLK1_S0"] = &CLK1_S0;
    CLK1_S1.initField(cfg, 0); fieldMap["CLK1_S1"] = &CLK1_S1;
    CLK1_S2.initField(cfg, 0); fieldMap["CLK1_S2"] = &CLK1_S2;
    CLK0_S0.initField(cfg, 0); fieldMap["CLK0_S0"] = &CLK0_S0;
    CLK0_S1.initField(cfg, 0); fieldMap["CLK0_S1"] = &CLK0_S1;
    CLK0_S2.initField(cfg, 1); fieldMap["CLK0_S2"] = &CLK0_S2;
    EN_160.initField(cfg, 1); fieldMap["EN_160"] = &EN_160;
    EN_320.initField(cfg, 0); fieldMap["EN_320"] = &EN_320;
    // 29
    No8b10b.initField(cfg, 0); fieldMap["No8b10b"] = &No8b10b;
    Clk2Out.initField(cfg, 0); fieldMap["Clk2Out"] = &Clk2Out;
    EmptyRecordCnfg.initField(cfg, 0); fieldMap["EmptyRecordCnfg"] = &EmptyRecordCnfg;
    LVDSDrvEn.initField(cfg, 1); fieldMap["LVDSDrvEn"] = &LVDSDrvEn;
    LVDSDrvSet30.initField(cfg, 1); fieldMap["LVDSDrvSet30"] = &LVDSDrvSet30;
    LVDSDrvSet12.initField(cfg, 1); fieldMap["LVDSDrvSet12"] = &LVDSDrvSet12;
    // 30
    TmpSensDiodeSel.initField(cfg, 0); fieldMap["TmpSensDiodeSel"] = &TmpSensDiodeSel;
    TmpSensDisable.initField(cfg, 0); fieldMap["TmpSensDisable"] = &TmpSensDisable;
    IleakRange.initField(cfg, 0); fieldMap["IleakRange"] = &IleakRange;
    // 31
    PlsrRiseUpTau.initField(cfg, 7); fieldMap["PlsrRiseUpTau"] = &PlsrRiseUpTau;
    PlsrPwr.initField(cfg, 1); fieldMap["PlsrPwr"] = &PlsrPwr;
    PlsrDelay.initField(cfg, 2); fieldMap["PlsrDelay"] = &PlsrDelay;
    ExtDigCalSW.initField(cfg, 0); fieldMap["ExtDigCalSW"] = &ExtDigCalSW;
    ExtAnaCalSW.initField(cfg, 0); fieldMap["ExtAnaCalSW"] = &ExtAnaCalSW;
    GADCSel.initField(cfg, 0); fieldMap["GADCSel"] = &GADCSel;
    // 32
    SELB0.initField(cfg, 0); fieldMap["SELB0"] = &SELB0;
    SELB1.initField(cfg, 0); fieldMap["SELB1"] = &SELB1;
    SELB2.initField(cfg, 0); fieldMap["SELB2"] = &SELB2;
    PrmpVbpMsbEn.initField(cfg, 0); fieldMap["PrmpVbpMsbEn"] = &PrmpVbpMsbEn;
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
    typedef std::map<std::string, FieldOperator<uint16_t>*>::iterator it_type;
    for(it_type iterator = fieldMap.begin(); iterator != fieldMap.end(); iterator++) {
        reg = doc->NewElement(iterator->first.c_str());
        reg->SetAttribute("value", iterator->second->value());
        reg->SetAttribute("type", "dec");
        gcfg->LinkEndChild(reg);
    }

    node->LinkEndChild(gcfg);
}

void Fei4GlobalCfg::toFileJson(json &j) {
    typedef std::map<std::string, FieldOperator<uint16_t>*>::iterator it_type;
    for(it_type iterator = fieldMap.begin(); iterator != fieldMap.end(); iterator++) {
         j["FE-I4B"]["GlobalConfig"][iterator->first] = iterator->second->value();
    }

}

void Fei4GlobalCfg::fromFileJson(json &j) {
    typedef std::map<std::string, FieldOperator<uint16_t>*>::iterator it_type;
    for(it_type iterator = fieldMap.begin(); iterator != fieldMap.end(); iterator++) {
         iterator->second->write((uint16_t) j["FE-I4B"]["GlobalConfig"][iterator->first]);
    }

}
