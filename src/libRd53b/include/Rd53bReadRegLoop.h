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
    Rd53bRegDefault Rd53bGlobalCfg::*TransSensorCfg[3][3] = {
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

    void writeConfig(json &config) override;
    void loadConfig(const json &config) override;
    double convertRingOscCntToMHz(double counter) const { return counter / (m_RingOscDur << 1) * 40; }

private:
    std::vector<unsigned short> m_VoltMux;
    std::vector<std::string> m_STDReg;
    std::vector<unsigned short> m_CurMux;
    std::vector<std::string> m_TempSensors;
    std::vector<std::string> m_RadSensors;

    uint16_t ReadADC(unsigned short Reg, bool doCur, Rd53b *fe);
    float ReadNTCTemp(Rd53b *fe, bool in_kelvin);
    float ReadTransSensor(Rd53b *fe, TransSensorLocation loc, TransSensorType type, Rd53bCfg::TransSensor sensor, bool in_kelvin = false);
    float ReadResistTemp(Rd53b *fe, bool in_kelvin); // Broken for RD53B. Need to be fixed

    uint16_t m_EnblRingOscA, m_EnblRingOscB, m_RingOscDur, m_RingOscRep;

    void init() override;
    void execPart1() override;
    void execPart2() override;
    void end() override;

    Rd53bRegDefault Rd53bGlobalCfg::*RingOscBEn[5] = {&Rd53b::RingOscBEnBl, &Rd53b::RingOscBEnBr, &Rd53b::RingOscBEnFf, &Rd53b::RingOscBEnLvt, &Rd53b::RingOscBEnCapA};
};

#endif
