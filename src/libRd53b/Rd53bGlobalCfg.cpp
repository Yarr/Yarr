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
    this->init();
}

Rd53bGlobalCfg::~Rd53bGlobalCfg() = default;

uint16_t Rd53bGlobalCfg::getValue(Rd53bRegDefault Rd53bGlobalCfg::*ref) const {
    return (this->*ref).read();
}

uint16_t Rd53bGlobalCfg::getValue(std::string name) const {
    if (regMap.find(name) != regMap.end()) {
        return (this->*regMap.at(name)).read();
    } else if (virtRegMap.find(name) != virtRegMap.end()) {
        return (this->*virtRegMap.at(name)).read();
    } else {
        logger->error("Register \"{}\" not found, could not read!", name);
    }
    return 0;
}

void Rd53bGlobalCfg::setValue(Rd53bRegDefault Rd53bGlobalCfg::*ref, uint16_t val) {
    (this->*ref).write(val);
}

void Rd53bGlobalCfg::setValue(std::string name, uint16_t val) {
    if (regMap.find(name) != regMap.end()) {
        (this->*regMap[name]).write(val);
    } else if (virtRegMap.find(name) != virtRegMap.end()) {
        (this->*virtRegMap[name]).write(val);
    } else {
        logger->error("Register \"{}\" not found, could not write!", name);
    }
}

uint16_t& Rd53bGlobalCfg::operator[](unsigned index) {
    if (index >= numRegs) {
        logger->critical("Trying to access config out of range!");
        exit(0);
    }
    return m_cfg[index];
}

