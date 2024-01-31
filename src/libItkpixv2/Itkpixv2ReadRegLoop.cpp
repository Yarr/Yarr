#include "Itkpixv2ReadRegLoop.h"

#include "logging.h"

namespace
{
    auto logger = logging::make_log("Itkpixv2ReadRegLoop");
}

Itkpixv2ReadRegLoop::Itkpixv2ReadRegLoop() : LoopActionBase(LOOP_STYLE_NOP)
{
    loopType = typeid(this);
    m_EnblRingOscA = 0;
    m_EnblRingOscB = 0;
    m_RingOscDur = 0;
    m_RingOscRep = 1;
}

//Configures the ADC, reads the register returns the first recieved register.
uint16_t Itkpixv2ReadRegLoop::ReadADC(unsigned short Reg, bool doCur, Itkpixv2 *fe) {
    if (fe == NULL)
        return 0;

    fe->confAdc(Reg, doCur);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    uint16_t regVal = fe->readSingleRegister(&Itkpixv2::MonitoringDataAdc);
    regVal = fe->readSingleRegister(&Itkpixv2::MonitoringDataAdc);

    return regVal;
}

float Itkpixv2ReadRegLoop::ReadNTCTemp(Itkpixv2 *fe, bool in_kelvin) {
    //Sensor Config
    if (fe == NULL)
        return 0;

    float voltage = fe->adcToV(this->ReadADC(2, false, fe));
    float current = fe->adcToI(this->ReadADC(9, true, fe));

    return fe->readNtcTemp(voltage / current, in_kelvin);
}

float Itkpixv2ReadRegLoop::ReadResistTemp(Itkpixv2 *fe, bool in_kelvin) {
    //Sensor Config
    if (fe == NULL)
        return 0;
    
    // Read current
    float current = fe->adcToI(this->ReadADC(9, true, fe));
    
    // Read top sensor: toggle Vref to top
    fe->writeRegister(&Itkpixv2::VrefRsensTop, 1);
    fe->writeRegister(&Itkpixv2::VrefRsensBot, 0);
    while (!g_tx->isCmdEmpty()){;}
    std::this_thread::sleep_for(std::chrono::microseconds(100));

    float voltageTop = fe->adcToV(this->ReadADC(5, false, fe));

    // Read bottom sensor: toggle Vref to bottom
    fe->writeRegister(&Itkpixv2::VrefRsensTop, 0);
    fe->writeRegister(&Itkpixv2::VrefRsensBot, 1);
    while (!g_tx->isCmdEmpty()){;}
    std::this_thread::sleep_for(std::chrono::microseconds(100));

    float voltageBot = fe->adcToV(this->ReadADC(6, false, fe));

    // Reset the Vref configuration
    fe->writeRegister(&Itkpixv2::VrefRsensTop, 0);
    fe->writeRegister(&Itkpixv2::VrefRsensBot, 0);
    while (!g_tx->isCmdEmpty()){;}
    std::this_thread::sleep_for(std::chrono::microseconds(100));

    logger->info("Polyresistor Temp Sensor: Bot({} V) Top ({} V) [Iref({} A)]", voltageTop, voltageBot, current);

    // Convert voltage difference into temperature. The sensors at top and bottom of the chip are good for providing relative measurements, not absolute one
    // TODO: verify calibration
    float tK = fe->vToTemp(voltageTop - voltageBot);
    if (in_kelvin) return tK;
    return tK - 273.15;
}

float Itkpixv2ReadRegLoop::ReadTransSensor(Itkpixv2 *fe, TransSensorLocation loc, TransSensorType type, Itkpixv2Cfg::TransSensor sensor, bool in_kelvin)
{
    //Sensor Config
    if (fe == NULL)
        return 0;

    // enable sensor, and switch off bias
    fe->writeRegister(TransSensorCfg[loc][0], 1);
    fe->writeRegister(TransSensorCfg[loc][2], 0);
    while (!g_tx->isCmdEmpty()){;}
    std::this_thread::sleep_for(std::chrono::microseconds(100));

    float VD = 0;
    // Loop over DEM and measure VMUX
    for (unsigned dem = 0; dem < 16; dem++)
    {
        fe->writeRegister(TransSensorCfg[loc][1], dem);
        while (!g_tx->isCmdEmpty()){;}
        std::this_thread::sleep_for(std::chrono::microseconds(100));
        // Readout VMUX
        VD += fe->adcToV(this->ReadADC(TransSensorVMUX[loc][type], false, fe));
    }

    // switch on bias and loop again
    fe->writeRegister(TransSensorCfg[loc][2], 1);
    while (!g_tx->isCmdEmpty()){;}
    std::this_thread::sleep_for(std::chrono::microseconds(100));

    float VDR = 0;
    // Loop over DEM and measure VMUX
    for (unsigned dem = 0; dem < 16; dem++)
    {
        fe->writeRegister(TransSensorCfg[loc][1], dem);
        while (!g_tx->isCmdEmpty()){;}
        std::this_thread::sleep_for(std::chrono::microseconds(100));
        // Readout VMUX
        VDR += fe->adcToV(this->ReadADC(TransSensorVMUX[loc][type], false, fe));
    }

    // Switch off sensor and bias
    fe->writeRegister(TransSensorCfg[loc][0], 0);
    fe->writeRegister(TransSensorCfg[loc][2], 0);
    while (!g_tx->isCmdEmpty()){;}
    std::this_thread::sleep_for(std::chrono::microseconds(100));

    float deltaV = (VDR - VD) / 16.;

    if (type == MOS)
        return fe->readMosTemp(deltaV, sensor, in_kelvin);
    else{
        // TODO: implement radiation sensor calibration
        return deltaV;
    }
}

