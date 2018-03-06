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
    PixPortal.init(0, &m_cfg[0], 0, 16, 0x0); regMap["PixPortal"] = &PixPortal;
    //1
    PixRegionCol.init(1, &m_cfg[1], 0, 8, 0x0); regMap["PixRegionCol"] = &PixRegionCol;
    //2
    PixRegionRow.init(2, &m_cfg[2], 0, 9, 0x0); regMap["PixRegionRow"] = &PixRegionRow;
    //3
    PixBroadcastEn.init(3, &m_cfg[3], 5, 1, 0x0); regMap["PixBroadcastEn"] = &PixBroadcastEn;
    PixAutoCol.init(3, &m_cfg[3], 4, 1, 0x0); regMap["PixAutoCol"] = &PixAutoCol;
    PixAutoRow.init(3, &m_cfg[3], 3, 1, 0x0); regMap["PixAutoRow"] = &PixAutoRow;
    PixBroadcastMask.init(3, &m_cfg[3], 0, 3, 0x0); regMap["PixBroadcastMask"] = &PixBroadcastMask;
    //4
    PixDefaultConfig.init(4, &m_cfg[4], 0, 16, 0x0);  regMap["PixDefaultConfig"] = &PixDefaultConfig;

    // Sync FE
    //5
    SyncIbiasp1.init(5, &m_cfg[5], 0, 9, 100); regMap["SyncIbiasp1"] = &SyncIbiasp1;
    //6
    SyncIbiasp2.init(6, &m_cfg[6], 0, 9, 150); regMap["SyncIbiasp2"] = &SyncIbiasp2;
    //7
    SyncIbiasSf.init(7, &m_cfg[7], 0, 9, 100); regMap["SyncIbiasSf"] = &SyncIbiasSf;
    //8
    SyncIbiasKrum.init(8, &m_cfg[8], 0, 9, 140); regMap["SyncIbiasKrum"] = &SyncIbiasKrum;
    //9
    SyncIbiasDisc.init(9, &m_cfg[9], 0, 9, 200); regMap["SyncIbiasDisc"] = &SyncIbiasDisc;
    //10
    SyncIctrlSynct.init(10, &m_cfg[10], 0, 10, 100); regMap["SyncIctrlSynct"] = &SyncIctrlSynct;
    //11
    SyncVbl.init(11, &m_cfg[11], 0, 10, 450); regMap["SyncVbl"] = &SyncVbl;
    //12
    SyncVth.init(12, &m_cfg[12], 0, 10, 300); regMap["SyncVth"] = &SyncVth;
    //13
    SyncVrefKrum.init(13, &m_cfg[13], 0, 10, 490); regMap["SyncVrefKrum"] = &SyncVrefKrum;
    //30
    SyncAutoZero.init(30, &m_cfg[30], 3, 2, 0); regMap["SyncAutoZero"] = &SyncAutoZero;
    SyncSelC2F.init(30, &m_cfg[30], 2, 1, 1); regMap["SyncSelC2F"] = &SyncSelC2F;
    SyncSelC4F.init(30, &m_cfg[30], 1, 1, 0); regMap["SyncSelC4F"] = &SyncSelC4F;
    SyncFastTot.init(30, &m_cfg[30], 0, 1, 0); regMap["SyncFastTot"] = &SyncFastTot;

    // Linear FE
    //14
    LinPaInBias.init(14, &m_cfg[14], 0, 9, 300); regMap["LinPaInBias"] = &LinPaInBias;
    //15
    LinFcBias.init(15, &m_cfg[15], 0, 8, 20); regMap["LinFcBias"] = &LinFcBias;
    //16
    LinKrumCurr.init(16, &m_cfg[16], 0, 9, 50); regMap["LinKrumCurr"] = &LinKrumCurr;
    //17
    LinLdac.init(17, &m_cfg[17], 0, 10, 80); regMap["LinLdac"] = &LinLdac;
    //18
    LinComp.init(18, &m_cfg[18], 0, 9, 110); regMap["LinComp"] = &LinComp;
    //19
    LinRefKrum.init(19, &m_cfg[19], 0, 10, 300); regMap["LinRefKrum"] = &LinRefKrum;
    //20
    LinVth.init(20, &m_cfg[20], 0, 10, 408); regMap["LinVth"] = &LinVth;

    // Diff FE
    //21
    DiffPrmp.init(21, &m_cfg[21], 0, 10, 500); regMap["DiffPrmp"] = &DiffPrmp;
    //22
    DiffFol.init(22, &m_cfg[22], 0, 10, 500); regMap["DiffFol"] = &DiffFol;
    //23
    DiffPrecomp.init(23, &m_cfg[23], 0, 10, 500); regMap["DiffPrecomp"] = &DiffPrecomp;
    //24
    DiffComp.init(24, &m_cfg[24], 0, 10, 528); regMap["DiffComp"] = &DiffComp;
    //25
    DiffVff.init(25, &m_cfg[25], 0, 10, 160); regMap["DiffVff"] = &DiffVff;
    //26
    DiffVth1.init(26, &m_cfg[26], 0, 10, 250); regMap["DiffVth1"] = &DiffVth1;
    //27
    DiffVth2.init(27, &m_cfg[27], 0, 10, 50); regMap["DiffVth2"] = &DiffVth2;
    //28
    DiffLcc.init(28, &m_cfg[28], 0, 10, 20); regMap["DiffLcc"] = &DiffLcc;
    //29
    DiffLccEn.init(29, &m_cfg[29], 1, 1, 0); regMap["DiffLccEn"] = &DiffLccEn;
    DiffFbCapEn.init(29, &m_cfg[29], 0, 1, 0); regMap["DiffFbCapEn"] = &DiffFbCapEn;

    //Power
    //31
    SldoAnalogTrim.init(31, &m_cfg[31], 5, 5, 16); regMap["SldoAnalogTrim"] = &SldoAnalogTrim;
    SldoDigitalTrim.init(31, &m_cfg[31], 0, 5, 16); regMap["SldoDigitalTrim"] = &SldoDigitalTrim;

    // Digital Matrix
    //32
    EnCoreColSync.init(32, &m_cfg[32], 0, 16, 0xFFFF); regMap["EnCoreColSync"] = &EnCoreColSync;
    //33
    EnCoreColLin1.init(33, &m_cfg[33], 0, 16, 0xFFFF); regMap["EnCoreColLin1"] = &EnCoreColLin1;
    //34
    EnCoreColLin2.init(34, &m_cfg[34], 0, 1, 1); regMap["EnCoreColLin2"] = &EnCoreColLin2;
    //35
    EnCoreColDiff1.init(35, &m_cfg[35], 0, 16, 0xFFFF); regMap["EnCoreColDiff1"] = &EnCoreColDiff1;
    //36
    EnCoreColDiff2.init(36, &m_cfg[36], 0, 1, 1); regMap["EnCoreColDiff2"] = &EnCoreColDiff2;
    //37
    LatencyConfig.init(37, &m_cfg[37], 0, 9, 64); regMap["LatencyConfig"] = &LatencyConfig;
    //38
    WrSyncDelaySync.init(38, &m_cfg[38], 0, 5, 16); regMap["WrSyncDelaySync"] = &WrSyncDelaySync;

    // Injection
    //39
    InjAnaMode.init(39, &m_cfg[39], 5, 1, 0); regMap["InjAnaMode"] = &InjAnaMode;
    InjEnDig.init(39, &m_cfg[39], 4, 1, 0); regMap["InjEnDig"] = &InjEnDig;
    InjDelay.init(39, &m_cfg[39], 0, 4, 0); regMap["InjDelay"] = &InjDelay;
    //41
    InjVcalHigh.init(41, &m_cfg[41], 0, 12, 500); regMap["InjVcalHigh"] = &InjVcalHigh;
    //42
    InjVcalMed.init(42, &m_cfg[42], 0, 12, 300); regMap["InjVcalMed"] = &InjVcalMed;
    //46
    CalColprSync1.init(46, &m_cfg[46], 0, 16, 0xFFFF); regMap["CalColprSync1"] = &CalColprSync1;
    //47
    CalColprSync2.init(47, &m_cfg[47], 0, 16, 0xFFFF); regMap["CalColprSync2"] = &CalColprSync2;
    //48
    CalColprSync3.init(48, &m_cfg[48], 0, 16, 0xFFFF); regMap["CalColprSync3"] = &CalColprSync3;
    //49
    CalColprSync4.init(49, &m_cfg[49], 0, 16, 0xFFFF); regMap["CalColprSync4"] = &CalColprSync4;
    //50
    CalColprLin1.init(50, &m_cfg[50], 0, 16, 0xFFFF); regMap["CalColprLin1"] = &CalColprLin1;
    //51
    CalColprLin2.init(51, &m_cfg[51], 0, 16, 0xFFFF); regMap["CalColprLin2"] = &CalColprLin2;
    //52
    CalColprLin3.init(52, &m_cfg[52], 0, 16, 0xFFFF); regMap["CalColprLin3"] = &CalColprLin3;
    //53
    CalColprLin4.init(53, &m_cfg[53], 0, 16, 0xFFFF); regMap["CalColprLin4"] = &CalColprLin4;
    //54
    CalColprLin5.init(54, &m_cfg[54], 0, 4, 0xF); regMap["CalColprLin5"] = &CalColprLin5;
    //55
    CalColprDiff1.init(55, &m_cfg[55], 0, 16, 0xFFFF); regMap["CalColprDiff1"] = &CalColprDiff1;
    //56
    CalColprDiff2.init(56, &m_cfg[56], 0, 16, 0xFFFF); regMap["CalColprDiff2"] = &CalColprDiff2;
    //57
    CalColprDiff3.init(57, &m_cfg[57], 0, 16, 0xFFFF); regMap["CalColprDiff3"] = &CalColprDiff3;
    //58
    CalColprDiff4.init(58, &m_cfg[58], 0, 16, 0xFFFF); regMap["CalColprDiff4"] = &CalColprDiff4;
    //59
    CalColprDiff5.init(59, &m_cfg[59], 0, 4, 0xF); regMap["CalColprDiff5"] = &CalColprDiff5;

    // Digital Functions
    //40
    ClkDelaySel.init(40, &m_cfg[40], 8, 1, 0); regMap["ClkDelaySel"] = &ClkDelaySel;
    ClkDelay.init(40, &m_cfg[40], 4, 4, 0); regMap["ClkDelay"] = &ClkDelay;
    CmdDelay.init(40, &m_cfg[40], 0, 4, 0); regMap["CmdDelay"] = &CmdDelay;
    //43
    ChSyncPhase.init(43, &m_cfg[43], 10, 2, 0); regMap["ChSyncPhase"] = &ChSyncPhase;
    ChSyncLock.init(43, &m_cfg[43], 5, 5, 16); regMap["ChSyncLock"] = &ChSyncLock;
    ChSyncUnlock.init(43, &m_cfg[43], 0, 5, 8); regMap["ChSyncUnlock"] = &ChSyncUnlock;
    //44
    GlobalPulseRt.init(44, &m_cfg[44], 0, 16, 0); regMap["GlobalPulseRt"] = &GlobalPulseRt;

    // I/O
    //60
    DebugConfig.init(60, &m_cfg[60], 0, 2, 0); regMap["DebugConfig"] = &DebugConfig;
    //61
    OutputDataReadDelay.init(61, &m_cfg[61], 7, 2, 0); regMap["OutputDataReadDelay"] = &OutputDataReadDelay;
    OutputSerType.init(61, &m_cfg[61], 6, 1, 0); regMap["OutputSerType"] = &OutputSerType;
    OutputActiveLanes.init(61, &m_cfg[61], 2, 4, 0xF); regMap["OutputActiveLanes"] = &OutputActiveLanes;
    OutputFmt.init(61, &m_cfg[61], 0, 2, 0); regMap["OutputFmt"] = &OutputFmt;
    //62
    OutPadConfig.init(62, &m_cfg[62], 0, 13, 0x1404); regMap["OutPadConfig"] = &OutPadConfig;
    //63
    GpLvdsRoute.init(63, &m_cfg[63], 0, 3, 0); regMap["GpLvdsRoute"] = &GpLvdsRoute;
    //64
    CdrSelDelClk.init(64, &m_cfg[64], 13, 1, 0); regMap["CdrSelDelClk"] = &CdrSelDelClk;
    CdrPdSel.init(64, &m_cfg[64], 11, 2, 0); regMap["CdrPdSel"] = &CdrPdSel;
    CdrPdDel.init(64, &m_cfg[64], 7, 4, 8); regMap["CdrPdDel"] = &CdrPdDel;
    CdrEnGck.init(64, &m_cfg[64], 6, 1, 0); regMap["CdrEnGck"] = &CdrEnGck;
    CdrVcoGain.init(64, &m_cfg[64], 3, 3, 3); regMap["CdrVcoGain"] = &CdrVcoGain;
    CdrSelSerClk.init(64, &m_cfg[64], 0, 3, 3); regMap["CdrSelSerClk"] = &CdrSelSerClk;
    //65
    VcoBuffBias.init(65, &m_cfg[65], 0, 10, 400); regMap["VcoBuffBias"] = &VcoBuffBias;
    //66
    CdrCpIbias.init(66, &m_cfg[66], 0, 10, 50); regMap["CdrCpIbias"] = &CdrCpIbias;
    //67
    VcoIbias.init(67, &m_cfg[67], 0, 10, 500); regMap["VcoIbias"] = &VcoIbias;
    //68
    SerSelOut0.init(68, &m_cfg[68], 0, 2, 1); regMap["SerSelOut0"] = &SerSelOut0;
    SerSelOut1.init(68, &m_cfg[68], 2, 2, 1); regMap["SerSelOut1"] = &SerSelOut1;
    SerSelOut2.init(68, &m_cfg[68], 4, 2, 1); regMap["SerSelOut2"] = &SerSelOut2;
    SerSelOut3.init(68, &m_cfg[68], 6, 2, 1); regMap["SerSelOut3"] = &SerSelOut3;
    //69
    CmlInvTap.init(69, &m_cfg[69], 6, 2, 0x0); regMap["CmlInvTap"] = &CmlInvTap;
    CmlEnTap.init(69, &m_cfg[69], 4, 2, 0x3); regMap["CmlEnTap"] = &CmlEnTap;
    CmlEn.init(69, &m_cfg[69], 0, 4, 0xF); regMap["CmlEn"] = &CmlEn;
    //70-72
    CmlTapBias0.init(70, &m_cfg[70], 0, 10, 400); regMap["CmlTapBias0"] = &CmlTapBias0;
    CmlTapBias1.init(71, &m_cfg[71], 0, 10, 0); regMap["CmlTapBias1"] = &CmlTapBias1;
    CmlTapBias2.init(72, &m_cfg[72], 0, 10, 0); regMap["CmlTapBias2"] = &CmlTapBias2;
    //73
    AuroraCcWait.init(73, &m_cfg[73], 2, 6, 25); regMap["AuroraCcWait"] = &AuroraCcWait;
    AuroraCcSend.init(73, &m_cfg[73], 0, 2, 3); regMap["AuroraCcSend"] = &AuroraCcSend;
    //74
    AuroraCbWaitLow.init(74, &m_cfg[74], 4, 4, 15); regMap["AuroraCbWaitLow"] = &AuroraCbWaitLow;
    AuroraCbSend.init(74, &m_cfg[74], 0, 4, 0); regMap["AuroraCbSend"] = &AuroraCbSend;
    //75
    AuroraCbWaitHigh.init(75, &m_cfg[75], 0, 16, 15); regMap["AuroraCbWaitHigh"] = &AuroraCbWaitHigh;
    //76
    AuroraInitWait.init(76, &m_cfg[76], 0, 11, 32); regMap["AuroraInitWait"] = &AuroraInitWait;
    //45
    MonFrameSkip.init(45, &m_cfg[45], 0, 8, 200); regMap["MonFrameSkip"] = &MonFrameSkip;
    //101-102
    AutoReadA0.init(101, &m_cfg[101], 0, 9, 136); regMap["AutoReadA0"] = &AutoReadA0;
    AutoReadB0.init(102, &m_cfg[102], 0, 9, 130); regMap["AutoReadB0"] = &AutoReadB0;
    //103-104
    AutoReadA1.init(103, &m_cfg[103], 0, 9, 118); regMap["AutoReadA1"] = &AutoReadA1;
    AutoReadB1.init(104, &m_cfg[104], 0, 9, 119); regMap["AutoReadB1"] = &AutoReadB1;
    //105-106
    AutoReadA2.init(105, &m_cfg[105], 0, 9, 120); regMap["AutoReadA2"] = &AutoReadA2;
    AutoReadB2.init(106, &m_cfg[106], 0, 9, 121); regMap["AutoReadB2"] = &AutoReadB2;
    //107-108
    AutoReadA3.init(107, &m_cfg[107], 0, 9, 122); regMap["AutoReadA3"] = &AutoReadA3;
    AutoReadB3.init(108, &m_cfg[108], 0, 9, 123); regMap["AutoReadB3"] = &AutoReadB3;

    // Test & Monitoring
    //77
    MonitorEnable.init(77, &m_cfg[77], 13, 1, 0); regMap["MonitorEnable"] = &MonitorEnable;
    MonitorImonMux.init(77, &m_cfg[77], 7, 6, 63); regMap["MonitorImonMux"] = &MonitorImonMux;
    MonitorVmonMux.init(77, &m_cfg[77], 0, 7, 127); regMap["MonitorVmonMux"] = &MonitorVmonMux;
    //78-81
    HitOr0MaskSync.init(78, &m_cfg[78], 0, 16, 0); regMap["HitOr0MaskSync"] = &HitOr0MaskSync;
    HitOr1MaskSync.init(79, &m_cfg[79], 0, 16, 0); regMap["HitOr1MaskSync"] = &HitOr1MaskSync;
    HitOr2MaskSync.init(80, &m_cfg[80], 0, 16, 0); regMap["HitOr2MaskSync"] = &HitOr2MaskSync;
    HitOr3MaskSync.init(81, &m_cfg[81], 0, 16, 0); regMap["HitOr3MaskSync"] = &HitOr3MaskSync;
    //82-89
    HitOr0MaskLin0.init(82, &m_cfg[82], 0, 16, 0); regMap["HitOr0MaskLin0"] = &HitOr0MaskLin0;
    HitOr0MaskLin1.init(83, &m_cfg[83], 0, 16, 0); regMap["HitOr0MaskLin1"] = &HitOr0MaskLin1;
    HitOr1MaskLin0.init(84, &m_cfg[84], 0, 16, 0); regMap["HitOr1MaskLin0"] = &HitOr1MaskLin0;
    HitOr1MaskLin1.init(85, &m_cfg[85], 0, 16, 0); regMap["HitOr1MaskLin1"] = &HitOr1MaskLin1;
    HitOr2MaskLin0.init(86, &m_cfg[86], 0, 16, 0); regMap["HitOr2MaskLin0"] = &HitOr2MaskLin0;
    HitOr2MaskLin1.init(87, &m_cfg[87], 0, 16, 0); regMap["HitOr2MaskLin1"] = &HitOr2MaskLin1;
    HitOr3MaskLin0.init(88, &m_cfg[88], 0, 16, 0); regMap["HitOr3MaskLin0"] = &HitOr3MaskLin0;
    HitOr3MaskLin1.init(89, &m_cfg[89], 0, 16, 0); regMap["HitOr3MaskLin1"] = &HitOr3MaskLin1;
    //90-97
    HitOr0MaskDiff0.init(90, &m_cfg[90], 0, 16, 0); regMap["HitOr0MaskDiff0"] = &HitOr0MaskDiff0;
    HitOr0MaskDiff1.init(91, &m_cfg[91], 0, 16, 0); regMap["HitOr0MaskDiff1"] = &HitOr0MaskDiff1;
    HitOr1MaskDiff0.init(92, &m_cfg[92], 0, 16, 0); regMap["HitOr1MaskDiff0"] = &HitOr1MaskDiff0;
    HitOr1MaskDiff1.init(93, &m_cfg[93], 0, 16, 0); regMap["HitOr1MaskDiff1"] = &HitOr1MaskDiff1;
    HitOr2MaskDiff0.init(94, &m_cfg[94], 0, 16, 0); regMap["HitOr2MaskDiff0"] = &HitOr2MaskDiff0;
    HitOr2MaskDiff1.init(95, &m_cfg[95], 0, 16, 0); regMap["HitOr2MaskDiff1"] = &HitOr2MaskDiff1;
    HitOr3MaskDiff0.init(96, &m_cfg[96], 0, 16, 0); regMap["HitOr3MaskDiff0"] = &HitOr3MaskDiff0;
    HitOr3MaskDiff1.init(97, &m_cfg[97], 0, 16, 0); regMap["HitOr3MaskDiff1"] = &HitOr3MaskDiff1;
    //98
    AdcRefTrim.init(98, &m_cfg[98], 6, 4, 0); regMap["AdcRefTrim"] = &AdcRefTrim;
    AdcTrim.init(98, &m_cfg[98], 0, 6, 0); regMap["AdcTrim"] = &AdcTrim;
    //99
    SensorCfg0.init(99, &m_cfg[99], 0, 11, 0); regMap["SensorCfg0"] = &SensorCfg0;
    SensorCfg1.init(100, &m_cfg[100], 0, 11, 0); regMap["SensorCfg1"] = &SensorCfg1;
    //109
    RingOscEn.init(109, &m_cfg[109], 0, 8, 0); regMap["RingOscEn"] = &RingOscEn;
    //110-117
    RingOsc0.init(110, &m_cfg[110], 0, 16, 0); regMap["RingOsc0"] = &RingOsc0;
    RingOsc1.init(111, &m_cfg[111], 0, 16, 0); regMap["RingOsc1"] = &RingOsc1;
    RingOsc2.init(112, &m_cfg[112], 0, 16, 0); regMap["RingOsc2"] = &RingOsc2;
    RingOsc3.init(113, &m_cfg[113], 0, 16, 0); regMap["RingOsc3"] = &RingOsc3;
    RingOsc4.init(114, &m_cfg[114], 0, 16, 0); regMap["RingOsc4"] = &RingOsc4;
    RingOsc5.init(115, &m_cfg[115], 0, 16, 0); regMap["RingOsc5"] = &RingOsc5;
    RingOsc6.init(116, &m_cfg[116], 0, 16, 0); regMap["RingOsc6"] = &RingOsc6;
    RingOsc7.init(117, &m_cfg[117], 0, 16, 0); regMap["RingOsc7"] = &RingOsc7;
    //118
    BcCounter.init(118, &m_cfg[118], 0, 16, 0); regMap["BcCounter"] = &BcCounter;
    //119
    TrigCounter.init(119, &m_cfg[119], 0, 16, 0); regMap["TrigCounter"] = &TrigCounter;
    //120
    LockLossCounter.init(120, &m_cfg[120], 0, 16, 0); regMap["LockLossCounter"] = &LockLossCounter;
    //121
    BflipWarnCounter.init(121, &m_cfg[121], 0, 16, 0); regMap["BflipWarnCounter"] = &BflipWarnCounter;
    //122
    BflipErrCounter.init(122, &m_cfg[122], 0, 16, 0); regMap["BflipErrCounter"] = &BflipErrCounter;
    //123
    CmdErrCounter.init(123, &m_cfg[123], 0, 16, 0); regMap["CmdErrCounter"] = &CmdErrCounter;
    //124-127
    FifoFullCounter0.init(124, &m_cfg[124], 0, 8, 0); regMap["FifoFullCounter0"] = &FifoFullCounter0;
    FifoFullCounter1.init(125, &m_cfg[125], 0, 8, 0); regMap["FifoFullCounter1"] = &FifoFullCounter1;
    FifoFullCounter2.init(126, &m_cfg[126], 0, 8, 0); regMap["FifoFullCounter2"] = &FifoFullCounter2;
    FifoFullCounter3.init(127, &m_cfg[127], 0, 8, 0); regMap["FifoFullCounter3"] = &FifoFullCounter3;
    //128
    AiPixCol.init(128, &m_cfg[128], 0, 8, 0); regMap["AiPixCol"] = &AiPixCol;
    //129
    AiPixRow.init(129, &m_cfg[129], 0, 9, 0); regMap["AiPixRow"] = &AiPixRow;
    //130-133
    HitOrCounter0.init(130, &m_cfg[130], 0, 16, 0); regMap["HitOrCounter0"] = &HitOrCounter0;
    HitOrCounter1.init(131, &m_cfg[131], 0, 16, 0); regMap["HitOrCounter1"] = &HitOrCounter1;
    HitOrCounter2.init(132, &m_cfg[132], 0, 16, 0); regMap["HitOrCounter2"] = &HitOrCounter2;
    HitOrCounter3.init(133, &m_cfg[133], 0, 16, 0); regMap["HitOrCounter3"] = &HitOrCounter3;
    //134
    SkipTriggerCounter.init(134, &m_cfg[134], 0, 16, 0); regMap["SkipTriggerCounter"] = &SkipTriggerCounter;
    //135
    ErrMask.init(135, &m_cfg[135], 0, 14, 0); regMap["ErrMask"] = &ErrMask;
    //136
    AdcRead.init(136, &m_cfg[136], 0, 11, 0); regMap["AdcRead"] = &AdcRead;
    //137
    SelfTrigEn.init(137, &m_cfg[137], 0, 4, 0); regMap["SelfTrigEn"] = &SelfTrigEn;
}

void Rd53aGlobalCfg::toFileJson(json &j) {
    for(auto it : regMap) {
        j["RD53A"]["GlobalConfig"][it.first] = it.second->read();
    }    
}

void Rd53aGlobalCfg::fromFileJson(json &j) {
    for (auto it : regMap) {
        if (!j["RD53A"]["GlobalConfig"][it.first].empty()) {
            it.second->write(j["RD53A"]["GlobalConfig"][it.first]);
        } else {
            std::cerr << " --> Error: Could not find register \"" << it.first << "\", using default!" << std::endl;
        }
    }
}
