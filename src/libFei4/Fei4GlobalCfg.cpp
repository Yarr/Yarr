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
    SME.initField(cfg, 0);
    EventLimit.initField(cfg, 0);
    Trig_Count.initField(cfg, 1);
    Conf_AddrEnable.initField(cfg, 1);
    ErrorMask_0.initField(cfg, 0x4600);
    ErrorMask_1.initField(cfg, 0x0040);
    PrmpVbp_R.initField(cfg, 43);
    BufVgOpAmp.initField(cfg, 160);
    PrmpVbp.initField(cfg, 43);
    TDACVbp.initField(cfg, 150);
    DisVbn.initField(cfg, 40);
    Amp2Vbn.initField(cfg, 79);
    Amp2VbpFol.initField(cfg, 26);
    Amp2Vbp.initField(cfg, 85);
    FDACVbn.initField(cfg, 30);
    Amp2Vbpff.initField(cfg, 50);
    PrmpVbnFol.initField(cfg, 106);
    PrmpVbnLCC.initField(cfg, 0);
    S1.initField(cfg, 0);
    S0.initField(cfg, 0);
    Pixel_latch_strobe.initField(cfg, 0);
    LVDSDrvIref.initField(cfg, 171);
    GADCCompBias.initField(cfg, 100);
    PllIbias.initField(cfg, 88);
    LVDSDrvVos.initField(cfg, 105);
    TempSensIbias.initField(cfg, 0);
    PllIcp.initField(cfg, 28);
    PlsrIDACRamp.initField(cfg, 213);
    VrefDigTune.initField(cfg, 110);
    PlsrVgOpAmp.initField(cfg, 255);
    PlsrDACbias.initField(cfg, 96);
    VrefAnTune.initField(cfg, 0);
    Vthin_Coarse.initField(cfg, 0);
    Vthin_Fine.initField(cfg, 220);
    HitLD.initField(cfg, 0);
    DJO.initField(cfg, 0);
    DigHitIn_Sel.initField(cfg, 0);
    PlsrDAC.initField(cfg, 54);
    Colpr_Mode.initField(cfg, 0);
    Colpr_Addr.initField(cfg, 0);
    DisableColCnfg0.initField(cfg, 0);
    DisableColCnfg1.initField(cfg, 0);
    DisableColCnfg2.initField(cfg, 0);
    Trig_Lat.initField(cfg, 210);
    CalPulseWidth.initField(cfg, 11);
    CalPulseDelay.initField(cfg, 0);
    StopModeConfig.initField(cfg, 0);
    HitDiscCnfg.initField(cfg, 0);
    PLL_Enable.initField(cfg, 1);
    EFS.initField(cfg, 0);
    StopClkPulse.initField(cfg, 0);
    ReadErrorReq.initField(cfg, 0);
    GADC_En.initField(cfg, 0);
    SRRead.initField(cfg, 0);
    HitOr.initField(cfg, 0);
    CalEn.initField(cfg, 0);
    SRClr.initField(cfg, 0);
    Latch_Enable.initField(cfg, 0);
    SR_Clock.initField(cfg, 0);
    LVDSDrvSet06.initField(cfg, 1);
    EN_40M.initField(cfg, 1);
    EN_80M.initField(cfg, 0);
    CLK1_S0.initField(cfg, 0);
    CLK1_S1.initField(cfg, 0);
    CLK1_S2.initField(cfg, 0);
    CLK0_S0.initField(cfg, 0);
    CLK0_S1.initField(cfg, 0);
    CLK0_S2.initField(cfg, 1);
    EN_160.initField(cfg, 1);
    EN_320.initField(cfg, 0);
    No8b10b.initField(cfg, 0);
    Clk2Out.initField(cfg, 0);
    EmptyRecordCnfg.initField(cfg, 0);
    LVDSDrvEn.initField(cfg, 1);
    LVDSDrvSet30.initField(cfg, 1);
    LVDSDrvSet12.initField(cfg, 1);
    TmpSensDiodeSel.initField(cfg, 0);
    TmpSensDisable.initField(cfg, 0);
    IleakRange.initField(cfg, 0);
    PlsrRiseUpTau.initField(cfg, 7);
    PlsrPwr.initField(cfg, 1);
    PlsrDelay.initField(cfg, 2);
    ExtDigCalSW.initField(cfg, 0);
    ExtAnaCalSW.initField(cfg, 0);
    GADCSel.initField(cfg, 0);
    SELB0.initField(cfg, 0);
    SELB1.initField(cfg, 0);
    SELB2.initField(cfg, 0);
    PrmpVbpMsbEn.initField(cfg, 0);
}


void Fei4GlobalCfg::toFile(std::string filename) {
    std::fstream file(filename, std::fstream::out | std::fstream::trunc);
    
    file << "# FEI4B-global-config" << std::endl;
    file << "# ###################" << std::endl;
    
    file << "SME              " << SME.value() << std::endl;
    file << "EventLimit       " << EventLimit.value() << std::endl;
    file << "Trig_Count       " << Trig_Count.value() << std::endl;
    file << "Conf_AddrEnable  " << Conf_AddrEnable.value() << std::endl;
    file << "ErrorMask_0      " << ErrorMask_0.value() << std::endl;
    file << "ErrorMadk_1      " << ErrorMask_1.value() << std::endl;
    file << "PrmpVbp_R        " << PrmpVbp_R.value() << std::endl;
    
    file.close();

}
