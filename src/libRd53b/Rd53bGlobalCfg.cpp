// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: RD53B library
// # Comment: RD53B global register
// # Date: May 2020
// ################################

#include "Rd53bGlobalCfg.h"

#include "logging.h"

namespace {
    auto logger = logging::make_log("Rd53bGlobalCfg");
}

Rd53bGlobalCfg::Rd53bGlobalCfg() {

}

Rd53bGlobalCfg::~Rd53bGlobalCfg() {

}

void Rd53bGlobalCfg::init() {
    // Reset array
    for (unsigned i=0; i<numRegs; i++) {
        m_cfg[i] = 0x00;
    }

    //0
    PixPortal.init          (  0, &m_cfg[  0], 0, 16, 0);
    //1
    PixRegionCol.init       (  1, &m_cfg[  0], 0, 16, 0);
    //2
    PixRegionRow.init       (  2, &m_cfg[  0], 0, 16, 0);
    //3
    PixMode.init            (  3, &m_cfg[  0], 0, 16, 0);
    //4
    PixDefaultConfig.init   (  4, &m_cfg[  0], 0, 16, 0);
    //5
    PixDefaultConfigB.init  (  5, &m_cfg[  0], 0, 16, 0);
    //6
    GcrDefaultConfig.init   (  6, &m_cfg[  0], 0, 16, 0);
    //7
    GcrDefaultConfigB.init  (  7, &m_cfg[  0], 0, 16, 0);

    // Diff AFE
    //8
    DiffPreampL.init        (  8, &m_cfg[  0], 0, 16, 0);
    //9
    DiffPreampR.init        (  9, &m_cfg[  0], 0, 16, 0);
    //10
    DiffPreampTL.init       ( 10, &m_cfg[  0], 0, 16, 0);
    //11
    DiffPreampTR.init       ( 11, &m_cfg[  0], 0, 16, 0);
    //12
    DiffPreampT.init        ( 12, &m_cfg[  0], 0, 16, 0);
    //13
    DiffPreampM.init        ( 13, &m_cfg[  0], 0, 16, 0);
    //14
    DiffPreComp.init        ( 14, &m_cfg[  0], 0, 16, 0);
    //15
    DiffComp.init           ( 15, &m_cfg[  0], 0, 16, 0);
    //16
    DiffVff.init            ( 16, &m_cfg[  0], 0, 16, 0);
    //17
    DiffTh1L.init           ( 17, &m_cfg[  0], 0, 16, 0);
    //18
    DiffTh1R.init           ( 18, &m_cfg[  0], 0, 16, 0);
    //19
    DiffTh1M.init           ( 19, &m_cfg[  0], 0, 16, 0);
    //20
    DiffTh2.init            ( 20, &m_cfg[  0], 0, 16, 0);
    //21
    DiffLcc.init            ( 21, &m_cfg[  0], 0, 16, 0);
    //37
    DiffLccEn.init          ( 37, &m_cfg[  0], 0, 16, 0);
    DiffFbCapEn.init        ( 37, &m_cfg[  0], 0, 16, 0);

    // Lin AFE
    //22
    LinPreampL.init         ( 22, &m_cfg[  0], 0, 16, 0);
    //23
    LinPreampR.init         ( 23, &m_cfg[  0], 0, 16, 0);
    //24
    LinPreampTL.init        ( 24, &m_cfg[  0], 0, 16, 0);
    //25
    LinPreampTR.init        ( 25, &m_cfg[  0], 0, 16, 0);
    //26
    LinPreampT.init         ( 26, &m_cfg[  0], 0, 16, 0);
    //27
    LinPreampM.init         ( 27, &m_cfg[  0], 0, 16, 0);
    //28
    LinFc.init              ( 28, &m_cfg[  0], 0, 16, 0);
    //29
    LinKrumCurr.init        ( 29, &m_cfg[  0], 0, 16, 0);
    //30
    LinRefKrum.init         ( 30, &m_cfg[  0], 0, 16, 0);
    //31
    LinComp.init            ( 31, &m_cfg[  0], 0, 16, 0);
    //32
    LinCompTa.init          ( 32, &m_cfg[  0], 0, 16, 0);
    //33
    LinGdacL.init           ( 33, &m_cfg[  0], 0, 16, 0);
    //34
    LinGdacR.init           ( 34, &m_cfg[  0], 0, 16, 0);
    //35
    LinGdacM.init           ( 35, &m_cfg[  0], 0, 16, 0);
    //36
    LinLdac.init            ( 36, &m_cfg[  0], 0, 16, 0);

    // Power
    //38
    SldoEnUndershuntA.init  ( 38, &m_cfg[  0], 0, 16, 0);
    SldoEnUndershuntB.init  ( 28, &m_cfg[  0], 0, 16, 0);
    SldoTrimA.init          ( 38, &m_cfg[  0], 0, 16, 0);
    SldoTrimB.init          ( 38, &m_cfg[  0], 0, 16, 0);

    // Pixel Matrix
    //39
    EnCoreCol3.init         ( 39, &m_cfg[  0], 0, 16, 0);
    //40
    EnCoreCol2.init         ( 40, &m_cfg[  0], 0, 16, 0);
    //41
    EnCoreCol1.init         ( 41, &m_cfg[  0], 0, 16, 0);
    //42
    EnCoreCol0.init         ( 42, &m_cfg[  0], 0, 16, 0);
    //43
    RstCoreCol3.init        ( 43, &m_cfg[  0], 0, 16, 0);
    //44
    RstCoreCol2.init        ( 44, &m_cfg[  0], 0, 16, 0);
    //45
    RstCoreCol1.init        ( 45, &m_cfg[  0], 0, 16, 0);
    //46
    RstCoreCol0.init        ( 46, &m_cfg[  0], 0, 16, 0);

    // Digital functions
    //47
    TrigMode.init           ( 47, &m_cfg[  0], 0, 16, 0);
    Latency.init            ( 47, &m_cfg[  0], 0, 16, 0);
    //48
    SelfTrigEn.init         ( 48, &m_cfg[  0], 0, 16, 0);
    SelfTrigDigThrEn.init   ( 48, &m_cfg[  0], 0, 16, 0);
    SelfTrigDigThr.init     ( 48, &m_cfg[  0], 0, 16, 0);
    //49
    SelTrigDelay.init       ( 49, &m_cfg[  0], 0, 16, 0);
    SelfTrigMulti.init      ( 49, &m_cfg[  0], 0, 16, 0);
    //50
    SelfTrigPattern.init    ( 50, &m_cfg[  0], 0, 16, 0);
    //51
    ColReadDelay.init       ( 51, &m_cfg[  0], 0, 16, 0);
    ReadTrigLatency.init    ( 51, &m_cfg[  0], 0, 16, 0);
    //52
    TruncTimeoutConf.init   ( 52, &m_cfg[  0], 0, 16, 0);
    //53
    InjDigEn.init           ( 53, &m_cfg[  0], 0, 16, 0);
    InjAnaMode.init         ( 53, &m_cfg[  0], 0, 16, 0);
    InjFineDelay.init       ( 53, &m_cfg[  0], 0, 16, 0);
    //54
    FineDelayClk.init       ( 54, &m_cfg[  0], 0, 16, 0);
    FineDelayData.init      ( 54, &m_cfg[  0], 0, 16, 0);
    //55
    InjVcalHigh.init        ( 55, &m_cfg[  0], 0, 16, 0);
    //56
    InjVcalMed.init         ( 56, &m_cfg[  0], 0, 16, 0);
    //57
    CapMeasEnPar.init       ( 57, &m_cfg[  0], 0, 16, 0);
    CapMeasEn.init          ( 57, &m_cfg[  0], 0, 16, 0);
    InjVcalRange.init       ( 57, &m_cfg[  0], 0, 16, 0);
    //58
    CdrOverwriteLimit.init  ( 58, &m_cfg[  0], 0, 16, 0);
    CdrPhaseDetSel.init     ( 58, &m_cfg[  0], 0, 16, 0);
    CdrClkSel.init          ( 58, &m_cfg[  0], 0, 16, 0);
    //59
    ChSyncLockThr.init      ( 59, &m_cfg[  0], 0, 16, 0);
    //60
    GlobalPulseConf.init    ( 60, &m_cfg[  0], 0, 16, 0);
    //61
    GlobalPulseWidth.init   ( 61, &m_cfg[  0], 0, 16, 0);
    //62
    ServiceBlockEn.init     ( 62, &m_cfg[  0], 0, 16, 0);
    ServiceBlockPeriod.init ( 62, &m_cfg[  0], 0, 16, 0);
    //63
    TotEnPtot.init          ( 63, &m_cfg[  0], 0, 16, 0);
    TotEnPtoa.init          ( 63, &m_cfg[  0], 0, 16, 0);
    TotEn80.init            ( 63, &m_cfg[  0], 0, 16, 0);
    TotEn6b4b.init          ( 63, &m_cfg[  0], 0, 16, 0);
    TotPtotLatency.init     ( 63, &m_cfg[  0], 0, 16, 0);
    //64
    PtotCoreColEn3.init     ( 64, &m_cfg[  0], 0, 16, 0);
    //65
    PtotCoreColEn2.init     ( 65, &m_cfg[  0], 0, 16, 0);
    //66
    PtotCoreColEn1.init     ( 66, &m_cfg[  0], 0, 16, 0);
    //67
    PtotCoreColEn0.init     ( 67, &m_cfg[  0], 0, 16, 0);
    //68
    DataMergeInPol.init     ( 68, &m_cfg[  0], 0, 16, 0);
    EnChipId.init           ( 68, &m_cfg[  0], 0, 16, 0);
    DataMergeSelClk.init    ( 68, &m_cfg[  0], 0, 16, 0);
    DataMergeEnClkGate.init ( 68, &m_cfg[  0], 0, 16, 0);
    DataMergeEn.init        ( 68, &m_cfg[  0], 0, 16, 0);
    DataMergeEnBond.init    ( 68, &m_cfg[  0], 0, 16, 0);
    //69
    DataMergeInMux3.init    ( 69, &m_cfg[  0], 0, 16, 0);
    DataMergeInMux2.init    ( 69, &m_cfg[  0], 0, 16, 0);
    DataMergeInMux1.init    ( 69, &m_cfg[  0], 0, 16, 0);
    DataMergeInMux0.init    ( 69, &m_cfg[  0], 0, 16, 0);
    DataMergeOutMux3.init   ( 69, &m_cfg[  0], 0, 16, 0);
    DataMergeOutMux2.init   ( 69, &m_cfg[  0], 0, 16, 0);
    DataMergeOutMux1.init   ( 69, &m_cfg[  0], 0, 16, 0);
    DataMergeOutMux0.init   ( 69, &m_cfg[  0], 0, 16, 0);
    //70-73
    EnCoreColCal3.init      ( 70, &m_cfg[  0], 0, 16, 0);
    EnCoreColCal2.init      ( 71, &m_cfg[  0], 0, 16, 0);
    EnCoreColCal1.init      ( 72, &m_cfg[  0], 0, 16, 0);
    EnCoreColCal0.init      ( 73, &m_cfg[  0], 0, 16, 0);
    //74
    DataEnBcid.init         ( 74, &m_cfg[  0], 0, 16, 0);
    DataEnL1id.init         ( 74, &m_cfg[  0], 0, 16, 0);
    DataEnEos.init          ( 74, &m_cfg[  0], 0, 16, 0);
    NumOfEventsInStream.init( 74, &m_cfg[  0], 0, 16, 0);
    //75
    DataEnBinaryRo.init     ( 75, &m_cfg[  0], 0, 16, 0);
    DataEnRaw.init          ( 75, &m_cfg[  0], 0, 16, 0);
    DataEnHitRemoval.init   ( 75, &m_cfg[  0], 0, 16, 0);
    DataMaxHits.init        ( 75, &m_cfg[  0], 0, 16, 0);
    DataEnIsoHitRemoval.init(  0, &m_cfg[  0], 0, 16, 0);
    DataMaxTot.init         ( 75, &m_cfg[  0], 0, 16, 0);
    //76
    EvenMask.init           ( 76, &m_cfg[  0], 0, 16, 0);
    //77
    OddMask.init            ( 77, &m_cfg[  0], 0, 16, 0);
    //78
    EfuseConfig.init        ( 78, &m_cfg[  0], 0, 16, 0);
    //79
    EfuseWriteData1.init    ( 79, &m_cfg[  0], 0, 16, 0);
    //80
    EfuseWriteData0.init    ( 80, &m_cfg[  0], 0, 16, 0);
    //81
    AuroraEnPrbs.init       ( 81, &m_cfg[  0], 0, 16, 0);
    AuroraActiveLanes.init  ( 81, &m_cfg[  0], 0, 16, 0);
    AuroraCCWait.init       ( 81, &m_cfg[  0], 0, 16, 0);
    AuroraCCSend.init       ( 81, &m_cfg[  0], 0, 16, 0);
    //82
    AuroraCBWait1.init      ( 82, &m_cfg[  0], 0, 16, 0);
    //83
    AuroraCBWait0.init      ( 83, &m_cfg[  0], 0, 16, 0);
    AuroraCBSend.init       ( 83, &m_cfg[  0], 0, 16, 0);
    //84
    AuroraInitWait.init     ( 84, &m_cfg[  0], 0, 16, 0);
    //85
    GpValReg.init           ( 85, &m_cfg[  0], 0, 16, 0);
    GpCmosEn.init           ( 85, &m_cfg[  0], 0, 16, 0);
    GpLvdsEn.init           ( 85, &m_cfg[  0], 0, 16, 0);
    GpLvdsBias.init         ( 85, &m_cfg[  0], 0, 16, 0);
    //86
    GpCmosRoute.init        ( 86, &m_cfg[  0], 0, 16, 0);
    //87
    GpLvdsPad3.init         ( 87, &m_cfg[  0], 0, 16, 0);
    GpLvdsPad2.init         ( 87, &m_cfg[  0], 0, 16, 0);
    //88
    GpLvdsPad1.init         (  88, &m_cfg[  0], 0, 16, 0);
    GpLvdsPad0.init         (  88, &m_cfg[  0], 0, 16, 0);
    //89
    CdrCp.init              ( 89, &m_cfg[  0], 0, 16, 0);
    //90
    CdrCpFd.init            ( 90, &m_cfg[  0], 0, 16, 0);
    //91
    CdrCpBuff.init          ( 91, &m_cfg[  0], 0, 16, 0);
    //92
    CdrVco.init             ( 92, &m_cfg[  0], 0, 16, 0);
    //93
    CdrVcoBuff.init         ( 93, &m_cfg[  0], 0, 16, 0);
    //94
    SerSelOut3.init         ( 94, &m_cfg[  0], 0, 16, 0);
    SerSelOut2.init         ( 94, &m_cfg[  0], 0, 16, 0);
    SerSelOut1.init         ( 94, &m_cfg[  0], 0, 16, 0);
    SerSelOut0.init         ( 94, &m_cfg[  0], 0, 16, 0);
    //95
    SerInvTap.init          ( 95, &m_cfg[  0], 0, 16, 0);
    SerEnTap.init           ( 95, &m_cfg[  0], 0, 16, 0);
    SerEnLane.init          ( 95, &m_cfg[  0], 0, 16, 0);
    //96
    CmlBias2.init           ( 96, &m_cfg[  0], 0, 16, 0);
    //97
    CmlBias1.init           ( 97, &m_cfg[  0], 0, 16, 0);
    //98
    CmlBias0.init           ( 98, &m_cfg[  0], 0, 16, 0);
    //99
    MonitorEnable.init      ( 99, &m_cfg[  0], 0, 16, 0);
    MonitorI.init           ( 99, &m_cfg[  0], 0, 16, 0);
    MonitorV.init           ( 99, &m_cfg[  0], 0, 16, 0);
    //100
    ErrWngMask.init         (100, &m_cfg[  0], 0, 16, 0);
    //101
    MonSensSldoDigEn.init   (101, &m_cfg[  0], 0, 16, 0);
    MonSensSldoDigDem.init  (101, &m_cfg[  0], 0, 16, 0);
    MonSensSldoDigSelBias.init(101, &m_cfg[  0], 0, 16, 0);
    MonSensSldoAnaEn.init   (101, &m_cfg[  0], 0, 16, 0);
    MonSensSldoAnaDem.init  (101, &m_cfg[  0], 0, 16, 0);
    MonSensSldoAnaSelBias.init(101, &m_cfg[  0], 0, 16, 0);
    //102
    MonSensAcbEn.init       (102, &m_cfg[  0], 0, 16, 0);
    MonSensAcbDem.init      (102, &m_cfg[  0], 0, 16, 0);
    MonSensAcbSelBias.init  (102, &m_cfg[  0], 0, 16, 0);
    //103
    VrefRsensBot.init       (103, &m_cfg[  0], 0, 16, 0);
    VrefRsensTop.init       (103, &m_cfg[  0], 0, 16, 0);
    VrefIn.init             (103, &m_cfg[  0], 0, 16, 0);
    MonAdcTrim.init         (103, &m_cfg[  0], 0, 16, 0);

    //104
    NtcDac.init             (104, &m_cfg[  0], 0, 16, 0);
    //105-108
    HitOrMask3.init         (105, &m_cfg[  0], 0, 16, 0);
    HitOrMask2.init         (106, &m_cfg[  0], 0, 16, 0);
    HitOrMask1.init         (107, &m_cfg[  0], 0, 16, 0);
    HitOrMask0.init         (108, &m_cfg[  0], 0, 16, 0);
    //109-116
    AutoRead0.init          (109, &m_cfg[  0], 0, 16, 0);
    AutoRead1.init          (110, &m_cfg[  0], 0, 16, 0);
    AutoRead2.init          (111, &m_cfg[  0], 0, 16, 0);
    AutoRead3.init          (112, &m_cfg[  0], 0, 16, 0);
    AutoRead4.init          (113, &m_cfg[  0], 0, 16, 0);
    AutoRead5.init          (114, &m_cfg[  0], 0, 16, 0);
    AutoRead6.init          (115, &m_cfg[  0], 0, 16, 0);
    AutoRead7.init          (116, &m_cfg[  0], 0, 16, 0);
    //117
    RingOscBClear.init      (117, &m_cfg[  0], 0, 16, 0);
    RingOscBEnBl.init       (117, &m_cfg[  0], 0, 16, 0);
    RingOscBEnCapA.init     (117, &m_cfg[  0], 0, 16, 0);
    RingOscBEnFf.init       (117, &m_cfg[  0], 0, 16, 0);
    RingOscBEnLvt.init      (117, &m_cfg[  0], 0, 16, 0);
    RingOscAClear.init      (117, &m_cfg[  0], 0, 16, 0);
    RingOscAEn.init         (117, &m_cfg[  0], 0, 16, 0);
    //118
    RingOscARoute.init      (118, &m_cfg[  0], 0, 16, 0);
    RingOscBRoute.init      (118, &m_cfg[  0], 0, 16, 0);
    //119-120
    RingOscAOut.init        (119, &m_cfg[  0], 0, 16, 0);
    RingOscBOut.init        (120, &m_cfg[  0], 0, 16, 0);
    //121-123 RO
    BcidCnt.init            (121, &m_cfg[  0], 0, 16, 0);
    TrigCnt.init            (122, &m_cfg[  0], 0, 16, 0);
    ReadTrigCnt.init        (123, &m_cfg[  0], 0, 16, 0);
    //124-128
    LockLossCnt.init        (124, &m_cfg[  0], 0, 16, 0);
    BitFLipWngCnt.init      (125, &m_cfg[  0], 0, 16, 0);
    BitFLipErrCnt.init      (126, &m_cfg[  0], 0, 16, 0);
    CmdErrCnt.init          (127, &m_cfg[  0], 0, 16, 0);
    RdWrFifoErrCnt.init     (128, &m_cfg[  0], 0, 16, 0);
    //129
    AiRegionRow.init        (129, &m_cfg[  0], 0, 16, 0);
    //130-133
    HitOrCnt3.init          (130, &m_cfg[  0], 0, 16, 0);
    HitOrCnt2.init          (131, &m_cfg[  0], 0, 16, 0);
    HitOrCnt1.init          (132, &m_cfg[  0], 0, 16, 0);
    HitOrCnt0.init          (133, &m_cfg[  0], 0, 16, 0);
    //134
    SkippedTrigCnt.init     (134, &m_cfg[  0], 0, 16, 0);
    //135-136
    EfuseReadData0.init     (135, &m_cfg[  0], 0, 16, 0);
    EfuseReadData1.init     (136, &m_cfg[  0], 0, 16, 0);
    //137
    MonitoringDataAdc.init  (137, &m_cfg[  0], 0, 16, 0);

}