void Rd53bGlobalCfg::init() {
    // Reset array
    for (unsigned i=0; i<numRegs; i++) {
        m_cfg[i] = 0x00;
    }

    //0
    PixPortal.init          (  0, &m_cfg[  0], 0, 16, 0); regMap["PixPortal"] = &Rd53bGlobalCfg::PixPortal;
    //1
    PixRegionCol.init       (  1, &m_cfg[  1], 0, 16, 0); regMap["PixRegionCol"] = &Rd53bGlobalCfg::PixRegionCol;
    //2
    PixRegionRow.init       (  2, &m_cfg[  2], 0, 16, 0); regMap["PixRegionRow"] = &Rd53bGlobalCfg::PixRegionRow;
    //3
    PixBroadcast.init       (  3, &m_cfg[  3], 2, 1, 0); regMap["PixBroadcast"] = &Rd53bGlobalCfg::PixBroadcast;
    PixConfMode.init        (  3, &m_cfg[  3], 1, 1, 1); regMap["PixConfMode"] = &Rd53bGlobalCfg::PixConfMode;
    PixAutoRow.init         (  3, &m_cfg[  3], 0, 1, 0); regMap["PixAutoRow"] = &Rd53bGlobalCfg::PixAutoRow;
    //4
    PixDefaultConfig.init   (  4, &m_cfg[  4], 0, 16, 0x9CE2); regMap["PixDefaultConfig"] = &Rd53bGlobalCfg::PixDefaultConfig;
    //5
    PixDefaultConfigB.init  (  5, &m_cfg[  5], 0, 16, 0x631D); regMap["PixDefaultConfigB"] = &Rd53bGlobalCfg::PixDefaultConfigB;
    //6
    GcrDefaultConfig.init   (  6, &m_cfg[  6], 0, 16, 0xAC75); regMap["GcrDefaultConfig"] = &Rd53bGlobalCfg::GcrDefaultConfig;
    //7
    GcrDefaultConfigB.init  (  7, &m_cfg[  7], 0, 16, 0x538A); regMap["GcrDefaultConfigB"] = &Rd53bGlobalCfg::GcrDefaultConfigB;

    // Diff AFE
    //8
    DiffPreampL.init        (  8, &m_cfg[  8], 0, 10, 800); regMap["DiffPreampL"] = &Rd53bGlobalCfg::DiffPreampL;
    //9
    DiffPreampR.init        (  9, &m_cfg[  9], 0, 10, 800); regMap["DiffPreampR"] = &Rd53bGlobalCfg::DiffPreampR;
    //10
    DiffPreampTL.init       ( 10, &m_cfg[ 10], 0, 10, 800); regMap["DiffPreampTL"] = &Rd53bGlobalCfg::DiffPreampTL;
    //11
    DiffPreampTR.init       ( 11, &m_cfg[ 11], 0, 10, 800); regMap["DiffPreampTR"] = &Rd53bGlobalCfg::DiffPreampTR;
    //12
    DiffPreampT.init        ( 12, &m_cfg[ 12], 0, 10, 800); regMap["DiffPreampT"] = &Rd53bGlobalCfg::DiffPreampT;
    //13
    DiffPreampM.init        ( 13, &m_cfg[ 13], 0, 10, 800); regMap["DiffPreampM"] = &Rd53bGlobalCfg::DiffPreampM;
    //14
    DiffPreComp.init        ( 14, &m_cfg[ 14], 0, 10, 350); regMap["DiffPreComp"] = &Rd53bGlobalCfg::DiffPreComp;
    //15
    DiffComp.init           ( 15, &m_cfg[ 15], 0, 10, 500); regMap["DiffComp"] = &Rd53bGlobalCfg::DiffComp;
    //16
    DiffVff.init            ( 16, &m_cfg[ 16], 0, 10, 160); regMap["DiffVff"] = &Rd53bGlobalCfg::DiffVff;
    //17
    DiffTh1L.init           ( 17, &m_cfg[ 17], 0, 10, 350); regMap["DiffTh1L"] = &Rd53bGlobalCfg::DiffTh1L;
    //18
    DiffTh1R.init           ( 18, &m_cfg[ 18], 0, 10, 350); regMap["DiffTh1R"] = &Rd53bGlobalCfg::DiffTh1R;
    //19
    DiffTh1M.init           ( 19, &m_cfg[ 19], 0, 10, 350); regMap["DiffTh1M"] = &Rd53bGlobalCfg::DiffTh1M;
    //20
    DiffTh2.init            ( 20, &m_cfg[ 20], 0, 10, 50); regMap["DiffTh2"] = &Rd53bGlobalCfg::DiffTh2;
    //21
    DiffLcc.init            ( 21, &m_cfg[ 21], 0, 10, 100); regMap["DiffLcc"] = &Rd53bGlobalCfg::DiffLcc;
    //37
    DiffLccEn.init          ( 37, &m_cfg[ 37], 1,  1, 0); regMap["DiffLccEn"] = &Rd53bGlobalCfg::DiffLccEn;
    DiffFbCapEn.init        ( 37, &m_cfg[ 37], 0,  1, 0); regMap["DiffFbCapEn"] = &Rd53bGlobalCfg::DiffFbCapEn;

    // Lin AFE
    //22
    LinPreampL.init         ( 22, &m_cfg[ 22], 0, 10, 300); regMap["LinPreampL"] = &Rd53bGlobalCfg::LinPreampL;
    //23
    LinPreampR.init         ( 23, &m_cfg[ 23], 0, 10, 300); regMap["LinPreampR"] = &Rd53bGlobalCfg::LinPreampR;
    //24
    LinPreampTL.init        ( 24, &m_cfg[ 24], 0, 10, 300); regMap["LinPreampTL"] = &Rd53bGlobalCfg::LinPreampTL;
    //25
    LinPreampTR.init        ( 25, &m_cfg[ 25], 0, 10, 300); regMap["LinPreampTR"] = &Rd53bGlobalCfg::LinPreampTR;
    //26
    LinPreampT.init         ( 26, &m_cfg[ 26], 0, 10, 300); regMap["LinPreampT"] = &Rd53bGlobalCfg::LinPreampT;
    //27
    LinPreampM.init         ( 27, &m_cfg[ 27], 0, 10, 300); regMap["LinPreampM"] = &Rd53bGlobalCfg::LinPreampM;
    //28
    LinFc.init              ( 28, &m_cfg[ 28], 0, 10, 20); regMap["LinFc"] = &Rd53bGlobalCfg::LinFc;
    //29
    LinKrumCurr.init        ( 29, &m_cfg[ 29], 0, 10, 50); regMap["LinKrumCurr"] = &Rd53bGlobalCfg::LinKrumCurr;
    //30
    LinRefKrum.init         ( 30, &m_cfg[ 30], 0, 10, 300); regMap["LinRefKrum"] = &Rd53bGlobalCfg::LinRefKrum;
    //31
    LinComp.init            ( 31, &m_cfg[ 31], 0, 10, 110); regMap["LinComp"] = &Rd53bGlobalCfg::LinComp;
    //32
    LinCompTa.init          ( 32, &m_cfg[ 32], 0, 10, 110); regMap["LinCompTa"] = &Rd53bGlobalCfg::LinCompTa;
    //33
    LinGdacL.init           ( 33, &m_cfg[ 33], 0, 10, 408); regMap["LinGdacL"] = &Rd53bGlobalCfg::LinGdacL;
    //34
    LinGdacR.init           ( 34, &m_cfg[ 34], 0, 10, 408); regMap["LinGdacR"] = &Rd53bGlobalCfg::LinGdacR;
    //35
    LinGdacM.init           ( 35, &m_cfg[ 35], 0, 10, 408); regMap["LinGdacM"] = &Rd53bGlobalCfg::LinGdacM;
    //36
    LinLdac.init            ( 36, &m_cfg[ 36], 0, 10, 100); regMap["LinLdac"] = &Rd53bGlobalCfg::LinLdac;

    // Power
    //38
    SldoEnUndershuntA.init  ( 38, &m_cfg[ 38], 9,  1, 0); regMap["SldoEnUndershuntA"] = &Rd53bGlobalCfg::SldoEnUndershuntA;
    SldoEnUndershuntD.init  ( 38, &m_cfg[ 38], 8,  1, 0); regMap["SldoEnUndershuntD"] = &Rd53bGlobalCfg::SldoEnUndershuntD;
    SldoTrimA.init          ( 38, &m_cfg[ 38], 4,  4, 8); regMap["SldoTrimA"] = &Rd53bGlobalCfg::SldoTrimA;
    SldoTrimD.init          ( 38, &m_cfg[ 38], 0,  4, 8); regMap["SldoTrimD"] = &Rd53bGlobalCfg::SldoTrimD;

    // Pixel Matrix
    //39
    EnCoreCol3.init         ( 39, &m_cfg[ 39], 0,  6, 63); regMap["EnCoreCol3"] = &Rd53bGlobalCfg::EnCoreCol3;
    //40
    EnCoreCol2.init         ( 40, &m_cfg[ 40], 0, 16, 65535); regMap["EnCoreCol2"] = &Rd53bGlobalCfg::EnCoreCol2;
    //41
    EnCoreCol1.init         ( 41, &m_cfg[ 41], 0, 16, 65535); regMap["EnCoreCol1"] = &Rd53bGlobalCfg::EnCoreCol1;
    //42
    EnCoreCol0.init         ( 42, &m_cfg[ 42], 0, 16, 65535); regMap["EnCoreCol0"] = &Rd53bGlobalCfg::EnCoreCol0;
    //43
    RstCoreCol3.init        ( 43, &m_cfg[ 43], 0,  6, 0); regMap["RstCoreCol3"] = &Rd53bGlobalCfg::RstCoreCol3;
    //44
    RstCoreCol2.init        ( 44, &m_cfg[ 44], 0, 16, 0); regMap["RstCoreCol2"] = &Rd53bGlobalCfg::RstCoreCol2;
    //45
    RstCoreCol1.init        ( 45, &m_cfg[ 45], 0, 16, 0); regMap["RstCoreCol1"] = &Rd53bGlobalCfg::RstCoreCol1;
    //46
    RstCoreCol0.init        ( 46, &m_cfg[ 46], 0, 16, 0); regMap["RstCoreCol0"] = &Rd53bGlobalCfg::RstCoreCol0;

    // Digital functions
    //47
    TwoLevelTrig.init       ( 47, &m_cfg[ 47], 9,  1, 0); regMap["TwoLevelTrig"] = &Rd53bGlobalCfg::TwoLevelTrig;
    Latency.init            ( 47, &m_cfg[ 47], 0,  9, 500); regMap["Latency"] = &Rd53bGlobalCfg::Latency;
    //48
    SelfTrigEn.init         ( 48, &m_cfg[ 48], 5,  1, 0); regMap["SelfTrigEn"] = &Rd53bGlobalCfg::SelfTrigEn;
    SelfTrigDigThrEn.init   ( 48, &m_cfg[ 48], 4,  1, 0); regMap["SelfTrigDigThrEn"] = &Rd53bGlobalCfg::SelfTrigDigThrEn;
    SelfTrigDigThr.init     ( 48, &m_cfg[ 48], 0,  4, 1); regMap["SelfTrigDigThr"] = &Rd53bGlobalCfg::SelfTrigDigThr;
    //49
    SelfTrigDelay.init      ( 49, &m_cfg[ 49], 5, 10, 512); regMap["SelfTrigDelay"] = &Rd53bGlobalCfg::SelfTrigDelay;
    SelfTrigMulti.init      ( 49, &m_cfg[ 49], 0, 5, 4); regMap["SelfTrigMulti"] = &Rd53bGlobalCfg::SelfTrigMulti;
    //50
    SelfTrigPattern.init    ( 50, &m_cfg[ 50], 0, 16, 65534); regMap["SelfTrigPattern"] = &Rd53bGlobalCfg::SelfTrigPattern;
    //51
    DataReadDelay.init      ( 51, &m_cfg[ 51], 12,  2, 0); regMap["DataReadDelay"] = &Rd53bGlobalCfg::DataReadDelay;
    ReadTrigLatency.init    ( 51, &m_cfg[ 51], 0, 12, 1000); regMap["ReadTrigLatency"] = &Rd53bGlobalCfg::ReadTrigLatency;
    //52
    TruncTimeoutConf.init   ( 52, &m_cfg[ 52], 0, 12, 0); regMap["TruncTimeoutConf"] = &Rd53bGlobalCfg::TruncTimeoutConf;
    //53
    InjDigEn.init           ( 53, &m_cfg[ 53], 7,  1, 0); regMap["InjDigEn"] = &Rd53bGlobalCfg::InjDigEn;
    InjAnaMode.init         ( 53, &m_cfg[ 53], 6,  1, 0); regMap["InjAnaMode"] = &Rd53bGlobalCfg::InjAnaMode;
    InjFineDelay.init       ( 53, &m_cfg[ 53], 0,  6, 5); regMap["InjFineDelay"] = &Rd53bGlobalCfg::InjFineDelay;
    //54
    FineDelayClk.init       ( 54, &m_cfg[ 54], 6,  6, 0); regMap["FineDelayClk"] = &Rd53bGlobalCfg::FineDelayClk;
    FineDelayData.init      ( 54, &m_cfg[ 54], 0,  6, 0); regMap["FineDelayData"] = &Rd53bGlobalCfg::FineDelayData;
    //55
    InjVcalHigh.init        ( 55, &m_cfg[ 55], 0, 12, 200); regMap["InjVcalHigh"] = &Rd53bGlobalCfg::InjVcalHigh;
    //56
    InjVcalMed.init         ( 56, &m_cfg[ 56], 0, 12, 200); regMap["InjVcalMed"] = &Rd53bGlobalCfg::InjVcalMed;
    //57
    CapMeasEnPar.init       ( 57, &m_cfg[ 57], 2,  1, 0); regMap["CapMeasEnPar"] = &Rd53bGlobalCfg::CapMeasEnPar;
    CapMeasEn.init          ( 57, &m_cfg[ 57], 1,  1, 0); regMap["CapMeasEn"] = &Rd53bGlobalCfg::CapMeasEn;
    InjVcalRange.init       ( 57, &m_cfg[ 57], 0,  1, 1); regMap["InjVcalRange"] = &Rd53bGlobalCfg::InjVcalRange;
    //58
    CdrOverwriteLimit.init  ( 58, &m_cfg[ 58], 4,  1, 0); regMap["CdrOverwriteLimit"] = &Rd53bGlobalCfg::CdrOverwriteLimit;
    CdrPhaseDetSel.init     ( 58, &m_cfg[ 58], 3,  1, 0); regMap["CdrPhaseDetSel"] = &Rd53bGlobalCfg::CdrPhaseDetSel;
    CdrClkSel.init          ( 58, &m_cfg[ 58], 0,  3, 0); regMap["CdrClkSel"] = &Rd53bGlobalCfg::CdrClkSel;
    //59
    ChSyncLockThr.init      ( 59, &m_cfg[ 59], 0,  5, 31); regMap["ChSyncLockThr"] = &Rd53bGlobalCfg::ChSyncLockThr;
    //60
    GlobalPulseConf.init    ( 60, &m_cfg[ 60], 0, 16, 0); regMap["GlobalPulseConf"] = &Rd53bGlobalCfg::GlobalPulseConf;
    //61
    GlobalPulseWidth.init   ( 61, &m_cfg[ 61], 0,  8, 0); regMap["GlobalPulseWidth"] = &Rd53bGlobalCfg::GlobalPulseWidth;
    //62
    ServiceBlockEn.init     ( 62, &m_cfg[ 62], 8,  1, 1); regMap["ServiceBlockEn"] = &Rd53bGlobalCfg::ServiceBlockEn;
    ServiceBlockPeriod.init ( 62, &m_cfg[ 62], 0,  8, 50); regMap["ServiceBlockPeriod"] = &Rd53bGlobalCfg::ServiceBlockPeriod;
    //63
    TotEnPtot.init          ( 63, &m_cfg[ 63], 12,  1, 0); regMap["TotEnPtot"] = &Rd53bGlobalCfg::TotEnPtot;
    TotEnPtoa.init          ( 63, &m_cfg[ 63], 11,  1, 0); regMap["TotEnPtoa"] = &Rd53bGlobalCfg::TotEnPtoa;
    TotEn80.init            ( 63, &m_cfg[ 63], 10,  1, 0); regMap["TotEn80"] = &Rd53bGlobalCfg::TotEn80;
    TotEn6b4b.init          ( 63, &m_cfg[ 63], 9,  1, 0); regMap["TotEn6b4b"] = &Rd53bGlobalCfg::TotEn6b4b;
    TotPtotLatency.init     ( 63, &m_cfg[ 63], 0,  9, 500); regMap["TotPtotLatency"] = &Rd53bGlobalCfg::TotPtotLatency;
    //64
    PtotCoreColEn3.init     ( 64, &m_cfg[ 64], 0,  6, 0); regMap["PtotCoreColEn3"] = &Rd53bGlobalCfg::PtotCoreColEn3;
    //65
    PtotCoreColEn2.init     ( 65, &m_cfg[ 65], 0, 16, 0); regMap["PtotCoreColEn2"] = &Rd53bGlobalCfg::PtotCoreColEn2;
    //66
    PtotCoreColEn1.init     ( 66, &m_cfg[ 66], 0, 16, 0); regMap["PtotCoreColEn1"] = &Rd53bGlobalCfg::PtotCoreColEn1;
    //67
    PtotCoreColEn0.init     ( 67, &m_cfg[ 67], 0, 16, 0); regMap["PtotCoreColEn0"] = &Rd53bGlobalCfg::PtotCoreColEn0;
    //68
    DataMergeInPol.init     ( 68, &m_cfg[ 68], 8,  4, 0); regMap["DataMergeInPol"] = &Rd53bGlobalCfg::DataMergeInPol;
    EnChipId.init           ( 68, &m_cfg[ 68], 7,  1, 0); regMap["EnChipId"] = &Rd53bGlobalCfg::EnChipId;
    DataMergeEnClkGate.init ( 68, &m_cfg[ 68], 6,  1, 0); regMap["DataMergeEnClkGate"] = &Rd53bGlobalCfg::DataMergeEnClkGate;
    DataMergeSelClk.init    ( 68, &m_cfg[ 68], 5,  1, 0); regMap["DataMergeSelClk"] = &Rd53bGlobalCfg::DataMergeSelClk;
    DataMergeEn.init        ( 68, &m_cfg[ 68], 1,  4, 0); regMap["DataMergeEn"] = &Rd53bGlobalCfg::DataMergeEn;
    DataMergeEnBond.init    ( 68, &m_cfg[ 68], 0,  1, 0); regMap["DataMergeEnBond"] = &Rd53bGlobalCfg::DataMergeEnBond;
    //69
    DataMergeInMux3.init    ( 69, &m_cfg[ 69], 14,  2, 3); regMap["DataMergeInMux3"] = &Rd53bGlobalCfg::DataMergeInMux3;
    DataMergeInMux2.init    ( 69, &m_cfg[ 69], 12,  2, 2); regMap["DataMergeInMux2"] = &Rd53bGlobalCfg::DataMergeInMux2;
    DataMergeInMux1.init    ( 69, &m_cfg[ 69], 10,  2, 1); regMap["DataMergeInMux1"] = &Rd53bGlobalCfg::DataMergeInMux1;
    DataMergeInMux0.init    ( 69, &m_cfg[ 69], 8,  2, 0); regMap["DataMergeInMux0"] = &Rd53bGlobalCfg::DataMergeInMux0;
    DataMergeOutMux3.init   ( 69, &m_cfg[ 69], 6,  2, 0); regMap["DataMergeOutMux3"] = &Rd53bGlobalCfg::DataMergeOutMux3;
    DataMergeOutMux2.init   ( 69, &m_cfg[ 69], 4,  2, 1); regMap["DataMergeOutMux2"] = &Rd53bGlobalCfg::DataMergeOutMux2;
    DataMergeOutMux1.init   ( 69, &m_cfg[ 69], 2,  2, 2); regMap["DataMergeOutMux1"] = &Rd53bGlobalCfg::DataMergeOutMux1;
    DataMergeOutMux0.init   ( 69, &m_cfg[ 69], 0,  2, 3); regMap["DataMergeOutMux0"] = &Rd53bGlobalCfg::DataMergeOutMux0;
    //70-73
    EnCoreColCal3.init      ( 70, &m_cfg[ 70], 0,  6, 0); regMap["EnCoreColCal3"] = &Rd53bGlobalCfg::EnCoreColCal3;
    EnCoreColCal2.init      ( 71, &m_cfg[ 71], 0, 16, 0); regMap["EnCoreColCal2"] = &Rd53bGlobalCfg::EnCoreColCal2;
    EnCoreColCal1.init      ( 72, &m_cfg[ 72], 0, 16, 0); regMap["EnCoreColCal1"] = &Rd53bGlobalCfg::EnCoreColCal1;
    EnCoreColCal0.init      ( 73, &m_cfg[ 73], 0, 16, 0); regMap["EnCoreColCal0"] = &Rd53bGlobalCfg::EnCoreColCal0;
    //74
    DataEnBcid.init         ( 74, &m_cfg[ 74], 10,  1, 0); regMap["DataEnBcid"] = &Rd53bGlobalCfg::DataEnBcid;
    DataEnL1id.init         ( 74, &m_cfg[ 74], 9,  1, 0); regMap["DataEnL1id"] = &Rd53bGlobalCfg::DataEnL1id;
    DataEnEos.init          ( 74, &m_cfg[ 74], 8,  1, 1); regMap["DataEnEos"] = &Rd53bGlobalCfg::DataEnEos;
    NumOfEventsInStream.init( 74, &m_cfg[ 74], 0,  8, 1); regMap["NumOfEventsInStream"] = &Rd53bGlobalCfg::NumOfEventsInStream;
    //75
    DataEnBinaryRo.init     ( 75, &m_cfg[ 75], 10,  1, 0); regMap["DataEnBinaryRo"] = &Rd53bGlobalCfg::DataEnBinaryRo;
    DataEnRaw.init          ( 75, &m_cfg[ 75], 9,  1, 0); regMap["DataEnRaw"] = &Rd53bGlobalCfg::DataEnRaw;
    DataEnHitRemoval.init   ( 75, &m_cfg[ 75], 8,  1, 0); regMap["DataEnHitRemoval"] = &Rd53bGlobalCfg::DataEnHitRemoval;
    DataMaxHits.init        ( 75, &m_cfg[ 75], 4,  4, 0); regMap["DataMaxHits"] = &Rd53bGlobalCfg::DataMaxHits;
    DataEnIsoHitRemoval.init( 75, &m_cfg[ 75], 3,  1, 0); regMap["DataEnIsoHitRemoval"] = &Rd53bGlobalCfg::DataEnIsoHitRemoval;
    DataMaxTot.init         ( 75, &m_cfg[ 75], 0,  3, 0); regMap["DataMaxTot"] = &Rd53bGlobalCfg::DataMaxTot;
    //76
    EvenMask.init           ( 76, &m_cfg[ 76], 0, 16, 0); regMap["EvenMask"] = &Rd53bGlobalCfg::EvenMask;
    //77
    OddMask.init            ( 77, &m_cfg[ 77], 0, 16, 0); regMap["OddMask"] = &Rd53bGlobalCfg::OddMask;
    //78
    EfuseConfig.init        ( 78, &m_cfg[ 78], 0, 16, 0); regMap["EfuseConfig"] = &Rd53bGlobalCfg::EfuseConfig;
    //79
    EfuseWriteData1.init    ( 79, &m_cfg[ 79], 0, 16, 0); regMap["EfuseWriteData1"] = &Rd53bGlobalCfg::EfuseWriteData1;
    //80
    EfuseWriteData0.init    ( 80, &m_cfg[ 80], 0, 16, 0); regMap["EfuseWriteData0"] = &Rd53bGlobalCfg::EfuseWriteData0;
    //81
    AuroraEnPrbs.init       ( 81, &m_cfg[ 81], 12,  1, 0); regMap["AuroraEnPrbs"] = &Rd53bGlobalCfg::AuroraEnPrbs;
    AuroraActiveLanes.init  ( 81, &m_cfg[ 81], 8,  4, 15); regMap["AuroraActiveLanes"] = &Rd53bGlobalCfg::AuroraActiveLanes;
    AuroraCCWait.init       ( 81, &m_cfg[ 81], 2,  6, 25); regMap["AuroraCCWait"] = &Rd53bGlobalCfg::AuroraCCWait;
    AuroraCCSend.init       ( 81, &m_cfg[ 81], 0,  2, 3); regMap["AuroraCCSend"] = &Rd53bGlobalCfg::AuroraCCSend;
    //82
    AuroraCBWait1.init      ( 82, &m_cfg[ 82], 0,  8, 0); regMap["AuroraCBWait1"] = &Rd53bGlobalCfg::AuroraCBWait1;
    //83
    AuroraCBWait0.init      ( 83, &m_cfg[ 83], 4, 12, 4095); regMap["AuroraCBWait0"] = &Rd53bGlobalCfg::AuroraCBWait0;
    AuroraCBSend.init       ( 83, &m_cfg[ 83], 0,  4, 0); regMap["AuroraCBSend"] = &Rd53bGlobalCfg::AuroraCBSend;
    //84
    AuroraInitWait.init     ( 84, &m_cfg[ 84], 0, 11, 32); regMap["AuroraInitWait"] = &Rd53bGlobalCfg::AuroraInitWait;
    //85
    GpValReg.init           ( 85, &m_cfg[ 85], 9,  4, 5); regMap["GpValReg"] = &Rd53bGlobalCfg::GpValReg;
    GpCmosEn.init           ( 85, &m_cfg[ 85], 8,  1, 1); regMap["GpCmosEn"] = &Rd53bGlobalCfg::GpCmosEn;
    GpCmosDs.init           ( 85, &m_cfg[ 85], 7,  1, 0); regMap["GpCmosDs"] = &Rd53bGlobalCfg::GpCmosDs;
    GpLvdsEn.init           ( 85, &m_cfg[ 85], 3,  4, 0xF); regMap["GpLvdsEn"] = &Rd53bGlobalCfg::GpLvdsEn;
    GpLvdsBias.init         ( 85, &m_cfg[ 85], 0,  3, 7); regMap["GpLvdsBias"] = &Rd53bGlobalCfg::GpLvdsBias;
    //86
    GpCmosRoute.init        ( 86, &m_cfg[ 86], 0,  7, 34); regMap["GpCmosRoute"] = &Rd53bGlobalCfg::GpCmosRoute;
    //87
    GpLvdsPad3.init         ( 87, &m_cfg[ 87], 6,  6, 35); regMap["GpLvdsPad3"] = &Rd53bGlobalCfg::GpLvdsPad3;
    GpLvdsPad2.init         ( 87, &m_cfg[ 87], 0,  6, 33); regMap["GpLvdsPad2"] = &Rd53bGlobalCfg::GpLvdsPad2;
    //88
    GpLvdsPad1.init         ( 88, &m_cfg[ 88], 6,  6, 1); regMap["GpLvdsPad1"] = &Rd53bGlobalCfg::GpLvdsPad1;
    GpLvdsPad0.init         ( 88, &m_cfg[ 88], 0,  6, 0); regMap["GpLvdsPad0"] = &Rd53bGlobalCfg::GpLvdsPad0;
    //89
    CdrCp.init              ( 89, &m_cfg[ 89], 0, 10, 40); regMap["CdrCp"] = &Rd53bGlobalCfg::CdrCp;
    //90
    CdrCpFd.init            ( 90, &m_cfg[ 90], 0, 10, 400); regMap["CdrCpFd"] = &Rd53bGlobalCfg::CdrCpFd;
    //91
    CdrCpBuff.init          ( 91, &m_cfg[ 91], 0, 10, 200); regMap["CdrCpBuff"] = &Rd53bGlobalCfg::CdrCpBuff;
    //92
    CdrVco.init             ( 92, &m_cfg[ 92], 0, 10, 1023); regMap["CdrVco"] = &Rd53bGlobalCfg::CdrVco;
    //93
    CdrVcoBuff.init         ( 93, &m_cfg[ 93], 0, 10, 500); regMap["CdrVcoBuff"] = &Rd53bGlobalCfg::CdrVcoBuff;
    //94
    SerSelOut3.init         ( 94, &m_cfg[ 94], 6,  2, 1); regMap["SerSelOut3"] = &Rd53bGlobalCfg::SerSelOut3;
    SerSelOut2.init         ( 94, &m_cfg[ 94], 4,  2, 1); regMap["SerSelOut2"] = &Rd53bGlobalCfg::SerSelOut2;
    SerSelOut1.init         ( 94, &m_cfg[ 94], 2,  2, 1); regMap["SerSelOut1"] = &Rd53bGlobalCfg::SerSelOut1;
    SerSelOut0.init         ( 94, &m_cfg[ 94], 0,  2, 1); regMap["SerSelOut0"] = &Rd53bGlobalCfg::SerSelOut0;
    //95
    SerInvTap.init          ( 95, &m_cfg[ 95], 6,  2, 0); regMap["SerInvTap"] = &Rd53bGlobalCfg::SerInvTap;
    SerEnTap.init           ( 95, &m_cfg[ 95], 4,  2, 0); regMap["SerEnTap"] = &Rd53bGlobalCfg::SerEnTap;
    SerEnLane.init          ( 95, &m_cfg[ 95], 0,  4, 15); regMap["SerEnLane"] = &Rd53bGlobalCfg::SerEnLane;
    //96
    CmlBias2.init           ( 96, &m_cfg[ 96], 0, 10, 0); regMap["CmlBias2"] = &Rd53bGlobalCfg::CmlBias2;
    //97
    CmlBias1.init           ( 97, &m_cfg[ 97], 0, 10, 0); regMap["CmlBias1"] = &Rd53bGlobalCfg::CmlBias1;
    //98
    CmlBias0.init           ( 98, &m_cfg[ 98], 0, 10, 500); regMap["CmlBias0"] = &Rd53bGlobalCfg::CmlBias0;
    //99
    MonitorEnable.init      ( 99, &m_cfg[ 99], 12,  1, 0); regMap["MonitorEnable"] = &Rd53bGlobalCfg::MonitorEnable;
    MonitorI.init           ( 99, &m_cfg[ 99], 6,  6, 63); regMap["MonitorI"] = &Rd53bGlobalCfg::MonitorI;
    MonitorV.init           ( 99, &m_cfg[ 99], 0,  6, 63); regMap["MonitorV"] = &Rd53bGlobalCfg::MonitorV;
    //100
    ErrWngMask.init         (100, &m_cfg[100], 0,  8, 0); regMap["ErrWngMask"] = &Rd53bGlobalCfg::ErrWngMask;
    //101
    MonSensSldoDigEn.init   (101, &m_cfg[101], 11,  1, 0); regMap["MonSensSldoDigEn"] = &Rd53bGlobalCfg::MonSensSldoDigEn;
    MonSensSldoDigDem.init  (101, &m_cfg[101], 7,  4, 0); regMap["MonSensSldoDigDem"] = &Rd53bGlobalCfg::MonSensSldoDigDem;
    MonSensSldoDigSelBias.init(101, &m_cfg[101], 6,  1, 0); regMap["MonSensSldoDigSelBias"] = &Rd53bGlobalCfg::MonSensSldoDigSelBias;
    MonSensSldoAnaEn.init   (101, &m_cfg[101], 5,  1, 0); regMap["MonSensSldoAnaEn"] = &Rd53bGlobalCfg::MonSensSldoAnaEn;
    MonSensSldoAnaDem.init  (101, &m_cfg[101], 1,  4, 0); regMap["MonSensSldoAnaDem"] = &Rd53bGlobalCfg::MonSensSldoAnaDem;
    MonSensSldoAnaSelBias.init(101, &m_cfg[101], 0,  1, 0); regMap["MonSensSldoAnaSelBias"] = &Rd53bGlobalCfg::MonSensSldoAnaSelBias;
    //102
    MonSensAcbEn.init       (102, &m_cfg[102], 5,  1, 0); regMap["MonSensAcbEn"] = &Rd53bGlobalCfg::MonSensAcbEn;
    MonSensAcbDem.init      (102, &m_cfg[102], 1,  4, 0); regMap["MonSensAcbDem"] = &Rd53bGlobalCfg::MonSensAcbDem;
    MonSensAcbSelBias.init  (102, &m_cfg[102], 0,  1, 0); regMap["MonSensAcbSelBias"] = &Rd53bGlobalCfg::MonSensAcbSelBias;
    //103
    VrefRsensBot.init       (103, &m_cfg[103], 8,  1, 0); regMap["VrefRsensBot"] = &Rd53bGlobalCfg::VrefRsensBot;
    VrefRsensTop.init       (103, &m_cfg[103], 7,  1, 0); regMap["VrefRsensTop"] = &Rd53bGlobalCfg::VrefRsensTop;
    VrefIn.init             (103, &m_cfg[103], 6,  1, 1); regMap["VrefIn"] = &Rd53bGlobalCfg::VrefIn;
    MonAdcTrim.init         (103, &m_cfg[103], 0,  6, 0); regMap["MonAdcTrim"] = &Rd53bGlobalCfg::MonAdcTrim;

    //104
    NtcDac.init             (104, &m_cfg[104], 0, 10, 100); regMap["NtcDac"] = &Rd53bGlobalCfg::NtcDac;
    //105-108
    HitOrMask3.init         (105, &m_cfg[105], 0,  6, 0); regMap["HitOrMask3"] = &Rd53bGlobalCfg::HitOrMask3;
    HitOrMask2.init         (106, &m_cfg[106], 0, 16, 0); regMap["HitOrMask2"] = &Rd53bGlobalCfg::HitOrMask2;
    HitOrMask1.init         (107, &m_cfg[107], 0, 16, 0); regMap["HitOrMask1"] = &Rd53bGlobalCfg::HitOrMask1;
    HitOrMask0.init         (108, &m_cfg[108], 0, 16, 0); regMap["HitOrMask0"] = &Rd53bGlobalCfg::HitOrMask0;
    //109-116
    AutoRead0.init          (109, &m_cfg[109], 0,  9, 137); regMap["AutoRead0"] = &Rd53bGlobalCfg::AutoRead0;
    AutoRead1.init          (110, &m_cfg[110], 0,  9, 133); regMap["AutoRead1"] = &Rd53bGlobalCfg::AutoRead1;
    AutoRead2.init          (111, &m_cfg[111], 0,  9, 121); regMap["AutoRead2"] = &Rd53bGlobalCfg::AutoRead2;
    AutoRead3.init          (112, &m_cfg[112], 0,  9, 122); regMap["AutoRead3"] = &Rd53bGlobalCfg::AutoRead3;
    AutoRead4.init          (113, &m_cfg[113], 0,  9, 124); regMap["AutoRead4"] = &Rd53bGlobalCfg::AutoRead4;
    AutoRead5.init          (114, &m_cfg[114], 0,  9, 127); regMap["AutoRead5"] = &Rd53bGlobalCfg::AutoRead5;
    AutoRead6.init          (115, &m_cfg[115], 0,  9, 126); regMap["AutoRead6"] = &Rd53bGlobalCfg::AutoRead6;
    AutoRead7.init          (116, &m_cfg[116], 0,  9, 125); regMap["AutoRead7"] = &Rd53bGlobalCfg::AutoRead7;
    //117
    RingOscBClear.init      (117, &m_cfg[117], 14,  1, 0); regMap["RingOscBClear"] = &Rd53bGlobalCfg::RingOscBClear;
    RingOscBEnBl.init       (117, &m_cfg[117], 13,  1, 0); regMap["RingOscBEnBl"] = &Rd53bGlobalCfg::RingOscBEnBl;
    RingOscBEnBr.init       (117, &m_cfg[117], 12,  1, 0); regMap["RingOscBEnBr"] = &Rd53bGlobalCfg::RingOscBEnBr;
    RingOscBEnCapA.init     (117, &m_cfg[117], 11,  1, 0); regMap["RingOscBEnCapA"] = &Rd53bGlobalCfg::RingOscBEnCapA;
    RingOscBEnFf.init       (117, &m_cfg[117], 10,  1, 0); regMap["RingOscBEnFf"] = &Rd53bGlobalCfg::RingOscBEnFf;
    RingOscBEnLvt.init      (117, &m_cfg[117], 9,  1, 0); regMap["RingOscBEnLvt"] = &Rd53bGlobalCfg::RingOscBEnLvt;
    RingOscAClear.init      (117, &m_cfg[117], 8,  1, 0); regMap["RingOscAClear"] = &Rd53bGlobalCfg::RingOscAClear;
    RingOscAEn.init         (117, &m_cfg[117], 0,  8, 0); regMap["RingOscAEn"] = &Rd53bGlobalCfg::RingOscAEn;
    //118
    RingOscARoute.init      (118, &m_cfg[118], 6,  3, 0); regMap["RingOscARoute"] = &Rd53bGlobalCfg::RingOscARoute;
    RingOscBRoute.init      (118, &m_cfg[118], 0,  6, 0); regMap["RingOscBRoute"] = &Rd53bGlobalCfg::RingOscBRoute;
    //119-120
    RingOscAOut.init        (119, &m_cfg[119], 0, 16, 0); regMap["RingOscAOut"] = &Rd53bGlobalCfg::RingOscAOut;
    RingOscBOut.init        (120, &m_cfg[120], 0, 16, 0); regMap["RingOscBOut"] = &Rd53bGlobalCfg::RingOscBOut;
    //121-123 RO
    BcidCnt.init            (121, &m_cfg[121], 0, 16, 0); regMap["BcidCnt"] = &Rd53bGlobalCfg::BcidCnt;
    TrigCnt.init            (122, &m_cfg[122], 0, 16, 0); regMap["TrigCnt"] = &Rd53bGlobalCfg::TrigCnt;
    ReadTrigCnt.init        (123, &m_cfg[123], 0, 16, 0); regMap["ReadTrigCnt"] = &Rd53bGlobalCfg::ReadTrigCnt;
    //124-128
    LockLossCnt.init        (124, &m_cfg[124], 0, 16, 0); regMap["LockLossCnt"] = &Rd53bGlobalCfg::LockLossCnt;
    BitFlipWngCnt.init      (125, &m_cfg[125], 0, 16, 0); regMap["BitFlipWngCnt"] = &Rd53bGlobalCfg::BitFlipWngCnt;
    BitFlipErrCnt.init      (126, &m_cfg[126], 0, 16, 0); regMap["BitFlipErrCnt"] = &Rd53bGlobalCfg::BitFlipErrCnt;
    CmdErrCnt.init          (127, &m_cfg[127], 0, 16, 0); regMap["CmdErrCnt"] = &Rd53bGlobalCfg::CmdErrCnt;
    RdWrFifoErrCnt.init     (128, &m_cfg[128], 0, 16, 0); regMap["RdWrFifoErrCnt"] = &Rd53bGlobalCfg::RdWrFifoErrCnt;
    //129
    AiRegionRow.init        (129, &m_cfg[129], 0,  9, 0); regMap["AiRegionRow"] = &Rd53bGlobalCfg::AiRegionRow;
    //130-133
    HitOrCnt3.init          (130, &m_cfg[130], 0, 16, 0); regMap["HitOrCnt3"] = &Rd53bGlobalCfg::HitOrCnt3;
    HitOrCnt2.init          (131, &m_cfg[131], 0, 16, 0); regMap["HitOrCnt2"] = &Rd53bGlobalCfg::HitOrCnt2;
    HitOrCnt1.init          (132, &m_cfg[132], 0, 16, 0); regMap["HitOrCnt1"] = &Rd53bGlobalCfg::HitOrCnt1;
    HitOrCnt0.init          (133, &m_cfg[133], 0, 16, 0); regMap["HitOrCnt0"] = &Rd53bGlobalCfg::HitOrCnt0;
    //134
    SkippedTrigCnt.init     (134, &m_cfg[134], 0, 16, 0); regMap["SkippedTrigCnt"] = &Rd53bGlobalCfg::SkippedTrigCnt;
    //135-136
    EfuseReadData1.init     (135, &m_cfg[135], 0, 16, 0); regMap["EfuseReadData1"] = &Rd53bGlobalCfg::EfuseReadData1;
    EfuseReadData0.init     (136, &m_cfg[136], 0, 16, 0); regMap["EfuseReadData0"] = &Rd53bGlobalCfg::EfuseReadData0;
    //137
    MonitoringDataAdc.init  (137, &m_cfg[137], 0, 12, 0); regMap["MonitoringDataAdc"] = &Rd53bGlobalCfg::MonitoringDataAdc;

    // Special virtual registers
    InjVcalDiff.init(&InjVcalMed, &InjVcalHigh, true); virtRegMap["InjVcalDiff"] = (Rd53bRegDefault Rd53bGlobalCfg::*)&Rd53bGlobalCfg::InjVcalDiff;
    DiffTh1.init({&DiffTh1M, &DiffTh1L, &DiffTh1R}); virtRegMap["DiffTh1"] = (Rd53bRegDefault Rd53bGlobalCfg::*) &Rd53bGlobalCfg::DiffTh1;
}

void Rd53bGlobalCfg::writeConfig(json &j) {
    for(auto it : regMap) {
        logger->debug("Writing reg: {}", it.first);
        j["RD53B"]["GlobalConfig"][it.first] = (this->*it.second).read();
    }    
}

void Rd53bGlobalCfg::loadConfig(json const &j) {
    for (auto it : regMap) {
        if (j.contains({"RD53B","GlobalConfig",it.first})) {
            (this->*it.second).write(j["RD53B"]["GlobalConfig"][it.first]);
        } else {
            logger->error("Could not find register \"{}\" using default!", it.first);
        }
    }
}
