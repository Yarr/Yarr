// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: ITkPixV2 library
// # Comment: ITkPixV2 global register
// # Date: Jul 2023
// ################################

#include "Itkpixv2GlobalCfg.h"

#include "logging.h"

namespace {
    auto logger = logging::make_log("Itkpixv2GlobalCfg");
}

Itkpixv2GlobalCfg::Itkpixv2GlobalCfg() {
    this->init();
}

Itkpixv2GlobalCfg::~Itkpixv2GlobalCfg() = default;

uint16_t Itkpixv2GlobalCfg::getValue(Itkpixv2RegDefault Itkpixv2GlobalCfg::*ref) const {
    return (this->*ref).read();
}

uint16_t Itkpixv2GlobalCfg::getValue(std::string name) const {
    if (regMap.find(name) != regMap.end()) {
        return (this->*regMap.at(name)).read();
    } else if (virtRegMap.find(name) != virtRegMap.end()) {
        return (this->*virtRegMap.at(name)).read();
    } else {
        logger->error("Register \"{}\" not found, could not read!", name);
    }
    return 0;
}

void Itkpixv2GlobalCfg::setValue(Itkpixv2RegDefault Itkpixv2GlobalCfg::*ref, uint16_t val) {
    (this->*ref).write(val);
}

void Itkpixv2GlobalCfg::setValue(std::string name, uint16_t val) {
    if (regMap.find(name) != regMap.end()) {
        (this->*regMap[name]).write(val);
    } else if (virtRegMap.find(name) != virtRegMap.end()) {
        (this->*virtRegMap[name]).write(val);
    } else {
        logger->error("Register \"{}\" not found, could not write!", name);
    }
}

uint16_t& Itkpixv2GlobalCfg::operator[](unsigned index) {
    if (index >= numRegs) {
        logger->critical("Trying to access config out of range!");
        exit(0);
    }
    return m_cfg[index];
}

