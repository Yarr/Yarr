#ifndef RD53BREADREGLOOP_H
#define RD53BREADREGLOOP_H

#include <array>
#include <chrono>
#include <thread>
#include "LoopActionBase.h"
#include "Rd53b.h"
#include "Rd53bCmd.h"

class Rd53bReadRegLoop : public LoopActionBase
{
    // Common enum and variables
public:
    // 0: digital SLDO, 1: analog SLDO, 2: center
    enum TransSensorLocation
    {
        DSLDO = 0,
        ASLDO = 1,
        ACB = 2
    };

    // 0: sensor enable, 1: path chonice, 2: bias selection
    Rd53bReg Rd53bGlobalCfg::*TransSensorCfg[3][3] = {
        {&Rd53b::MonSensSldoDigEn, &Rd53b::MonSensSldoDigDem, &Rd53b::MonSensSldoDigSelBias},
        {&Rd53b::MonSensSldoAnaEn, &Rd53b::MonSensSldoAnaDem, &Rd53b::MonSensSldoAnaSelBias},
        {&Rd53b::MonSensAcbEn, &Rd53b::MonSensAcbDem, &Rd53b::MonSensAcbSelBias}};

    enum TransSensorType
    {
        BJT = 0, // Radiation
        MOS = 1  // Temperature
    };

    unsigned TransSensorVMUX[3][2] = {
        {15, 16},
        {13, 14},
        {17, 18}};

    unsigned bankBEnableMap[34] = { 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1,
                                    2, 2, 2, 2, 2, 2, 
                                    3, 3, 3, 3, 
                                    4, 4, 4, 4, 4, 4, 4, 4};

public:
    Rd53bReadRegLoop();

    void writeConfig(json &config);
    void loadConfig(const json &config);
    double convertRingOscCntToMHz(double counter) { return counter / (m_RingOscDur << 1) * 40; }

private:
    std::vector<unsigned short> m_VoltMux;
    std::vector<std::string> m_STDReg;
    std::vector<unsigned short> m_CurMux;
    std::vector<std::string> m_TempSensors;
    std::vector<std::string> m_RadSensors;

    uint16_t ReadRegister(Rd53bReg Rd53bGlobalCfg::*ref, Rd53b *tmpFE);
    uint16_t ReadADC(unsigned short Reg, bool doCur = false, Rd53b *tmpFE = NULL);
    float ReadNTCTemp(Rd53b *tmpFE, bool in_kelvin = false);
    float ReadTransSensor(Rd53b *tmpFE, TransSensorLocation loc, TransSensorType type, bool in_kelvin = false);
    float ReadResistTemp(Rd53b *tmpFE = NULL, bool in_kelvin = false); // Broken for RD53B. Need to be fixed

    uint16_t m_EnblRingOscA, m_EnblRingOscB, m_RingOscDur, m_RingOscRep;

    void init();
    void execPart1();
    void execPart2();
    void end();

    Rd53bReg Rd53bGlobalCfg::*RingOscBEn[5] = {&Rd53b::RingOscBEnBl, &Rd53b::RingOscBEnBr, &Rd53b::RingOscBEnFf, &Rd53b::RingOscBEnLvt, &Rd53b::RingOscBEnCapA};
};

#endif
