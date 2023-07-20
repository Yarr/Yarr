#ifndef ITKPIXV2READREGLOOP_H
#define ITKPIXV2READREGLOOP_H

#include <array>
#include <chrono>
#include <thread>
#include "LoopActionBase.h"
#include "Itkpixv2.h"
#include "Itkpixv2Cmd.h"

class Itkpixv2ReadRegLoop : public LoopActionBase
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
    Itkpixv2RegDefault Itkpixv2GlobalCfg::*TransSensorCfg[3][3] = {
        {&Itkpixv2::MonSensSldoDigEn, &Itkpixv2::MonSensSldoDigDem, &Itkpixv2::MonSensSldoDigSelBias},
        {&Itkpixv2::MonSensSldoAnaEn, &Itkpixv2::MonSensSldoAnaDem, &Itkpixv2::MonSensSldoAnaSelBias},
        {&Itkpixv2::MonSensAcbEn, &Itkpixv2::MonSensAcbDem, &Itkpixv2::MonSensAcbSelBias}};

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
    Itkpixv2ReadRegLoop();

    void writeConfig(json &config) override;
    void loadConfig(const json &config) override;
    double convertRingOscCntToMHz(double counter) const { return counter / (m_RingOscDur << 1) * 40; }

private:
    std::vector<unsigned short> m_VoltMux;
    std::vector<std::string> m_STDReg;
    std::vector<unsigned short> m_CurMux;
    std::vector<std::string> m_TempSensors;
    std::vector<std::string> m_RadSensors;

    uint16_t ReadADC(unsigned short Reg, bool doCur, Itkpixv2 *fe);
    float ReadNTCTemp(Itkpixv2 *fe, bool in_kelvin);
    float ReadTransSensor(Itkpixv2 *fe, TransSensorLocation loc, TransSensorType type, Itkpixv2Cfg::TransSensor sensor, bool in_kelvin = false);
    float ReadResistTemp(Itkpixv2 *fe, bool in_kelvin); // Broken for ITKPIXV2. Need to be fixed

    uint16_t m_EnblRingOscA, m_EnblRingOscB, m_RingOscDur, m_RingOscRep;

    void init() override;
    void execPart1() override;
    void execPart2() override;
    void end() override;

    Itkpixv2RegDefault Itkpixv2GlobalCfg::*RingOscBEn[5] = {&Itkpixv2::RingOscBEnBl, &Itkpixv2::RingOscBEnBr, &Itkpixv2::RingOscBEnFf, &Itkpixv2::RingOscBEnLvt, &Itkpixv2::RingOscBEnCapA};
};

#endif