void Itkpixv2ReadRegLoop::init()
{
    SPDLOG_LOGGER_TRACE(logger, "");
    m_done = false;

    if (m_STDReg.size() == 1 && m_STDReg[0] == "All")
    {
        m_STDReg.clear();
        for (std::pair<std::string, Itkpixv2RegDefault Itkpixv2GlobalCfg::*> tmpMap : keeper->globalFe<Itkpixv2>()->regMap)
        {
            m_STDReg.push_back(tmpMap.first);
        }
    }
}

void Itkpixv2ReadRegLoop::execPart1()
{
    //Scan chip by chip.
    for (unsigned id=0; id<keeper->getNumOfEntries(); id++) {
        auto fe = keeper->getFe(id);
        if (fe->getActive()) {
            g_tx->setCmdEnable(dynamic_cast<FrontEndCfg*>(fe)->getTxChannel());
            std::string feName = dynamic_cast<FrontEndCfg *>(fe)->getName();
            Itkpixv2 *feItkpixv2 = dynamic_cast<Itkpixv2 *>(fe);

            logger->info("Measuring for FE {} on Rx {}", feName, id);

            // Reading Standard Registers
            for (auto Reg : m_STDReg) {
                if (feItkpixv2->regMap.find(Reg) != feItkpixv2->regMap.end()) {
                    uint16_t RegisterVal = (feItkpixv2->*(feItkpixv2->regMap[Reg])).applyMask(feItkpixv2->readSingleRegister(feItkpixv2->regMap[Reg]));
                    logger->info("[{}][{}] REG: {}, Value: {}", id, feName, Reg, RegisterVal);

                    uint16_t StoredVal = (feItkpixv2->*(feItkpixv2->regMap[Reg])).read();

                    // Compare the Register with the stored value, it's a safety mechanism.
                    if (StoredVal != RegisterVal) {
                        logger->warn("[{}][{}] For Reg: {}, the stored register value ({}) doesn't match the one on the chip ({}).", id, feName, Reg, StoredVal, RegisterVal);
                    }
                } else {
                    logger->warn("[{}][{}] Requested Register {} not found, please check your runcard", id, feName, Reg);
                }
            }

            // Reading Voltage  ADC
            for (auto Reg : m_VoltMux) {
                uint16_t ADCVal = ReadADC(Reg, false, feItkpixv2);
                logger->info("[{}][{}] MON MUX_V: {}, Value: {} => {} V", id, feName, Reg, ADCVal, dynamic_cast<Itkpixv2 *>(fe)->adcToV(ADCVal));
            }

            // Reading Temperature sensors from the ADC
            for (auto Reg : m_TempSensors) {
                if (Reg == "NTC") {
                    float TempVal = ReadNTCTemp(feItkpixv2, false);
                    logger->info("[{}][{}] MON NTC: {} C", id, feName, TempVal);
                } else if (Reg == "Poly") {
                    float TempVal = ReadResistTemp(feItkpixv2, false);
                    logger->info("[{}][{}] MON poly resistor temperature sensor chip top minus bottom: {} C", id, feName, TempVal);
                } else if (Reg == "MOS") {
                    float TempValDSLDO = ReadTransSensor(feItkpixv2, DSLDO, MOS, Itkpixv2Cfg::DSLDO);
                    float TempValASLDO = ReadTransSensor(feItkpixv2, ASLDO, MOS, Itkpixv2Cfg::ASLDO);
                    float TempValACB = ReadTransSensor(feItkpixv2, ACB, MOS, Itkpixv2Cfg::ACB);
                    logger->info("[{}][{}] MON MOS temperature sensors digital SLDO: {} C, analog SLDO: {} C, ACB: {} C", id, feName, TempValDSLDO, TempValASLDO, TempValACB);
                }
            }

            for (auto Reg : m_RadSensors) {
                if (Reg == "BJT") {
                    float RadValDSLDO = ReadTransSensor(feItkpixv2, DSLDO, BJT, Itkpixv2Cfg::Other);
                    float RadValASLDO = ReadTransSensor(feItkpixv2, ASLDO, BJT, Itkpixv2Cfg::Other);
                    float RadValACB = ReadTransSensor(feItkpixv2, ACB, BJT, Itkpixv2Cfg::Other);
                    logger->info("[{}][{}] MON BJT radiation sensors digital SLDO: {}, analog SLDO: {}, ACB: {}", id, feName, RadValDSLDO, RadValASLDO, RadValACB);
                }
            }

            // Reading Current ADC
            for (auto Reg : m_CurMux) {
                uint16_t ADCVal = ReadADC(Reg, true, feItkpixv2);
                logger->info("[{}][{}] MON MUX_C: {} Value: {} => {} uA", id, feName, Reg, ADCVal, dynamic_cast<Itkpixv2 *>(fe)->adcToI(ADCVal)/1e-6);
            }

            // Need to run bank A and bank B separately. Global pulse can only drive one bank at a time
            // There are 42 oscillators in total, 8 from bank A, 34 from bank B
            double RingValuesSumA[8] = {0};
            double RingValuesSumSquaredA[8] = {0};

            if (m_EnblRingOscA > 0)
                logger->info("Starting Ring Osc in bank A ({} repetitions)", m_RingOscRep);

            // Enable Ring Osicilator in bank A
            // In m_EnblRingOsc, [0:7] are for bank A, and [8:12] are for bank B
            feItkpixv2->writeRegister(&Itkpixv2::RingOscAEn, (m_EnblRingOscA & 0xff));

            while (!g_tx->isCmdEmpty()){;}
            std::this_thread::sleep_for(std::chrono::microseconds(100));

            for (unsigned i = 0; i < m_RingOscRep; i++)
            {
                // Read bank A register values: there are 8 in total
                for (uint16_t tmpCount = 0; tmpCount < 8; tmpCount++)
                {
                    if (((m_EnblRingOscA >> tmpCount) & 0x1) == 0)
                        continue;
                    feItkpixv2->writeRegister(&Itkpixv2::RingOscARoute, tmpCount);
                    // Reset bank A counters
                    feItkpixv2->writeRegister(&Itkpixv2::RingOscAClear, 1);
                    feItkpixv2->writeRegister(&Itkpixv2::RingOscAClear, 0);
                    while (!g_tx->isCmdEmpty()){;}
                    std::this_thread::sleep_for(std::chrono::microseconds(100));

                    // Run oscillators for some time
                    feItkpixv2->runRingOsc(m_RingOscDur, false);

                    double value = feItkpixv2->readSingleRegister(&Itkpixv2::RingOscAOut) & 0xFFF;
                    RingValuesSumA[tmpCount] += value;
                    RingValuesSumSquaredA[tmpCount] += pow(value, 2);
                }
            }

            for (uint16_t tmpCount = 0; tmpCount < 8; tmpCount++)
            {
                if (((m_EnblRingOscA >> tmpCount) & 0x1) == 0)
                    continue;
                // Calculate average
                RingValuesSumA[tmpCount] /= (double)m_RingOscRep;
                // Calculate std dev
                if (m_RingOscRep > 1)
                {
                    RingValuesSumSquaredA[tmpCount] = sqrt((RingValuesSumSquaredA[tmpCount] - m_RingOscRep * pow(RingValuesSumA[tmpCount], 2)) / (double)(m_RingOscRep - 1));
                }
                else
                {
                    RingValuesSumSquaredA[tmpCount] = 0;
                }

                logger->info("[{}][{}] Bank A Ring Buffer: {} Values: {} +- {}", id, feName, tmpCount,
                             RingValuesSumA[tmpCount], RingValuesSumSquaredA[tmpCount]);
                logger->info("[{}][{}] Frequency: {} +- {} MHz", id, feName,
                             convertRingOscCntToMHz(RingValuesSumA[tmpCount]), convertRingOscCntToMHz(RingValuesSumSquaredA[tmpCount]));
            }

            // Now run ring oscillators in bank B
            double RingValuesSumB[34] = {0};
            double RingValuesSumSquaredB[34] = {0};            

            if (m_EnblRingOscB > 0)
                logger->info("Starting Ring Osc in bank B ({} repetitions)", m_RingOscRep);

            for (uint16_t tmpCount = 0; tmpCount < 5; tmpCount++)
            {
                if ((m_EnblRingOscB >> tmpCount) & 0x1)
                {
                    feItkpixv2->writeRegister(RingOscBEn[tmpCount], 1);
                }
            }
            while (!g_tx->isCmdEmpty()){;}
            std::this_thread::sleep_for(std::chrono::microseconds(100));
            
            for (unsigned i = 0; i < m_RingOscRep; i++)
            {
                for (uint16_t tmpCount = 0; tmpCount < 34; tmpCount++)
                {
                    if (((m_EnblRingOscB >> bankBEnableMap[tmpCount]) & 0x1) == 0)
                        continue;
                    feItkpixv2->writeRegister(&Itkpixv2::RingOscBRoute, tmpCount);
                    // Reset bank B counters
                    feItkpixv2->writeRegister(&Itkpixv2::RingOscBClear, 1);
                    feItkpixv2->writeRegister(&Itkpixv2::RingOscBClear, 0);
                    while (!g_tx->isCmdEmpty()){;}
                    std::this_thread::sleep_for(std::chrono::microseconds(100));

                    // Run oscillators for some time
                    feItkpixv2->runRingOsc(m_RingOscDur, true);

                    double value = feItkpixv2->readSingleRegister(&Itkpixv2::RingOscBOut) & 0xFFF;
                    RingValuesSumB[tmpCount] += value;
                    RingValuesSumSquaredB[tmpCount] += pow(value, 2);
                }
            }

            for (uint16_t tmpCount = 0; tmpCount < 34; tmpCount++)
            {
                if (((m_EnblRingOscB >> bankBEnableMap[tmpCount]) & 0x1) == 0)
                    continue;                
                // Calculate average
                RingValuesSumB[tmpCount] = RingValuesSumB[tmpCount] / (double)m_RingOscRep;
                // Calculate std dev
                if (m_RingOscRep > 1)
                {
                    RingValuesSumSquaredB[tmpCount] = sqrt((RingValuesSumSquaredB[tmpCount] - m_RingOscRep * pow(RingValuesSumB[tmpCount], 2)) / (double)(m_RingOscRep - 1));
                }
                else
                {
                    RingValuesSumSquaredB[tmpCount] = 0;
                }

                logger->info("[{}][{}] Bank B Ring Buffer: {} Values: {} +- {}", id, feName, tmpCount,
                            RingValuesSumB[tmpCount], RingValuesSumSquaredB[tmpCount]);
                logger->info("[{}][{}] Frequency: {} +- {} MHz", id, feName,
                            convertRingOscCntToMHz(RingValuesSumB[tmpCount]), convertRingOscCntToMHz(RingValuesSumSquaredB[tmpCount]));
            }
        }
    }
    dynamic_cast<HwController *>(g_rx)->runMode(); //This is needed to revert back the setupMode
}

