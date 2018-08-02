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
    PixPortal.init(0, &m_cfg[0], 0, 16, 0x0); regMap["PixPortal"] = &Rd53aGlobalCfg::PixPortal;
    //1
    PixRegionCol.init(1, &m_cfg[1], 0, 8, 0x0); regMap["PixRegionCol"] = &Rd53aGlobalCfg::PixRegionCol;
    //2
    PixRegionRow.init(2, &m_cfg[2], 0, 9, 0x0); regMap["PixRegionRow"] = &Rd53aGlobalCfg::PixRegionRow;
    //3
    PixBroadcastEn.init(3, &m_cfg[3], 5, 1, 0x0); regMap["PixBroadcastEn"] = &Rd53aGlobalCfg::PixBroadcastEn;
    PixAutoCol.init(3, &m_cfg[3], 4, 1, 0x0); regMap["PixAutoCol"] = &Rd53aGlobalCfg::PixAutoCol;
    PixAutoRow.init(3, &m_cfg[3], 3, 1, 0x0); regMap["PixAutoRow"] = &Rd53aGlobalCfg::PixAutoRow;
    PixBroadcastMask.init(3, &m_cfg[3], 0, 3, 0x0); regMap["PixBroadcastMask"] = &Rd53aGlobalCfg::PixBroadcastMask;
    //4
    PixDefaultConfig.init(4, &m_cfg[4], 0, 16, 0x0);  regMap["PixDefaultConfig"] = &Rd53aGlobalCfg::PixDefaultConfig;

    // Sync FE
    //5
    SyncIbiasp1.init(5, &m_cfg[5], 0, 9, 80); regMap["SyncIbiasp1"] = &Rd53aGlobalCfg::SyncIbiasp1;
    //6
    SyncIbiasp2.init(6, &m_cfg[6], 0, 9, 120); regMap["SyncIbiasp2"] = &Rd53aGlobalCfg::SyncIbiasp2;
    //7
    SyncIbiasSf.init(7, &m_cfg[7], 0, 9, 80); regMap["SyncIbiasSf"] = &Rd53aGlobalCfg::SyncIbiasSf;
    //8
    SyncIbiasKrum.init(8, &m_cfg[8], 0, 9, 55); regMap["SyncIbiasKrum"] = &Rd53aGlobalCfg::SyncIbiasKrum;
    //9
    SyncIbiasDisc.init(9, &m_cfg[9], 0, 9, 300); regMap["SyncIbiasDisc"] = &Rd53aGlobalCfg::SyncIbiasDisc;
    //10
    SyncIctrlSynct.init(10, &m_cfg[10], 0, 10, 100); regMap["SyncIctrlSynct"] = &Rd53aGlobalCfg::SyncIctrlSynct;
    //11
    SyncVbl.init(11, &m_cfg[11], 0, 10, 400); regMap["SyncVbl"] = &Rd53aGlobalCfg::SyncVbl;
    //12
    SyncVth.init(12, &m_cfg[12], 0, 10, 300); regMap["SyncVth"] = &Rd53aGlobalCfg::SyncVth;
    //13
    SyncVrefKrum.init(13, &m_cfg[13], 0, 10, 450); regMap["SyncVrefKrum"] = &Rd53aGlobalCfg::SyncVrefKrum;
    //30
    SyncAutoZero.init(30, &m_cfg[30], 3, 2, 0); regMap["SyncAutoZero"] = &Rd53aGlobalCfg::SyncAutoZero;
    SyncSelC2F.init(30, &m_cfg[30], 2, 1, 0); regMap["SyncSelC2F"] = &Rd53aGlobalCfg::SyncSelC2F;
    SyncSelC4F.init(30, &m_cfg[30], 1, 1, 1); regMap["SyncSelC4F"] = &Rd53aGlobalCfg::SyncSelC4F;
    SyncFastTot.init(30, &m_cfg[30], 0, 1, 0); regMap["SyncFastTot"] = &Rd53aGlobalCfg::SyncFastTot;

    // Linear FE
    //14
    LinPaInBias.init(14, &m_cfg[14], 0, 9, 300); regMap["LinPaInBias"] = &Rd53aGlobalCfg::LinPaInBias;
    //15
    LinFcBias.init(15, &m_cfg[15], 0, 8, 20); regMap["LinFcBias"] = &Rd53aGlobalCfg::LinFcBias;
    //16
    LinKrumCurr.init(16, &m_cfg[16], 0, 9, 50); regMap["LinKrumCurr"] = &Rd53aGlobalCfg::LinKrumCurr;
    //17
    LinLdac.init(17, &m_cfg[17], 0, 10, 100); regMap["LinLdac"] = &Rd53aGlobalCfg::LinLdac;
    //18
    LinComp.init(18, &m_cfg[18], 0, 9, 110); regMap["LinComp"] = &Rd53aGlobalCfg::LinComp;
    //19
    LinRefKrum.init(19, &m_cfg[19], 0, 10, 300); regMap["LinRefKrum"] = &Rd53aGlobalCfg::LinRefKrum;
    //20
    LinVth.init(20, &m_cfg[20], 0, 10, 400); regMap["LinVth"] = &Rd53aGlobalCfg::LinVth;

    // Diff FE
    //21
    DiffPrmp.init(21, &m_cfg[21], 0, 10, 500); regMap["DiffPrmp"] = &Rd53aGlobalCfg::DiffPrmp;
    //22
    DiffFol.init(22, &m_cfg[22], 0, 10, 500); regMap["DiffFol"] = &Rd53aGlobalCfg::DiffFol;
    //23
    DiffPrecomp.init(23, &m_cfg[23], 0, 10, 400); regMap["DiffPrecomp"] = &Rd53aGlobalCfg::DiffPrecomp;
    //24
    DiffComp.init(24, &m_cfg[24], 0, 10, 1000); regMap["DiffComp"] = &Rd53aGlobalCfg::DiffComp;
    //25
    DiffVff.init(25, &m_cfg[25], 0, 10, 50); regMap["DiffVff"] = &Rd53aGlobalCfg::DiffVff;
    //26
    DiffVth1.init(26, &m_cfg[26], 0, 10, 250); regMap["DiffVth1"] = &Rd53aGlobalCfg::DiffVth1;
    //27
    DiffVth2.init(27, &m_cfg[27], 0, 10, 50); regMap["DiffVth2"] = &Rd53aGlobalCfg::DiffVth2;
    //28
    DiffLcc.init(28, &m_cfg[28], 0, 10, 20); regMap["DiffLcc"] = &Rd53aGlobalCfg::DiffLcc;
    //29
    DiffLccEn.init(29, &m_cfg[29], 1, 1, 0); regMap["DiffLccEn"] = &Rd53aGlobalCfg::DiffLccEn;
    DiffFbCapEn.init(29, &m_cfg[29], 0, 1, 0); regMap["DiffFbCapEn"] = &Rd53aGlobalCfg::DiffFbCapEn;

    //Power
    //31
    SldoAnalogTrim.init(31, &m_cfg[31], 5, 5, 26); regMap["SldoAnalogTrim"] = &Rd53aGlobalCfg::SldoAnalogTrim;
    SldoDigitalTrim.init(31, &m_cfg[31], 0, 5, 26); regMap["SldoDigitalTrim"] = &Rd53aGlobalCfg::SldoDigitalTrim;

    // Digital Matrix
    //32
    EnCoreColSync.init(32, &m_cfg[32], 0, 16, 0xFFFF); regMap["EnCoreColSync"] = &Rd53aGlobalCfg::EnCoreColSync;
    //33
    EnCoreColLin1.init(33, &m_cfg[33], 0, 16, 0xFFFF); regMap["EnCoreColLin1"] = &Rd53aGlobalCfg::EnCoreColLin1;
    //34
    EnCoreColLin2.init(34, &m_cfg[34], 0, 1, 1); regMap["EnCoreColLin2"] = &Rd53aGlobalCfg::EnCoreColLin2;
    //35
    EnCoreColDiff1.init(35, &m_cfg[35], 0, 16, 0xFFFF); regMap["EnCoreColDiff1"] = &Rd53aGlobalCfg::EnCoreColDiff1;
    //36
    EnCoreColDiff2.init(36, &m_cfg[36], 0, 1, 1); regMap["EnCoreColDiff2"] = &Rd53aGlobalCfg::EnCoreColDiff2;
    //37
    LatencyConfig.init(37, &m_cfg[37], 0, 9, 64); regMap["LatencyConfig"] = &Rd53aGlobalCfg::LatencyConfig;
    //38
    WrSyncDelaySync.init(38, &m_cfg[38], 0, 5, 16); regMap["WrSyncDelaySync"] = &Rd53aGlobalCfg::WrSyncDelaySync;

    // Injection
    //39
    InjAnaMode.init(39, &m_cfg[39], 5, 1, 0); regMap["InjAnaMode"] = &Rd53aGlobalCfg::InjAnaMode;
    InjEnDig.init(39, &m_cfg[39], 4, 1, 0); regMap["InjEnDig"] = &Rd53aGlobalCfg::InjEnDig;
    InjDelay.init(39, &m_cfg[39], 0, 4, 0); regMap["InjDelay"] = &Rd53aGlobalCfg::InjDelay;
    //41
    InjVcalHigh.init(41, &m_cfg[41], 0, 12, 1000); regMap["InjVcalHigh"] = &Rd53aGlobalCfg::InjVcalHigh;
    //42
    InjVcalMed.init(42, &m_cfg[42], 0, 12, 1000); regMap["InjVcalMed"] = &Rd53aGlobalCfg::InjVcalMed;
    //46
    CalColprSync1.init(46, &m_cfg[46], 0, 16, 0xFFFF); regMap["CalColprSync1"] = &Rd53aGlobalCfg::CalColprSync1;
    //47
    CalColprSync2.init(47, &m_cfg[47], 0, 16, 0xFFFF); regMap["CalColprSync2"] = &Rd53aGlobalCfg::CalColprSync2;
    //48
    CalColprSync3.init(48, &m_cfg[48], 0, 16, 0xFFFF); regMap["CalColprSync3"] = &Rd53aGlobalCfg::CalColprSync3;
    //49
    CalColprSync4.init(49, &m_cfg[49], 0, 16, 0xFFFF); regMap["CalColprSync4"] = &Rd53aGlobalCfg::CalColprSync4;
    //50
    CalColprLin1.init(50, &m_cfg[50], 0, 16, 0xFFFF); regMap["CalColprLin1"] = &Rd53aGlobalCfg::CalColprLin1;
    //51
    CalColprLin2.init(51, &m_cfg[51], 0, 16, 0xFFFF); regMap["CalColprLin2"] = &Rd53aGlobalCfg::CalColprLin2;
    //52
    CalColprLin3.init(52, &m_cfg[52], 0, 16, 0xFFFF); regMap["CalColprLin3"] = &Rd53aGlobalCfg::CalColprLin3;
    //53
    CalColprLin4.init(53, &m_cfg[53], 0, 16, 0xFFFF); regMap["CalColprLin4"] = &Rd53aGlobalCfg::CalColprLin4;
    //54
    CalColprLin5.init(54, &m_cfg[54], 0, 4, 0xF); regMap["CalColprLin5"] = &Rd53aGlobalCfg::CalColprLin5;
    //55
    CalColprDiff1.init(55, &m_cfg[55], 0, 16, 0xFFFF); regMap["CalColprDiff1"] = &Rd53aGlobalCfg::CalColprDiff1;
    //56
    CalColprDiff2.init(56, &m_cfg[56], 0, 16, 0xFFFF); regMap["CalColprDiff2"] = &Rd53aGlobalCfg::CalColprDiff2;
    //57
    CalColprDiff3.init(57, &m_cfg[57], 0, 16, 0xFFFF); regMap["CalColprDiff3"] = &Rd53aGlobalCfg::CalColprDiff3;
    //58
    CalColprDiff4.init(58, &m_cfg[58], 0, 16, 0xFFFF); regMap["CalColprDiff4"] = &Rd53aGlobalCfg::CalColprDiff4;
    //59
    CalColprDiff5.init(59, &m_cfg[59], 0, 4, 0xF); regMap["CalColprDiff5"] = &Rd53aGlobalCfg::CalColprDiff5;

    // Digital Functions
    //40
    ClkDelaySel.init(40, &m_cfg[40], 8, 1, 0); regMap["ClkDelaySel"] = &Rd53aGlobalCfg::ClkDelaySel;
    ClkDelay.init(40, &m_cfg[40], 4, 4, 0); regMap["ClkDelay"] = &Rd53aGlobalCfg::ClkDelay;
    CmdDelay.init(40, &m_cfg[40], 0, 4, 0); regMap["CmdDelay"] = &Rd53aGlobalCfg::CmdDelay;
    //43
    ChSyncPhase.init(43, &m_cfg[43], 10, 2, 0); regMap["ChSyncPhase"] = &Rd53aGlobalCfg::ChSyncPhase;
    ChSyncLock.init(43, &m_cfg[43], 5, 5, 16); regMap["ChSyncLock"] = &Rd53aGlobalCfg::ChSyncLock;
    ChSyncUnlock.init(43, &m_cfg[43], 0, 5, 8); regMap["ChSyncUnlock"] = &Rd53aGlobalCfg::ChSyncUnlock;
    //44
    GlobalPulseRt.init(44, &m_cfg[44], 0, 16, 0); regMap["GlobalPulseRt"] = &Rd53aGlobalCfg::GlobalPulseRt;

    // I/O
    //60
    DebugConfig.init(60, &m_cfg[60], 0, 2, 0); regMap["DebugConfig"] = &Rd53aGlobalCfg::DebugConfig;
    //61
    OutputDataReadDelay.init(61, &m_cfg[61], 7, 2, 0); regMap["OutputDataReadDelay"] = &Rd53aGlobalCfg::OutputDataReadDelay;
    OutputSerType.init(61, &m_cfg[61], 6, 1, 0); regMap["OutputSerType"] = &Rd53aGlobalCfg::OutputSerType;
    OutputActiveLanes.init(61, &m_cfg[61], 2, 4, 0xF); regMap["OutputActiveLanes"] = &Rd53aGlobalCfg::OutputActiveLanes;
    OutputFmt.init(61, &m_cfg[61], 0, 2, 0); regMap["OutputFmt"] = &Rd53aGlobalCfg::OutputFmt;
    //62
    OutPadConfig.init(62, &m_cfg[62], 0, 13, 0x1404); regMap["OutPadConfig"] = &Rd53aGlobalCfg::OutPadConfig;
    //63
    GpLvdsRoute.init(63, &m_cfg[63], 0, 3, 0); regMap["GpLvdsRoute"] = &Rd53aGlobalCfg::GpLvdsRoute;
    //64
    CdrSelDelClk.init(64, &m_cfg[64], 13, 1, 0); regMap["CdrSelDelClk"] = &Rd53aGlobalCfg::CdrSelDelClk;
    CdrPdSel.init(64, &m_cfg[64], 11, 2, 0); regMap["CdrPdSel"] = &Rd53aGlobalCfg::CdrPdSel;
    CdrPdDel.init(64, &m_cfg[64], 7, 4, 8); regMap["CdrPdDel"] = &Rd53aGlobalCfg::CdrPdDel;
    CdrEnGck.init(64, &m_cfg[64], 6, 1, 0); regMap["CdrEnGck"] = &Rd53aGlobalCfg::CdrEnGck;
    CdrVcoGain.init(64, &m_cfg[64], 3, 3, 3); regMap["CdrVcoGain"] = &Rd53aGlobalCfg::CdrVcoGain;
    CdrSelSerClk.init(64, &m_cfg[64], 0, 3, 3); regMap["CdrSelSerClk"] = &Rd53aGlobalCfg::CdrSelSerClk;
    //65
    VcoBuffBias.init(65, &m_cfg[65], 0, 10, 400); regMap["VcoBuffBias"] = &Rd53aGlobalCfg::VcoBuffBias;
    //66
    CdrCpIbias.init(66, &m_cfg[66], 0, 10, 50); regMap["CdrCpIbias"] = &Rd53aGlobalCfg::CdrCpIbias;
    //67
    VcoIbias.init(67, &m_cfg[67], 0, 10, 500); regMap["VcoIbias"] = &Rd53aGlobalCfg::VcoIbias;
    //68
    SerSelOut0.init(68, &m_cfg[68], 0, 2, 1); regMap["SerSelOut0"] = &Rd53aGlobalCfg::SerSelOut0;
    SerSelOut1.init(68, &m_cfg[68], 2, 2, 1); regMap["SerSelOut1"] = &Rd53aGlobalCfg::SerSelOut1;
    SerSelOut2.init(68, &m_cfg[68], 4, 2, 1); regMap["SerSelOut2"] = &Rd53aGlobalCfg::SerSelOut2;
    SerSelOut3.init(68, &m_cfg[68], 6, 2, 1); regMap["SerSelOut3"] = &Rd53aGlobalCfg::SerSelOut3;
    //69
    CmlInvTap.init(69, &m_cfg[69], 6, 2, 0x0); regMap["CmlInvTap"] = &Rd53aGlobalCfg::CmlInvTap;
    CmlEnTap.init(69, &m_cfg[69], 4, 2, 0x1); regMap["CmlEnTap"] = &Rd53aGlobalCfg::CmlEnTap;
    CmlEn.init(69, &m_cfg[69], 0, 4, 0xF); regMap["CmlEn"] = &Rd53aGlobalCfg::CmlEn;
    //70-72
    CmlTapBias0.init(70, &m_cfg[70], 0, 10, 600); regMap["CmlTapBias0"] = &Rd53aGlobalCfg::CmlTapBias0;
    CmlTapBias1.init(71, &m_cfg[71], 0, 10, 0); regMap["CmlTapBias1"] = &Rd53aGlobalCfg::CmlTapBias1;
    CmlTapBias2.init(72, &m_cfg[72], 0, 10, 0); regMap["CmlTapBias2"] = &Rd53aGlobalCfg::CmlTapBias2;
    //73
    AuroraCcWait.init(73, &m_cfg[73], 2, 6, 25); regMap["AuroraCcWait"] = &Rd53aGlobalCfg::AuroraCcWait;
    AuroraCcSend.init(73, &m_cfg[73], 0, 2, 3); regMap["AuroraCcSend"] = &Rd53aGlobalCfg::AuroraCcSend;
    //74
    AuroraCbWaitLow.init(74, &m_cfg[74], 4, 4, 15); regMap["AuroraCbWaitLow"] = &Rd53aGlobalCfg::AuroraCbWaitLow;
    AuroraCbSend.init(74, &m_cfg[74], 0, 4, 0); regMap["AuroraCbSend"] = &Rd53aGlobalCfg::AuroraCbSend;
    //75
    AuroraCbWaitHigh.init(75, &m_cfg[75], 0, 16, 15); regMap["AuroraCbWaitHigh"] = &Rd53aGlobalCfg::AuroraCbWaitHigh;
    //76
    AuroraInitWait.init(76, &m_cfg[76], 0, 11, 32); regMap["AuroraInitWait"] = &Rd53aGlobalCfg::AuroraInitWait;
    //45
    MonFrameSkip.init(45, &m_cfg[45], 0, 8, 200); regMap["MonFrameSkip"] = &Rd53aGlobalCfg::MonFrameSkip;
    //101-102
    AutoReadA0.init(101, &m_cfg[101], 0, 9, 136); regMap["AutoReadA0"] = &Rd53aGlobalCfg::AutoReadA0;
    AutoReadB0.init(102, &m_cfg[102], 0, 9, 130); regMap["AutoReadB0"] = &Rd53aGlobalCfg::AutoReadB0;
    //103-104
    AutoReadA1.init(103, &m_cfg[103], 0, 9, 118); regMap["AutoReadA1"] = &Rd53aGlobalCfg::AutoReadA1;
    AutoReadB1.init(104, &m_cfg[104], 0, 9, 119); regMap["AutoReadB1"] = &Rd53aGlobalCfg::AutoReadB1;
    //105-106
    AutoReadA2.init(105, &m_cfg[105], 0, 9, 120); regMap["AutoReadA2"] = &Rd53aGlobalCfg::AutoReadA2;
    AutoReadB2.init(106, &m_cfg[106], 0, 9, 121); regMap["AutoReadB2"] = &Rd53aGlobalCfg::AutoReadB2;
    //107-108
    AutoReadA3.init(107, &m_cfg[107], 0, 9, 122); regMap["AutoReadA3"] = &Rd53aGlobalCfg::AutoReadA3;
    AutoReadB3.init(108, &m_cfg[108], 0, 9, 123); regMap["AutoReadB3"] = &Rd53aGlobalCfg::AutoReadB3;

    // Test & Monitoring
    //77
    MonitorEnable.init(77, &m_cfg[77], 13, 1, 0); regMap["MonitorEnable"] = &Rd53aGlobalCfg::MonitorEnable;
    MonitorImonMux.init(77, &m_cfg[77], 7, 6, 63); regMap["MonitorImonMux"] = &Rd53aGlobalCfg::MonitorImonMux;
    MonitorVmonMux.init(77, &m_cfg[77], 0, 7, 127); regMap["MonitorVmonMux"] = &Rd53aGlobalCfg::MonitorVmonMux;
    //78-81
    HitOr0MaskSync.init(78, &m_cfg[78], 0, 16, 0); regMap["HitOr0MaskSync"] = &Rd53aGlobalCfg::HitOr0MaskSync;
    HitOr1MaskSync.init(79, &m_cfg[79], 0, 16, 0); regMap["HitOr1MaskSync"] = &Rd53aGlobalCfg::HitOr1MaskSync;
    HitOr2MaskSync.init(80, &m_cfg[80], 0, 16, 0); regMap["HitOr2MaskSync"] = &Rd53aGlobalCfg::HitOr2MaskSync;
    HitOr3MaskSync.init(81, &m_cfg[81], 0, 16, 0); regMap["HitOr3MaskSync"] = &Rd53aGlobalCfg::HitOr3MaskSync;
    //82-89
    HitOr0MaskLin0.init(82, &m_cfg[82], 0, 16, 0); regMap["HitOr0MaskLin0"] = &Rd53aGlobalCfg::HitOr0MaskLin0;
    HitOr0MaskLin1.init(83, &m_cfg[83], 0, 1, 0); regMap["HitOr0MaskLin1"] = &Rd53aGlobalCfg::HitOr0MaskLin1;
    HitOr1MaskLin0.init(84, &m_cfg[84], 0, 16, 0); regMap["HitOr1MaskLin0"] = &Rd53aGlobalCfg::HitOr1MaskLin0;
    HitOr1MaskLin1.init(85, &m_cfg[85], 0, 1, 0); regMap["HitOr1MaskLin1"] = &Rd53aGlobalCfg::HitOr1MaskLin1;
    HitOr2MaskLin0.init(86, &m_cfg[86], 0, 16, 0); regMap["HitOr2MaskLin0"] = &Rd53aGlobalCfg::HitOr2MaskLin0;
    HitOr2MaskLin1.init(87, &m_cfg[87], 0, 1, 0); regMap["HitOr2MaskLin1"] = &Rd53aGlobalCfg::HitOr2MaskLin1;
    HitOr3MaskLin0.init(88, &m_cfg[88], 0, 16, 0); regMap["HitOr3MaskLin0"] = &Rd53aGlobalCfg::HitOr3MaskLin0;
    HitOr3MaskLin1.init(89, &m_cfg[89], 0, 1, 0); regMap["HitOr3MaskLin1"] = &Rd53aGlobalCfg::HitOr3MaskLin1;
    //90-97
    HitOr0MaskDiff0.init(90, &m_cfg[90], 0, 16, 0); regMap["HitOr0MaskDiff0"] = &Rd53aGlobalCfg::HitOr0MaskDiff0;
    HitOr0MaskDiff1.init(91, &m_cfg[91], 0, 1, 0); regMap["HitOr0MaskDiff1"] = &Rd53aGlobalCfg::HitOr0MaskDiff1;
    HitOr1MaskDiff0.init(92, &m_cfg[92], 0, 16, 0); regMap["HitOr1MaskDiff0"] = &Rd53aGlobalCfg::HitOr1MaskDiff0;
    HitOr1MaskDiff1.init(93, &m_cfg[93], 0, 1, 0); regMap["HitOr1MaskDiff1"] = &Rd53aGlobalCfg::HitOr1MaskDiff1;
    HitOr2MaskDiff0.init(94, &m_cfg[94], 0, 16, 0); regMap["HitOr2MaskDiff0"] = &Rd53aGlobalCfg::HitOr2MaskDiff0;
    HitOr2MaskDiff1.init(95, &m_cfg[95], 0, 1, 0); regMap["HitOr2MaskDiff1"] = &Rd53aGlobalCfg::HitOr2MaskDiff1;
    HitOr3MaskDiff0.init(96, &m_cfg[96], 0, 16, 0); regMap["HitOr3MaskDiff0"] = &Rd53aGlobalCfg::HitOr3MaskDiff0;
    HitOr3MaskDiff1.init(97, &m_cfg[97], 0, 1, 0); regMap["HitOr3MaskDiff1"] = &Rd53aGlobalCfg::HitOr3MaskDiff1;
    //98
    AdcRefTrim.init(98, &m_cfg[98], 6, 4, 0); regMap["AdcRefTrim"] = &Rd53aGlobalCfg::AdcRefTrim;
    AdcTrim.init(98, &m_cfg[98], 0, 6, 0); regMap["AdcTrim"] = &Rd53aGlobalCfg::AdcTrim;
    //99
    SensorCfg0.init(99, &m_cfg[99], 0, 11, 0); regMap["SensorCfg0"] = &Rd53aGlobalCfg::SensorCfg0;
    SensorCfg1.init(100, &m_cfg[100], 0, 11, 0); regMap["SensorCfg1"] = &Rd53aGlobalCfg::SensorCfg1;
    //109
    RingOscEn.init(109, &m_cfg[109], 0, 8, 0); regMap["RingOscEn"] = &Rd53aGlobalCfg::RingOscEn;
    //110-117
    RingOsc0.init(110, &m_cfg[110], 0, 16, 0); regMap["RingOsc0"] = &Rd53aGlobalCfg::RingOsc0;
    RingOsc1.init(111, &m_cfg[111], 0, 16, 0); regMap["RingOsc1"] = &Rd53aGlobalCfg::RingOsc1;
    RingOsc2.init(112, &m_cfg[112], 0, 16, 0); regMap["RingOsc2"] = &Rd53aGlobalCfg::RingOsc2;
    RingOsc3.init(113, &m_cfg[113], 0, 16, 0); regMap["RingOsc3"] = &Rd53aGlobalCfg::RingOsc3;
    RingOsc4.init(114, &m_cfg[114], 0, 16, 0); regMap["RingOsc4"] = &Rd53aGlobalCfg::RingOsc4;
    RingOsc5.init(115, &m_cfg[115], 0, 16, 0); regMap["RingOsc5"] = &Rd53aGlobalCfg::RingOsc5;
    RingOsc6.init(116, &m_cfg[116], 0, 16, 0); regMap["RingOsc6"] = &Rd53aGlobalCfg::RingOsc6;
    RingOsc7.init(117, &m_cfg[117], 0, 16, 0); regMap["RingOsc7"] = &Rd53aGlobalCfg::RingOsc7;
    //118
    BcCounter.init(118, &m_cfg[118], 0, 16, 0); regMap["BcCounter"] = &Rd53aGlobalCfg::BcCounter;
    //119
    TrigCounter.init(119, &m_cfg[119], 0, 16, 0); regMap["TrigCounter"] = &Rd53aGlobalCfg::TrigCounter;
    //120
    LockLossCounter.init(120, &m_cfg[120], 0, 16, 0); regMap["LockLossCounter"] = &Rd53aGlobalCfg::LockLossCounter;
    //121
    BflipWarnCounter.init(121, &m_cfg[121], 0, 16, 0); regMap["BflipWarnCounter"] = &Rd53aGlobalCfg::BflipWarnCounter;
    //122
    BflipErrCounter.init(122, &m_cfg[122], 0, 16, 0); regMap["BflipErrCounter"] = &Rd53aGlobalCfg::BflipErrCounter;
    //123
    CmdErrCounter.init(123, &m_cfg[123], 0, 16, 0); regMap["CmdErrCounter"] = &Rd53aGlobalCfg::CmdErrCounter;
    //124-127
    FifoFullCounter0.init(124, &m_cfg[124], 0, 8, 0); regMap["FifoFullCounter0"] = &Rd53aGlobalCfg::FifoFullCounter0;
    FifoFullCounter1.init(125, &m_cfg[125], 0, 8, 0); regMap["FifoFullCounter1"] = &Rd53aGlobalCfg::FifoFullCounter1;
    FifoFullCounter2.init(126, &m_cfg[126], 0, 8, 0); regMap["FifoFullCounter2"] = &Rd53aGlobalCfg::FifoFullCounter2;
    FifoFullCounter3.init(127, &m_cfg[127], 0, 8, 0); regMap["FifoFullCounter3"] = &Rd53aGlobalCfg::FifoFullCounter3;
    //128
    AiPixCol.init(128, &m_cfg[128], 0, 8, 0); regMap["AiPixCol"] = &Rd53aGlobalCfg::AiPixCol;
    //129
    AiPixRow.init(129, &m_cfg[129], 0, 9, 0); regMap["AiPixRow"] = &Rd53aGlobalCfg::AiPixRow;
    //130-133
    HitOrCounter0.init(130, &m_cfg[130], 0, 16, 0); regMap["HitOrCounter0"] = &Rd53aGlobalCfg::HitOrCounter0;
    HitOrCounter1.init(131, &m_cfg[131], 0, 16, 0); regMap["HitOrCounter1"] = &Rd53aGlobalCfg::HitOrCounter1;
    HitOrCounter2.init(132, &m_cfg[132], 0, 16, 0); regMap["HitOrCounter2"] = &Rd53aGlobalCfg::HitOrCounter2;
    HitOrCounter3.init(133, &m_cfg[133], 0, 16, 0); regMap["HitOrCounter3"] = &Rd53aGlobalCfg::HitOrCounter3;
    //134
    SkipTriggerCounter.init(134, &m_cfg[134], 0, 16, 0); regMap["SkipTriggerCounter"] = &Rd53aGlobalCfg::SkipTriggerCounter;
    //135
    ErrMask.init(135, &m_cfg[135], 0, 14, 0); regMap["ErrMask"] = &Rd53aGlobalCfg::ErrMask;
    //136
    AdcRead.init(136, &m_cfg[136], 0, 11, 0); regMap["AdcRead"] = &Rd53aGlobalCfg::AdcRead;
    //137
    SelfTrigEn.init(137, &m_cfg[137], 0, 4, 0); regMap["SelfTrigEn"] = &Rd53aGlobalCfg::SelfTrigEn;

    // Special diff registers
    InjVcalDiff.init(&InjVcalMed, &InjVcalHigh, true); regMap["InjVcalDiff"] = (Rd53aReg Rd53aGlobalCfg::*)&Rd53aGlobalCfg::InjVcalDiff;
}

void Rd53aGlobalCfg::toFileJson(json &j) {
    for(auto it : regMap) {
        j["RD53A"]["GlobalConfig"][it.first] = (this->*it.second).read();
    }    
}

void Rd53aGlobalCfg::fromFileJson(json &j) {
    for (auto it : regMap) {
        if (!j["RD53A"]["GlobalConfig"][it.first].empty()) {
            (this->*it.second).write(j["RD53A"]["GlobalConfig"][it.first]);
        } else {
            std::cerr << " --> Error: Could not find register \"" << it.first << "\", using default!" << std::endl;
        }
    }
}