void Itkpixv2GlobalCfg::init() {
    // Reset array
    for (unsigned i=0; i<numRegs; i++) {
        m_cfg[i] = 0x00;
    }

    //0
    PixPortal.init          (  0, &m_cfg[  0], 0, 16, 0); regMap["PixPortal"] = &Itkpixv2GlobalCfg::PixPortal;
    //1
    PixRegionCol.init       (  1, &m_cfg[  1], 0, 16, 0); regMap["PixRegionCol"] = &Itkpixv2GlobalCfg::PixRegionCol;
    //2
    PixRegionRow.init       (  2, &m_cfg[  2], 0, 16, 0); regMap["PixRegionRow"] = &Itkpixv2GlobalCfg::PixRegionRow;
    //3
    PixBroadcast.init       (  3, &m_cfg[  3], 2, 1, 0); regMap["PixBroadcast"] = &Itkpixv2GlobalCfg::PixBroadcast;
    PixConfMode.init        (  3, &m_cfg[  3], 1, 1, 1); regMap["PixConfMode"] = &Itkpixv2GlobalCfg::PixConfMode;
    PixAutoRow.init         (  3, &m_cfg[  3], 0, 1, 0); regMap["PixAutoRow"] = &Itkpixv2GlobalCfg::PixAutoRow;
    //4
    PixDefaultConfig.init   (  4, &m_cfg[  4], 0, 16, 0x9CE2); regMap["PixDefaultConfig"] = &Itkpixv2GlobalCfg::PixDefaultConfig;
    //5
    PixDefaultConfigB.init  (  5, &m_cfg[  5], 0, 16, 0x631D); regMap["PixDefaultConfigB"] = &Itkpixv2GlobalCfg::PixDefaultConfigB;
    //6
    GcrDefaultConfig.init   (  6, &m_cfg[  6], 0, 16, 0xAC75); regMap["GcrDefaultConfig"] = &Itkpixv2GlobalCfg::GcrDefaultConfig;
    //7
    GcrDefaultConfigB.init  (  7, &m_cfg[  7], 0, 16, 0x538A); regMap["GcrDefaultConfigB"] = &Itkpixv2GlobalCfg::GcrDefaultConfigB;

    // Diff AFE
    //8
    DiffPreampL.init        (  8, &m_cfg[  8], 0, 10, 800); regMap["DiffPreampL"] = &Itkpixv2GlobalCfg::DiffPreampL;
    //9
    DiffPreampR.init        (  9, &m_cfg[  9], 0, 10, 800); regMap["DiffPreampR"] = &Itkpixv2GlobalCfg::DiffPreampR;
    //10
    DiffPreampTL.init       ( 10, &m_cfg[ 10], 0, 10, 800); regMap["DiffPreampTL"] = &Itkpixv2GlobalCfg::DiffPreampTL;
    //11
    DiffPreampTR.init       ( 11, &m_cfg[ 11], 0, 10, 800); regMap["DiffPreampTR"] = &Itkpixv2GlobalCfg::DiffPreampTR;
    //12
    DiffPreampT.init        ( 12, &m_cfg[ 12], 0, 10, 800); regMap["DiffPreampT"] = &Itkpixv2GlobalCfg::DiffPreampT;
    //13
    DiffPreampM.init        ( 13, &m_cfg[ 13], 0, 10, 800); regMap["DiffPreampM"] = &Itkpixv2GlobalCfg::DiffPreampM;
    //14
    DiffPreComp.init        ( 14, &m_cfg[ 14], 0, 10, 350); regMap["DiffPreComp"] = &Itkpixv2GlobalCfg::DiffPreComp;
    //15
    DiffComp.init           ( 15, &m_cfg[ 15], 0, 10, 500); regMap["DiffComp"] = &Itkpixv2GlobalCfg::DiffComp;
    //16
    DiffVff.init            ( 16, &m_cfg[ 16], 0, 10, 160); regMap["DiffVff"] = &Itkpixv2GlobalCfg::DiffVff;
    //17
    DiffTh1L.init           ( 17, &m_cfg[ 17], 0, 10, 350); regMap["DiffTh1L"] = &Itkpixv2GlobalCfg::DiffTh1L;
    //18
    DiffTh1R.init           ( 18, &m_cfg[ 18], 0, 10, 350); regMap["DiffTh1R"] = &Itkpixv2GlobalCfg::DiffTh1R;
    //19
    DiffTh1M.init           ( 19, &m_cfg[ 19], 0, 10, 350); regMap["DiffTh1M"] = &Itkpixv2GlobalCfg::DiffTh1M;
    //20
    DiffTh2.init            ( 20, &m_cfg[ 20], 0, 10, 50); regMap["DiffTh2"] = &Itkpixv2GlobalCfg::DiffTh2;
    //21
    DiffLcc.init            ( 21, &m_cfg[ 21], 0, 10, 100); regMap["DiffLcc"] = &Itkpixv2GlobalCfg::DiffLcc;
    //37
    DiffLccEn.init          ( 37, &m_cfg[ 37], 1,  1, 0); regMap["DiffLccEn"] = &Itkpixv2GlobalCfg::DiffLccEn;
    DiffFbCapEn.init        ( 37, &m_cfg[ 37], 0,  1, 0); regMap["DiffFbCapEn"] = &Itkpixv2GlobalCfg::DiffFbCapEn;

    // Lin AFE
    //22
    LinPreampL.init         ( 22, &m_cfg[ 22], 0, 10, 300); regMap["LinPreampL"] = &Itkpixv2GlobalCfg::LinPreampL;
    //23
    LinPreampR.init         ( 23, &m_cfg[ 23], 0, 10, 300); regMap["LinPreampR"] = &Itkpixv2GlobalCfg::LinPreampR;
    //24
    LinPreampTL.init        ( 24, &m_cfg[ 24], 0, 10, 300); regMap["LinPreampTL"] = &Itkpixv2GlobalCfg::LinPreampTL;
    //25
    LinPreampTR.init        ( 25, &m_cfg[ 25], 0, 10, 300); regMap["LinPreampTR"] = &Itkpixv2GlobalCfg::LinPreampTR;
    //26
    LinPreampT.init         ( 26, &m_cfg[ 26], 0, 10, 300); regMap["LinPreampT"] = &Itkpixv2GlobalCfg::LinPreampT;
    //27
    LinPreampM.init         ( 27, &m_cfg[ 27], 0, 10, 300); regMap["LinPreampM"] = &Itkpixv2GlobalCfg::LinPreampM;
    //28
    LinFc.init              ( 28, &m_cfg[ 28], 0, 10, 20); regMap["LinFc"] = &Itkpixv2GlobalCfg::LinFc;
    //29
    LinKrumCurr.init        ( 29, &m_cfg[ 29], 0, 10, 50); regMap["LinKrumCurr"] = &Itkpixv2GlobalCfg::LinKrumCurr;
    //30
    LinRefKrum.init         ( 30, &m_cfg[ 30], 0, 10, 300); regMap["LinRefKrum"] = &Itkpixv2GlobalCfg::LinRefKrum;
    //31
    LinComp.init            ( 31, &m_cfg[ 31], 0, 10, 110); regMap["LinComp"] = &Itkpixv2GlobalCfg::LinComp;
    //32
    LinCompTa.init          ( 32, &m_cfg[ 32], 0, 10, 110); regMap["LinCompTa"] = &Itkpixv2GlobalCfg::LinCompTa;
    //33
    LinGdacL.init           ( 33, &m_cfg[ 33], 0, 10, 408); regMap["LinGdacL"] = &Itkpixv2GlobalCfg::LinGdacL;
    //34
    LinGdacR.init           ( 34, &m_cfg[ 34], 0, 10, 408); regMap["LinGdacR"] = &Itkpixv2GlobalCfg::LinGdacR;
    //35
    LinGdacM.init           ( 35, &m_cfg[ 35], 0, 10, 408); regMap["LinGdacM"] = &Itkpixv2GlobalCfg::LinGdacM;
    //36
    LinLdac.init            ( 36, &m_cfg[ 36], 0, 10, 100); regMap["LinLdac"] = &Itkpixv2GlobalCfg::LinLdac;

    // Power
    //38
    SldoEnUndershuntA.init  ( 38, &m_cfg[ 38], 9,  1, 0); regMap["SldoEnUndershuntA"] = &Itkpixv2GlobalCfg::SldoEnUndershuntA;
    SldoEnUndershuntD.init  ( 38, &m_cfg[ 38], 8,  1, 0); regMap["SldoEnUndershuntD"] = &Itkpixv2GlobalCfg::SldoEnUndershuntD;
    SldoTrimA.init          ( 38, &m_cfg[ 38], 4,  4, 8); regMap["SldoTrimA"] = &Itkpixv2GlobalCfg::SldoTrimA;
    SldoTrimD.init          ( 38, &m_cfg[ 38], 0,  4, 8); regMap["SldoTrimD"] = &Itkpixv2GlobalCfg::SldoTrimD;

    // Pixel Matrix
    //39
    EnCoreCol3.init         ( 39, &m_cfg[ 39], 0,  6, 63); regMap["EnCoreCol3"] = &Itkpixv2GlobalCfg::EnCoreCol3;
    //40
    EnCoreCol2.init         ( 40, &m_cfg[ 40], 0, 16, 65535); regMap["EnCoreCol2"] = &Itkpixv2GlobalCfg::EnCoreCol2;
    //41
    EnCoreCol1.init         ( 41, &m_cfg[ 41], 0, 16, 65535); regMap["EnCoreCol1"] = &Itkpixv2GlobalCfg::EnCoreCol1;
    //42
    EnCoreCol0.init         ( 42, &m_cfg[ 42], 0, 16, 65535); regMap["EnCoreCol0"] = &Itkpixv2GlobalCfg::EnCoreCol0;
    //43
    RstCoreCol3.init        ( 43, &m_cfg[ 43], 0,  6, 0); regMap["RstCoreCol3"] = &Itkpixv2GlobalCfg::RstCoreCol3;
    //44
    RstCoreCol2.init        ( 44, &m_cfg[ 44], 0, 16, 0); regMap["RstCoreCol2"] = &Itkpixv2GlobalCfg::RstCoreCol2;
    //45
    RstCoreCol1.init        ( 45, &m_cfg[ 45], 0, 16, 0); regMap["RstCoreCol1"] = &Itkpixv2GlobalCfg::RstCoreCol1;
    //46
    RstCoreCol0.init        ( 46, &m_cfg[ 46], 0, 16, 0); regMap["RstCoreCol0"] = &Itkpixv2GlobalCfg::RstCoreCol0;

    // Digital functions
    //47
    TwoLevelTrig.init       ( 47, &m_cfg[ 47], 9,  1, 0); regMap["TwoLevelTrig"] = &Itkpixv2GlobalCfg::TwoLevelTrig;
    Latency.init            ( 47, &m_cfg[ 47], 0,  9, 500); regMap["Latency"] = &Itkpixv2GlobalCfg::Latency;
    //48
    SelfTrigEn.init         ( 48, &m_cfg[ 48], 5,  1, 0); regMap["SelfTrigEn"] = &Itkpixv2GlobalCfg::SelfTrigEn;
    SelfTrigDigThrEn.init   ( 48, &m_cfg[ 48], 4,  1, 0); regMap["SelfTrigDigThrEn"] = &Itkpixv2GlobalCfg::SelfTrigDigThrEn;
    SelfTrigDigThr.init     ( 48, &m_cfg[ 48], 0,  4, 1); regMap["SelfTrigDigThr"] = &Itkpixv2GlobalCfg::SelfTrigDigThr;
    //49
    SelfTrigDelay.init      ( 49, &m_cfg[ 49], 5, 10, 512); regMap["SelfTrigDelay"] = &Itkpixv2GlobalCfg::SelfTrigDelay;
    SelfTrigMulti.init      ( 49, &m_cfg[ 49], 0, 5, 4); regMap["SelfTrigMulti"] = &Itkpixv2GlobalCfg::SelfTrigMulti;
    //50
    SelfTrigPattern.init    ( 50, &m_cfg[ 50], 0, 16, 65534); regMap["SelfTrigPattern"] = &Itkpixv2GlobalCfg::SelfTrigPattern;
    //51
    DataReadDelay.init      ( 51, &m_cfg[ 51], 12,  2, 0); regMap["DataReadDelay"] = &Itkpixv2GlobalCfg::DataReadDelay;
    ReadTrigLatency.init    ( 51, &m_cfg[ 51], 0, 12, 1000); regMap["ReadTrigLatency"] = &Itkpixv2GlobalCfg::ReadTrigLatency;
    //52
    TruncTimeoutConf.init   ( 52, &m_cfg[ 52], 0, 12, 0); regMap["TruncTimeoutConf"] = &Itkpixv2GlobalCfg::TruncTimeoutConf;
    //53
    InjDigEn.init           ( 53, &m_cfg[ 53], 7,  1, 0); regMap["InjDigEn"] = &Itkpixv2GlobalCfg::InjDigEn;
    InjAnaMode.init         ( 53, &m_cfg[ 53], 6,  1, 0); regMap["InjAnaMode"] = &Itkpixv2GlobalCfg::InjAnaMode;
    InjFineDelay.init       ( 53, &m_cfg[ 53], 0,  6, 5); regMap["InjFineDelay"] = &Itkpixv2GlobalCfg::InjFineDelay;
    //54
    FineDelayClk.init       ( 54, &m_cfg[ 54], 6,  6, 0); regMap["FineDelayClk"] = &Itkpixv2GlobalCfg::FineDelayClk;
    FineDelayData.init      ( 54, &m_cfg[ 54], 0,  6, 0); regMap["FineDelayData"] = &Itkpixv2GlobalCfg::FineDelayData;
    //55
    InjVcalHigh.init        ( 55, &m_cfg[ 55], 0, 12, 200); regMap["InjVcalHigh"] = &Itkpixv2GlobalCfg::InjVcalHigh;
    //56
    InjVcalMed.init         ( 56, &m_cfg[ 56], 0, 12, 200); regMap["InjVcalMed"] = &Itkpixv2GlobalCfg::InjVcalMed;
    //57
    CapMeasEnPar.init       ( 57, &m_cfg[ 57], 2,  1, 0); regMap["CapMeasEnPar"] = &Itkpixv2GlobalCfg::CapMeasEnPar;
    CapMeasEn.init          ( 57, &m_cfg[ 57], 1,  1, 0); regMap["CapMeasEn"] = &Itkpixv2GlobalCfg::CapMeasEn;
    InjVcalRange.init       ( 57, &m_cfg[ 57], 0,  1, 1); regMap["InjVcalRange"] = &Itkpixv2GlobalCfg::InjVcalRange;
    //58
    CdrOverwriteLimit.init  ( 58, &m_cfg[ 58], 4,  1, 0); regMap["CdrOverwriteLimit"] = &Itkpixv2GlobalCfg::CdrOverwriteLimit;
    CdrPhaseDetSel.init     ( 58, &m_cfg[ 58], 3,  1, 0); regMap["CdrPhaseDetSel"] = &Itkpixv2GlobalCfg::CdrPhaseDetSel;
    CdrClkSel.init          ( 58, &m_cfg[ 58], 0,  3, 1); regMap["CdrClkSel"] = &Itkpixv2GlobalCfg::CdrClkSel;
    //59
    ChSyncLockThr.init      ( 59, &m_cfg[ 59], 0,  5, 31); regMap["ChSyncLockThr"] = &Itkpixv2GlobalCfg::ChSyncLockThr;
    //60
    GlobalPulseConf.init    ( 60, &m_cfg[ 60], 0, 16, 0); regMap["GlobalPulseConf"] = &Itkpixv2GlobalCfg::GlobalPulseConf;
    //61
    GlobalPulseWidth.init   ( 61, &m_cfg[ 61], 0,  8, 0); regMap["GlobalPulseWidth"] = &Itkpixv2GlobalCfg::GlobalPulseWidth;
    //62
    ServiceBlockEn.init     ( 62, &m_cfg[ 62], 8,  1, 1); regMap["ServiceBlockEn"] = &Itkpixv2GlobalCfg::ServiceBlockEn;
    ServiceBlockPeriod.init ( 62, &m_cfg[ 62], 0,  8, 50); regMap["ServiceBlockPeriod"] = &Itkpixv2GlobalCfg::ServiceBlockPeriod;
    //63
    TotEnPtot.init          ( 63, &m_cfg[ 63], 12,  1, 0); regMap["TotEnPtot"] = &Itkpixv2GlobalCfg::TotEnPtot;
    TotEnPtoa.init          ( 63, &m_cfg[ 63], 11,  1, 0); regMap["TotEnPtoa"] = &Itkpixv2GlobalCfg::TotEnPtoa;
    TotEn80.init            ( 63, &m_cfg[ 63], 10,  1, 0); regMap["TotEn80"] = &Itkpixv2GlobalCfg::TotEn80;
    TotEn6b4b.init          ( 63, &m_cfg[ 63], 9,  1, 0); regMap["TotEn6b4b"] = &Itkpixv2GlobalCfg::TotEn6b4b;
    TotPtotLatency.init     ( 63, &m_cfg[ 63], 0,  9, 500); regMap["TotPtotLatency"] = &Itkpixv2GlobalCfg::TotPtotLatency;
    //64
    PtotCoreColEn3.init     ( 64, &m_cfg[ 64], 0,  6, 0); regMap["PtotCoreColEn3"] = &Itkpixv2GlobalCfg::PtotCoreColEn3;
    //65
    PtotCoreColEn2.init     ( 65, &m_cfg[ 65], 0, 16, 0); regMap["PtotCoreColEn2"] = &Itkpixv2GlobalCfg::PtotCoreColEn2;
    //66
    PtotCoreColEn1.init     ( 66, &m_cfg[ 66], 0, 16, 0); regMap["PtotCoreColEn1"] = &Itkpixv2GlobalCfg::PtotCoreColEn1;
    //67
    PtotCoreColEn0.init     ( 67, &m_cfg[ 67], 0, 16, 0); regMap["PtotCoreColEn0"] = &Itkpixv2GlobalCfg::PtotCoreColEn0;
    //68
    DataMergeInPol.init     ( 68, &m_cfg[ 68], 8,  4, 0); regMap["DataMergeInPol"] = &Itkpixv2GlobalCfg::DataMergeInPol;
    EnChipId.init           ( 68, &m_cfg[ 68], 7,  1, 0); regMap["EnChipId"] = &Itkpixv2GlobalCfg::EnChipId;
    DataMergeEnClkGate.init ( 68, &m_cfg[ 68], 6,  1, 0); regMap["DataMergeEnClkGate"] = &Itkpixv2GlobalCfg::DataMergeEnClkGate;
    DataMergeSelClk.init    ( 68, &m_cfg[ 68], 5,  1, 0); regMap["DataMergeSelClk"] = &Itkpixv2GlobalCfg::DataMergeSelClk;
    DataMergeEn.init        ( 68, &m_cfg[ 68], 1,  4, 0); regMap["DataMergeEn"] = &Itkpixv2GlobalCfg::DataMergeEn;
    DataMergeEnBond.init    ( 68, &m_cfg[ 68], 0,  1, 0); regMap["DataMergeEnBond"] = &Itkpixv2GlobalCfg::DataMergeEnBond;
    //69
    DataMergeInMux3.init    ( 69, &m_cfg[ 69], 14,  2, 3); regMap["DataMergeInMux3"] = &Itkpixv2GlobalCfg::DataMergeInMux3;
    DataMergeInMux2.init    ( 69, &m_cfg[ 69], 12,  2, 2); regMap["DataMergeInMux2"] = &Itkpixv2GlobalCfg::DataMergeInMux2;
    DataMergeInMux1.init    ( 69, &m_cfg[ 69], 10,  2, 1); regMap["DataMergeInMux1"] = &Itkpixv2GlobalCfg::DataMergeInMux1;
    DataMergeInMux0.init    ( 69, &m_cfg[ 69], 8,  2, 0); regMap["DataMergeInMux0"] = &Itkpixv2GlobalCfg::DataMergeInMux0;
    DataMergeOutMux3.init   ( 69, &m_cfg[ 69], 6,  2, 0); regMap["DataMergeOutMux3"] = &Itkpixv2GlobalCfg::DataMergeOutMux3;
    DataMergeOutMux2.init   ( 69, &m_cfg[ 69], 4,  2, 1); regMap["DataMergeOutMux2"] = &Itkpixv2GlobalCfg::DataMergeOutMux2;
    DataMergeOutMux1.init   ( 69, &m_cfg[ 69], 2,  2, 2); regMap["DataMergeOutMux1"] = &Itkpixv2GlobalCfg::DataMergeOutMux1;
    DataMergeOutMux0.init   ( 69, &m_cfg[ 69], 0,  2, 3); regMap["DataMergeOutMux0"] = &Itkpixv2GlobalCfg::DataMergeOutMux0;
    //70-73
    EnCoreColCal3.init      ( 70, &m_cfg[ 70], 0,  6, 0); regMap["EnCoreColCal3"] = &Itkpixv2GlobalCfg::EnCoreColCal3;
    EnCoreColCal2.init      ( 71, &m_cfg[ 71], 0, 16, 0); regMap["EnCoreColCal2"] = &Itkpixv2GlobalCfg::EnCoreColCal2;
    EnCoreColCal1.init      ( 72, &m_cfg[ 72], 0, 16, 0); regMap["EnCoreColCal1"] = &Itkpixv2GlobalCfg::EnCoreColCal1;
    EnCoreColCal0.init      ( 73, &m_cfg[ 73], 0, 16, 0); regMap["EnCoreColCal0"] = &Itkpixv2GlobalCfg::EnCoreColCal0;
    //74
    DataEnBcid.init         ( 74, &m_cfg[ 74], 10,  1, 0); regMap["DataEnBcid"] = &Itkpixv2GlobalCfg::DataEnBcid;
    DataEnL1id.init         ( 74, &m_cfg[ 74], 9,  1, 0); regMap["DataEnL1id"] = &Itkpixv2GlobalCfg::DataEnL1id;
    DataEnEos.init          ( 74, &m_cfg[ 74], 8,  1, 1); regMap["DataEnEos"] = &Itkpixv2GlobalCfg::DataEnEos;
    NumOfEventsInStream.init( 74, &m_cfg[ 74], 0,  8, 1); regMap["NumOfEventsInStream"] = &Itkpixv2GlobalCfg::NumOfEventsInStream;
    //75
    DataEnBinaryRo.init     ( 75, &m_cfg[ 75], 10,  1, 0); regMap["DataEnBinaryRo"] = &Itkpixv2GlobalCfg::DataEnBinaryRo;
    DataEnRaw.init          ( 75, &m_cfg[ 75], 9,  1, 0); regMap["DataEnRaw"] = &Itkpixv2GlobalCfg::DataEnRaw;
    DataEnHitRemoval.init   ( 75, &m_cfg[ 75], 8,  1, 0); regMap["DataEnHitRemoval"] = &Itkpixv2GlobalCfg::DataEnHitRemoval;
    DataMaxHits.init        ( 75, &m_cfg[ 75], 4,  4, 0); regMap["DataMaxHits"] = &Itkpixv2GlobalCfg::DataMaxHits;
    DataEnIsoHitRemoval.init( 75, &m_cfg[ 75], 3,  1, 0); regMap["DataEnIsoHitRemoval"] = &Itkpixv2GlobalCfg::DataEnIsoHitRemoval;
    DataMaxTot.init         ( 75, &m_cfg[ 75], 0,  3, 0); regMap["DataMaxTot"] = &Itkpixv2GlobalCfg::DataMaxTot;
    //76
    EvenMask.init           ( 76, &m_cfg[ 76], 0, 16, 0); regMap["EvenMask"] = &Itkpixv2GlobalCfg::EvenMask;
    //77
    OddMask.init            ( 77, &m_cfg[ 77], 0, 16, 0); regMap["OddMask"] = &Itkpixv2GlobalCfg::OddMask;
    //78
    EfuseConfig.init        ( 78, &m_cfg[ 78], 0, 16, 0); regMap["EfuseConfig"] = &Itkpixv2GlobalCfg::EfuseConfig;
    //79
    EfuseWriteData1.init    ( 79, &m_cfg[ 79], 0, 16, 0); regMap["EfuseWriteData1"] = &Itkpixv2GlobalCfg::EfuseWriteData1;
    //80
    EfuseWriteData0.init    ( 80, &m_cfg[ 80], 0, 16, 0); regMap["EfuseWriteData0"] = &Itkpixv2GlobalCfg::EfuseWriteData0;
    //81
    AuroraEnPrbs.init       ( 81, &m_cfg[ 81], 12,  1, 0); regMap["AuroraEnPrbs"] = &Itkpixv2GlobalCfg::AuroraEnPrbs;
    AuroraActiveLanes.init  ( 81, &m_cfg[ 81], 8,  4, 15); regMap["AuroraActiveLanes"] = &Itkpixv2GlobalCfg::AuroraActiveLanes;
    AuroraCCWait.init       ( 81, &m_cfg[ 81], 2,  6, 25); regMap["AuroraCCWait"] = &Itkpixv2GlobalCfg::AuroraCCWait;
    AuroraCCSend.init       ( 81, &m_cfg[ 81], 0,  2, 3); regMap["AuroraCCSend"] = &Itkpixv2GlobalCfg::AuroraCCSend;
    //82
    AuroraCBWait1.init      ( 82, &m_cfg[ 82], 0,  8, 0); regMap["AuroraCBWait1"] = &Itkpixv2GlobalCfg::AuroraCBWait1;
    //83
    AuroraCBWait0.init      ( 83, &m_cfg[ 83], 4, 12, 4095); regMap["AuroraCBWait0"] = &Itkpixv2GlobalCfg::AuroraCBWait0;
    AuroraCBSend.init       ( 83, &m_cfg[ 83], 0,  4, 0); regMap["AuroraCBSend"] = &Itkpixv2GlobalCfg::AuroraCBSend;
    //84
    AuroraInitWait.init     ( 84, &m_cfg[ 84], 0, 11, 32); regMap["AuroraInitWait"] = &Itkpixv2GlobalCfg::AuroraInitWait;
    //85
    GpValReg.init           ( 85, &m_cfg[ 85], 9,  4, 5); regMap["GpValReg"] = &Itkpixv2GlobalCfg::GpValReg;
    GpCmosEn.init           ( 85, &m_cfg[ 85], 8,  1, 1); regMap["GpCmosEn"] = &Itkpixv2GlobalCfg::GpCmosEn;
    GpCmosDs.init           ( 85, &m_cfg[ 85], 7,  1, 0); regMap["GpCmosDs"] = &Itkpixv2GlobalCfg::GpCmosDs;
    GpLvdsEn.init           ( 85, &m_cfg[ 85], 3,  4, 0xF); regMap["GpLvdsEn"] = &Itkpixv2GlobalCfg::GpLvdsEn;
    GpLvdsBias.init         ( 85, &m_cfg[ 85], 0,  3, 7); regMap["GpLvdsBias"] = &Itkpixv2GlobalCfg::GpLvdsBias;
    //86
    GpCmosRoute.init        ( 86, &m_cfg[ 86], 0,  7, 34); regMap["GpCmosRoute"] = &Itkpixv2GlobalCfg::GpCmosRoute;
    //87
    GpLvdsPad3.init         ( 87, &m_cfg[ 87], 6,  6, 35); regMap["GpLvdsPad3"] = &Itkpixv2GlobalCfg::GpLvdsPad3;
    GpLvdsPad2.init         ( 87, &m_cfg[ 87], 0,  6, 33); regMap["GpLvdsPad2"] = &Itkpixv2GlobalCfg::GpLvdsPad2;
    //88
    GpLvdsPad1.init         ( 88, &m_cfg[ 88], 6,  6, 1); regMap["GpLvdsPad1"] = &Itkpixv2GlobalCfg::GpLvdsPad1;
    GpLvdsPad0.init         ( 88, &m_cfg[ 88], 0,  6, 0); regMap["GpLvdsPad0"] = &Itkpixv2GlobalCfg::GpLvdsPad0;
    //89
    CdrCp.init              ( 89, &m_cfg[ 89], 0, 10, 40); regMap["CdrCp"] = &Itkpixv2GlobalCfg::CdrCp;
    //90
    CdrCpFd.init            ( 90, &m_cfg[ 90], 0, 10, 400); regMap["CdrCpFd"] = &Itkpixv2GlobalCfg::CdrCpFd;
    //91
    CdrCpBuff.init          ( 91, &m_cfg[ 91], 0, 10, 200); regMap["CdrCpBuff"] = &Itkpixv2GlobalCfg::CdrCpBuff;
    //92
    CdrVco.init             ( 92, &m_cfg[ 92], 0, 10, 1023); regMap["CdrVco"] = &Itkpixv2GlobalCfg::CdrVco;
    //93
    CdrVcoBuff.init         ( 93, &m_cfg[ 93], 0, 10, 500); regMap["CdrVcoBuff"] = &Itkpixv2GlobalCfg::CdrVcoBuff;
    //94
    SerSelOut3.init         ( 94, &m_cfg[ 94], 6,  2, 1); regMap["SerSelOut3"] = &Itkpixv2GlobalCfg::SerSelOut3;
    SerSelOut2.init         ( 94, &m_cfg[ 94], 4,  2, 1); regMap["SerSelOut2"] = &Itkpixv2GlobalCfg::SerSelOut2;
    SerSelOut1.init         ( 94, &m_cfg[ 94], 2,  2, 1); regMap["SerSelOut1"] = &Itkpixv2GlobalCfg::SerSelOut1;
    SerSelOut0.init         ( 94, &m_cfg[ 94], 0,  2, 1); regMap["SerSelOut0"] = &Itkpixv2GlobalCfg::SerSelOut0;
    //95
    SerInvTap.init          ( 95, &m_cfg[ 95], 6,  2, 0); regMap["SerInvTap"] = &Itkpixv2GlobalCfg::SerInvTap;
    SerEnTap.init           ( 95, &m_cfg[ 95], 4,  2, 0); regMap["SerEnTap"] = &Itkpixv2GlobalCfg::SerEnTap;
    SerEnLane.init          ( 95, &m_cfg[ 95], 0,  4, 15); regMap["SerEnLane"] = &Itkpixv2GlobalCfg::SerEnLane;
    //96
    CmlBias2.init           ( 96, &m_cfg[ 96], 0, 10, 0); regMap["CmlBias2"] = &Itkpixv2GlobalCfg::CmlBias2;
    //97
    CmlBias1.init           ( 97, &m_cfg[ 97], 0, 10, 0); regMap["CmlBias1"] = &Itkpixv2GlobalCfg::CmlBias1;
    //98
    CmlBias0.init           ( 98, &m_cfg[ 98], 0, 10, 500); regMap["CmlBias0"] = &Itkpixv2GlobalCfg::CmlBias0;
    //99
    MonitorEnable.init      ( 99, &m_cfg[ 99], 12,  1, 0); regMap["MonitorEnable"] = &Itkpixv2GlobalCfg::MonitorEnable;
    MonitorI.init           ( 99, &m_cfg[ 99], 6,  6, 63); regMap["MonitorI"] = &Itkpixv2GlobalCfg::MonitorI;
    MonitorV.init           ( 99, &m_cfg[ 99], 0,  6, 63); regMap["MonitorV"] = &Itkpixv2GlobalCfg::MonitorV;
    //100
    ErrWngMask.init         (100, &m_cfg[100], 0,  8, 0); regMap["ErrWngMask"] = &Itkpixv2GlobalCfg::ErrWngMask;
    //101
    MonSensSldoDigEn.init   (101, &m_cfg[101], 11,  1, 0); regMap["MonSensSldoDigEn"] = &Itkpixv2GlobalCfg::MonSensSldoDigEn;
    MonSensSldoDigDem.init  (101, &m_cfg[101], 7,  4, 0); regMap["MonSensSldoDigDem"] = &Itkpixv2GlobalCfg::MonSensSldoDigDem;
    MonSensSldoDigSelBias.init(101, &m_cfg[101], 6,  1, 0); regMap["MonSensSldoDigSelBias"] = &Itkpixv2GlobalCfg::MonSensSldoDigSelBias;
    MonSensSldoAnaEn.init   (101, &m_cfg[101], 5,  1, 0); regMap["MonSensSldoAnaEn"] = &Itkpixv2GlobalCfg::MonSensSldoAnaEn;
    MonSensSldoAnaDem.init  (101, &m_cfg[101], 1,  4, 0); regMap["MonSensSldoAnaDem"] = &Itkpixv2GlobalCfg::MonSensSldoAnaDem;
    MonSensSldoAnaSelBias.init(101, &m_cfg[101], 0,  1, 0); regMap["MonSensSldoAnaSelBias"] = &Itkpixv2GlobalCfg::MonSensSldoAnaSelBias;
    //102
    MonSensAcbEn.init       (102, &m_cfg[102], 5,  1, 0); regMap["MonSensAcbEn"] = &Itkpixv2GlobalCfg::MonSensAcbEn;
    MonSensAcbDem.init      (102, &m_cfg[102], 1,  4, 0); regMap["MonSensAcbDem"] = &Itkpixv2GlobalCfg::MonSensAcbDem;
    MonSensAcbSelBias.init  (102, &m_cfg[102], 0,  1, 0); regMap["MonSensAcbSelBias"] = &Itkpixv2GlobalCfg::MonSensAcbSelBias;
    //103
    VrefRsensBot.init       (103, &m_cfg[103], 8,  1, 0); regMap["VrefRsensBot"] = &Itkpixv2GlobalCfg::VrefRsensBot;
    VrefRsensTop.init       (103, &m_cfg[103], 7,  1, 0); regMap["VrefRsensTop"] = &Itkpixv2GlobalCfg::VrefRsensTop;
    VrefIn.init             (103, &m_cfg[103], 6,  1, 1); regMap["VrefIn"] = &Itkpixv2GlobalCfg::VrefIn;
    MonAdcTrim.init         (103, &m_cfg[103], 0,  6, 0); regMap["MonAdcTrim"] = &Itkpixv2GlobalCfg::MonAdcTrim;

    //104
    NtcDac.init             (104, &m_cfg[104], 0, 10, 100); regMap["NtcDac"] = &Itkpixv2GlobalCfg::NtcDac;
    //105-108
    HitOrMask3.init         (105, &m_cfg[105], 0,  6, 0); regMap["HitOrMask3"] = &Itkpixv2GlobalCfg::HitOrMask3;
    HitOrMask2.init         (106, &m_cfg[106], 0, 16, 0); regMap["HitOrMask2"] = &Itkpixv2GlobalCfg::HitOrMask2;
    HitOrMask1.init         (107, &m_cfg[107], 0, 16, 0); regMap["HitOrMask1"] = &Itkpixv2GlobalCfg::HitOrMask1;
    HitOrMask0.init         (108, &m_cfg[108], 0, 16, 0); regMap["HitOrMask0"] = &Itkpixv2GlobalCfg::HitOrMask0;
    //109-116
    AutoRead0.init          (109, &m_cfg[109], 0,  9, 137); regMap["AutoRead0"] = &Itkpixv2GlobalCfg::AutoRead0;
    AutoRead1.init          (110, &m_cfg[110], 0,  9, 133); regMap["AutoRead1"] = &Itkpixv2GlobalCfg::AutoRead1;
    AutoRead2.init          (111, &m_cfg[111], 0,  9, 121); regMap["AutoRead2"] = &Itkpixv2GlobalCfg::AutoRead2;
    AutoRead3.init          (112, &m_cfg[112], 0,  9, 122); regMap["AutoRead3"] = &Itkpixv2GlobalCfg::AutoRead3;
    AutoRead4.init          (113, &m_cfg[113], 0,  9, 124); regMap["AutoRead4"] = &Itkpixv2GlobalCfg::AutoRead4;
    AutoRead5.init          (114, &m_cfg[114], 0,  9, 127); regMap["AutoRead5"] = &Itkpixv2GlobalCfg::AutoRead5;
    AutoRead6.init          (115, &m_cfg[115], 0,  9, 126); regMap["AutoRead6"] = &Itkpixv2GlobalCfg::AutoRead6;
    AutoRead7.init          (116, &m_cfg[116], 0,  9, 125); regMap["AutoRead7"] = &Itkpixv2GlobalCfg::AutoRead7;
    //117
    RingOscBClear.init      (117, &m_cfg[117], 14,  1, 0); regMap["RingOscBClear"] = &Itkpixv2GlobalCfg::RingOscBClear;
    RingOscBEnBl.init       (117, &m_cfg[117], 13,  1, 0); regMap["RingOscBEnBl"] = &Itkpixv2GlobalCfg::RingOscBEnBl;
    RingOscBEnBr.init       (117, &m_cfg[117], 12,  1, 0); regMap["RingOscBEnBr"] = &Itkpixv2GlobalCfg::RingOscBEnBr;
    RingOscBEnCapA.init     (117, &m_cfg[117], 11,  1, 0); regMap["RingOscBEnCapA"] = &Itkpixv2GlobalCfg::RingOscBEnCapA;
    RingOscBEnFf.init       (117, &m_cfg[117], 10,  1, 0); regMap["RingOscBEnFf"] = &Itkpixv2GlobalCfg::RingOscBEnFf;
    RingOscBEnLvt.init      (117, &m_cfg[117], 9,  1, 0); regMap["RingOscBEnLvt"] = &Itkpixv2GlobalCfg::RingOscBEnLvt;
    RingOscAClear.init      (117, &m_cfg[117], 8,  1, 0); regMap["RingOscAClear"] = &Itkpixv2GlobalCfg::RingOscAClear;
    RingOscAEn.init         (117, &m_cfg[117], 0,  8, 0); regMap["RingOscAEn"] = &Itkpixv2GlobalCfg::RingOscAEn;
    //118
    RingOscARoute.init      (118, &m_cfg[118], 6,  3, 0); regMap["RingOscARoute"] = &Itkpixv2GlobalCfg::RingOscARoute;
    RingOscBRoute.init      (118, &m_cfg[118], 0,  6, 0); regMap["RingOscBRoute"] = &Itkpixv2GlobalCfg::RingOscBRoute;
    //119-120
    RingOscAOut.init        (119, &m_cfg[119], 0, 16, 0); regMap["RingOscAOut"] = &Itkpixv2GlobalCfg::RingOscAOut;
    RingOscBOut.init        (120, &m_cfg[120], 0, 16, 0); regMap["RingOscBOut"] = &Itkpixv2GlobalCfg::RingOscBOut;
    //121-123 RO
    BcidCnt.init            (121, &m_cfg[121], 0, 16, 0); regMap["BcidCnt"] = &Itkpixv2GlobalCfg::BcidCnt;
    TrigCnt.init            (122, &m_cfg[122], 0, 16, 0); regMap["TrigCnt"] = &Itkpixv2GlobalCfg::TrigCnt;
    ReadTrigCnt.init        (123, &m_cfg[123], 0, 16, 0); regMap["ReadTrigCnt"] = &Itkpixv2GlobalCfg::ReadTrigCnt;
    //124-128
    LockLossCnt.init        (124, &m_cfg[124], 0, 16, 0); regMap["LockLossCnt"] = &Itkpixv2GlobalCfg::LockLossCnt;
    BitFlipWngCnt.init      (125, &m_cfg[125], 0, 16, 0); regMap["BitFlipWngCnt"] = &Itkpixv2GlobalCfg::BitFlipWngCnt;
    BitFlipErrCnt.init      (126, &m_cfg[126], 0, 16, 0); regMap["BitFlipErrCnt"] = &Itkpixv2GlobalCfg::BitFlipErrCnt;
    CmdErrCnt.init          (127, &m_cfg[127], 0, 16, 0); regMap["CmdErrCnt"] = &Itkpixv2GlobalCfg::CmdErrCnt;
    RdWrFifoErrCnt.init     (128, &m_cfg[128], 0, 16, 0); regMap["RdWrFifoErrCnt"] = &Itkpixv2GlobalCfg::RdWrFifoErrCnt;
    //129
    AiRegionRow.init        (129, &m_cfg[129], 0,  9, 0); regMap["AiRegionRow"] = &Itkpixv2GlobalCfg::AiRegionRow;
    //130-133
    HitOrCnt3.init          (130, &m_cfg[130], 0, 16, 0); regMap["HitOrCnt3"] = &Itkpixv2GlobalCfg::HitOrCnt3;
    HitOrCnt2.init          (131, &m_cfg[131], 0, 16, 0); regMap["HitOrCnt2"] = &Itkpixv2GlobalCfg::HitOrCnt2;
    HitOrCnt1.init          (132, &m_cfg[132], 0, 16, 0); regMap["HitOrCnt1"] = &Itkpixv2GlobalCfg::HitOrCnt1;
    HitOrCnt0.init          (133, &m_cfg[133], 0, 16, 0); regMap["HitOrCnt0"] = &Itkpixv2GlobalCfg::HitOrCnt0;
    //134
    SkippedTrigCnt.init     (134, &m_cfg[134], 0, 16, 0); regMap["SkippedTrigCnt"] = &Itkpixv2GlobalCfg::SkippedTrigCnt;
    //135-136
    EfuseReadData1.init     (135, &m_cfg[135], 0, 16, 0); regMap["EfuseReadData1"] = &Itkpixv2GlobalCfg::EfuseReadData1;
    EfuseReadData0.init     (136, &m_cfg[136], 0, 16, 0); regMap["EfuseReadData0"] = &Itkpixv2GlobalCfg::EfuseReadData0;
    //137
    MonitoringDataAdc.init  (137, &m_cfg[137], 0, 12, 0); regMap["MonitoringDataAdc"] = &Itkpixv2GlobalCfg::MonitoringDataAdc;

    // Special virtual registers
    InjVcalDiff.init(&InjVcalMed, &InjVcalHigh, true); virtRegMap["InjVcalDiff"] = (Itkpixv2RegDefault Itkpixv2GlobalCfg::*)&Itkpixv2GlobalCfg::InjVcalDiff;
    DiffTh1.init({&DiffTh1M, &DiffTh1L, &DiffTh1R}); virtRegMap["DiffTh1"] = (Itkpixv2RegDefault Itkpixv2GlobalCfg::*) &Itkpixv2GlobalCfg::DiffTh1;
}

void Itkpixv2GlobalCfg::writeConfig(json &j) {
    for(auto it : regMap) {
        logger->debug("Writing reg: {}", it.first);
        j["ITKPIXV2"]["GlobalConfig"][it.first] = (this->*it.second).read();
    }    
}

void Itkpixv2GlobalCfg::loadConfig(json const &j) {
    for (auto it : regMap) {
        if (j.contains({"ITKPIXV2","GlobalConfig",it.first})) {
            (this->*it.second).write(j["ITKPIXV2"]["GlobalConfig"][it.first]);
        } else {
            logger->error("Could not find register \"{}\" using default!", it.first);
        }
    }
}