void Itkpixv2ReadRegLoop::execPart2()
{
    m_done = true;
}

void Itkpixv2ReadRegLoop::end()
{
    // Reset to min
}

void Itkpixv2ReadRegLoop::writeConfig(json &config)
{
    config["EnblRingOscA"] = m_EnblRingOscA;
    config["EnblRingOscB"] = m_EnblRingOscB;
    config["RingOscDur"] = m_RingOscDur;
    config["RingOscRep"] = m_RingOscRep;
    config["Registers"] = m_STDReg;
    config["TempSensors"] = m_TempSensors;
    config["RadSensors"] = m_RadSensors;
    config["CurMux"] = m_CurMux;
    config["VoltMux"] = m_VoltMux; 
}

void Itkpixv2ReadRegLoop::loadConfig(const json &config)
{
    if (config.contains("EnblRingOscA"))
        m_EnblRingOscA = config["EnblRingOscA"];
    if (config.contains("EnblRingOscB"))
        m_EnblRingOscB = config["EnblRingOscB"];        
    if (config.contains("RingOscRep"))
        m_RingOscRep = config["RingOscRep"];
    if (config.contains("RingOscDur"))
        m_RingOscDur = config["RingOscDur"];

    if (config.contains("VoltMux"))
        for (auto Reg : config["VoltMux"])
            m_VoltMux.push_back(Reg);

    if (config.contains("CurMux"))
        for (auto Reg : config["CurMux"])
            m_CurMux.push_back(Reg);

    if (config.contains("Registers"))
        for (auto Reg : config["Registers"])
        {
            m_STDReg.push_back(Reg);

            // If Reg is ALL, instead loop over all registers
            if (Reg == "All")
            {
                m_STDReg.clear();
                m_STDReg.push_back(Reg);
                break;
            }
            if (Reg == "Null")
            {
                m_STDReg.clear();
                break;
            }
        }
    
    if (config.contains("TempSensors"))
        for (auto Reg : config["TempSensors"])
            m_TempSensors.push_back(Reg);

    if (config.contains("RadSensors"))
        for (auto Reg : config["RadSensors"])
            m_RadSensors.push_back(Reg);                
}
