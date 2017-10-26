// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: RD53A library
// # Comment: RD53A global register
// # Date: August 2017
// ################################

#include "Rd53aGlobalCfg.h"

Rd53aGlobalCfg::Rd53aGlobalCfg() {
    this->init();
}

Rd53aGlobalCfg::~Rd53aGlobalCfg() {
    
}

void Rd53aGlobalCfg::init() {
    for (unsigned int i=0; i<numRegs; i++)
        m_cfg[i] = 0x00;

    //0
    PixPortalHigh.init(0, &m_cfg[0], 8, 8, 0x0); regMap["PixPortalHigh"] = &PixPortalHigh; // TODO rename odd/even
    PixPortalLow.init(0, &m_cfg[0], 0, 8, 0x0);
    //1
    RegionCol.init(1, &m_cfg[1], 0, 8, 0x0);
    //2
    RegionRow.init(2, &m_cfg[2], 0, 9, 0x0);
    //3
    PixMode.init(3, &m_cfg[3], 4, 3, 0x0); // TODO need table in doc
    BMask.init(3, &m_cfg[3], 0, 3, 0x0);
    //4
    PixDefaultConfig.init(4, &m_cfg[4], 0, 16, 0x9CE2); // TODO why not the same
    //5
    Ibiasp1Sync.init(5, &m_cfg[5], 0, 9, 100);
    //6
    Ibiasp2Sync.init(99, &m_cfg[6], 0, 9, 150);
    //7
    IbiasSfSync.init(99, &m_cfg[7], 0, 9, 100);
    //8
    IbiasKrumSync.init(99, &m_cfg[8], 0, 9, 140);
    //9
    IbiasDiscSync.init(99, &m_cfg[9], 0, 9, 200);
    //10
    IctrlSynctSync.init(99, &m_cfg[10], 0, 10, 100);
    //11
    VblSync.init(99, &m_cfg[11], 0, 10, 450);
    //12
    VthSync.init(99, &m_cfg[12], 0, 10, 300);
    //13
    VrefKrumSync.init(99, &m_cfg[13], 0, 10, 490);
    //14
    PaInBiasLin.init(99, &m_cfg[14], 0, 9, 300);
    //15
    FcBiasLin.init(99, &m_cfg[15], 0, 8, 20);
    //16
    KrumCurrLin.init(99, &m_cfg[16], 0, 9, 50);
    //17
    LdacLin.init(99, &m_cfg[17], 0, 10, 80);
    //18
    CompLin.init(99, &m_cfg[18], 0, 9, 110);
    //19
    RefKrumLin.init(99, &m_cfg[19], 0, 10, 300);
    //20
    VthresholdLin.init(99, &m_cfg[20], 0, 10, 408);
    //21
    PrmpDiff.init(99, &m_cfg[21], 0, 10, 533);
    //22
    FolDiff.init(99, &m_cfg[22], 0, 10, 542);
    //23
    PrecompDiff.init(99, &m_cfg[23], 0, 10, 551);
    //24
    CompDiff.init(99, &m_cfg[24], 0, 10, 528);
    //25
    VffDiff.init(99, &m_cfg[25], 0, 10, 164);
    //26
    Vth1Diff.init(99, &m_cfg[26], 0, 10, 1023);
    //27
    Vth2Diff.init(99, &m_cfg[27], 0, 10, 0);
    //28
    LccDiff.init(99, &m_cfg[28], 0, 10, 20);
    //29
    ConfFeDiff.init(99, &m_cfg[29], 0, 2, 2); // not sure about value
    //31
    SldoAnalogTrim.init(99, &m_cfg[31], 5, 5, 16);
    SldoDigitalTrim.init(99, &m_cfg[31], 0, 5, 16);
    //32
    EnCoreColSync.init(32, &m_cfg[32], 0, 16, 0xFFFF);
    //33
    EnCoreColLin1.init(33, &m_cfg[33], 0, 16, 0xFFFF);
    //34
    EnCoreColLin2.init(34, &m_cfg[34], 0, 1, 1);
    //35
    EnCoreColDiff1.init(35, &m_cfg[35], 0, 16, 0xFFFF);
    //36
    EnCoreColDiff2.init(36, &m_cfg[36], 0, 1, 1);
    //37
    LatencyConfig.init(99, &m_cfg[37], 0, 9, 500);
    //38
    WrSyncDelaySync.init(99, &m_cfg[38], 0, 5, 16);
    //39
    InjModeDel.init(99, &m_cfg[39], 0, 6, 32); // not sure about value
    //40
    ClkDataDelay.init(99, &m_cfg[40], 0, 9, 0);
    //41
    VcalHigh.init(99, &m_cfg[41], 0, 11, 500);
    //42
    VcalMed.init(99, &m_cfg[42], 0, 11, 300);
    //43
    ChSyncConf.init(99, &m_cfg[43], 0, 10, 0); // not sure about value
    //44
    GlobalPulseRt.init(99, &m_cfg[44], 0, 16, 0);
    //46
    CalColprSync1.init(46, &m_cfg[46], 0, 16, 0xFFFF);
    //47
    CalColprSync2.init(47, &m_cfg[47], 0, 16, 0xFFFF);
    //48
    CalColprSync3.init(48, &m_cfg[48], 0, 16, 0xFFFF);
    //49
    CalColprSync4.init(49, &m_cfg[49], 0, 16, 0xFFFF);
    //50
    CalColprLin1.init(50, &m_cfg[50], 0, 16, 0xFFFF);
    //51
    CalColprLin2.init(51, &m_cfg[51], 0, 16, 0xFFFF);
    //52
    CalColprLin3.init(52, &m_cfg[52], 0, 16, 0xFFFF);
    //53
    CalColprLin4.init(53, &m_cfg[53], 0, 16, 0xFFFF);
    //54
    CalColprLin5.init(54, &m_cfg[54], 0, 4, 0xF);
    //55
    CalColprDiff1.init(55, &m_cfg[55], 0, 16, 0xFFFF);
    //56
    CalColprDiff2.init(56, &m_cfg[56], 0, 16, 0xFFFF);
    //57
    CalColprDiff3.init(57, &m_cfg[57], 0, 16, 0xFFFF);
    //58
    CalColprDiff4.init(58, &m_cfg[58], 0, 16, 0xFFFF);
    //59
    CalColprDiff5.init(59, &m_cfg[59], 0, 4, 0xF);
    //60
    DebugConfig.init(99, &m_cfg[60], 0, 16, 0);
}
