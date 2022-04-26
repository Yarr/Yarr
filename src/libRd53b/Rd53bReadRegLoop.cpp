#include "Rd53bReadRegLoop.h"

#include "logging.h"

namespace
{
    auto logger = logging::make_log("Rd53bReadRegLoop");
}

Rd53bReadRegLoop::Rd53bReadRegLoop() : LoopActionBase(LOOP_STYLE_NOP)
{
    loopType = typeid(this);
    m_EnblRingOscA = 0;
    m_EnblRingOscB = 0;
    m_RingOscDur = 0;
    m_RingOscRep = 1;
}

uint16_t Rd53bReadRegLoop::ReadRegister(Rd53bReg Rd53bGlobalCfg::*ref, Rd53b *tmpFE = NULL)
{

    if (tmpFE == NULL)
        tmpFE = keeper->globalFe<Rd53b>();

    g_rx->flushBuffer();

    g_tx->setCmdEnable(dynamic_cast<FrontEndCfg *>(tmpFE)->getTxChannel());
    tmpFE->readRegister(ref);
    std::this_thread::sleep_for(std::chrono::microseconds(500));
    g_tx->setCmdEnable(keeper->getTxMask());

    std::vector<std::shared_ptr<RawData>> dataVec = g_rx->readData();
    std::shared_ptr<RawData> data;
    if (dataVec.size() > 0) {
        data = dataVec[0];
    }
    
    if (!data)
    {
        logger->warn("Warning!!! No Word Recieved in ReadRegister");
        return 0xffff;
    }

    unsigned size = data->getSize();
    logger->debug("Word size is: {}", size);
    for (unsigned c = 0; c < size / 2; c++)
    {
        if (c * 2 + 1 < size)
        {
            std::pair<uint32_t, uint32_t> readReg = Rd53b::decodeSingleRegRead(data->get(c * 2), data->get(c * 2 + 1));
            if (readReg.first == (tmpFE->*ref).addr())
            {
                return readReg.second;
            }
        }
        else
        {
            logger->warn("Warning!!! Halfword recieved in ADC Register Read {}", data->get(c * 2));
            return 0xffff;
        }
    }

    logger->warn("Warning!!! Requested Register {} not found in received words", (tmpFE->*ref).addr());
    return 0xffff;
}

//Configures the ADC, reads the register returns the first recieved register.
uint16_t Rd53bReadRegLoop::ReadADC(unsigned short Reg, bool doCur, Rd53b *tmpFE)
{

    if (tmpFE == NULL)
        tmpFE = keeper->globalFe<Rd53b>();

    g_tx->setCmdEnable(dynamic_cast<FrontEndCfg *>(tmpFE)->getTxChannel());
    tmpFE->confADC(Reg, doCur);
    g_tx->setCmdEnable(keeper->getTxMask());

    uint16_t RegVal = ReadRegister(&Rd53b::MonitoringDataAdc, dynamic_cast<Rd53b *>(tmpFE));

    return RegVal;
}

float Rd53bReadRegLoop::ReadNTCTemp(Rd53b *tmpFE, bool in_kelvin)
{
    //Sensor Config
    if (tmpFE == NULL)
        tmpFE = keeper->globalFe<Rd53b>();

    float voltage = tmpFE->ADCtoV(this->ReadADC(2, false, tmpFE));
    float current = tmpFE->ADCtoI(this->ReadADC(9, true, tmpFE));

    return tmpFE->readNTCTemp(voltage / current, in_kelvin);
}

float Rd53bReadRegLoop::ReadResistTemp(Rd53b *tmpFE, bool in_kelvin)
{
    //Sensor Config
    if (tmpFE == NULL)
        tmpFE = keeper->globalFe<Rd53b>();
    
    // Read top sensor: toggle Vref to top
    tmpFE->writeRegister(&Rd53b::VrefRsensTop, 1);
    tmpFE->writeRegister(&Rd53b::VrefRsensBot, 0);
    tmpFE->writeRegister(&Rd53b::VrefIn, 0);
    while (!g_tx->isCmdEmpty()){;}
    std::this_thread::sleep_for(std::chrono::microseconds(100));

    float voltageTop = tmpFE->ADCtoV(this->ReadADC(5, false, tmpFE));

    // Read bottom sensor: toggle Vref to bottom
    tmpFE->writeRegister(&Rd53b::VrefRsensTop, 0);
    tmpFE->writeRegister(&Rd53b::VrefRsensBot, 1);
    tmpFE->writeRegister(&Rd53b::VrefIn, 0);
    while (!g_tx->isCmdEmpty()){;}
    std::this_thread::sleep_for(std::chrono::microseconds(100));

    float voltageBot = tmpFE->ADCtoV(this->ReadADC(6, false, tmpFE));

    // Reset the Vref configuration
    tmpFE->writeRegister(&Rd53b::VrefRsensTop, 0);
    tmpFE->writeRegister(&Rd53b::VrefRsensBot, 0);
    tmpFE->writeRegister(&Rd53b::VrefIn, 1);
    while (!g_tx->isCmdEmpty()){;}
    std::this_thread::sleep_for(std::chrono::microseconds(100));

    // Convert voltage difference into temperature. The sensors at top and bottom of the chip are good for providing relative measurements, not absolute one
    // TODO: verify calibration
    float tK = tmpFE->VtoTemp(voltageTop - voltageBot);
    if (in_kelvin) return tK;
    return tK - 273.15;
}

float Rd53bReadRegLoop::ReadTransSensor(Rd53b *tmpFE, TransSensorLocation loc, TransSensorType type, bool in_kelvin)
{
    //Sensor Config
    if (tmpFE == NULL)
        tmpFE = keeper->globalFe<Rd53b>();

    // enable sensor, and switch off bias
    tmpFE->writeRegister(TransSensorCfg[loc][0], 1);
    tmpFE->writeRegister(TransSensorCfg[loc][2], 0);
    while (!g_tx->isCmdEmpty()){;}
    std::this_thread::sleep_for(std::chrono::microseconds(100));

    float VD = 0;
    // Loop over DEM and measure VMUX
    for (unsigned dem = 0; dem < 16; dem++)
    {
        tmpFE->writeRegister(TransSensorCfg[loc][1], dem);
        while (!g_tx->isCmdEmpty()){;}
        std::this_thread::sleep_for(std::chrono::microseconds(100));
        // Readout VMUX
        VD += tmpFE->ADCtoV(this->ReadADC(TransSensorVMUX[loc][type], false, tmpFE));
    }

    // switch on bias and loop again
    tmpFE->writeRegister(TransSensorCfg[loc][2], 1);
    while (!g_tx->isCmdEmpty()){;}
    std::this_thread::sleep_for(std::chrono::microseconds(100));

    float VDR = 0;
    // Loop over DEM and measure VMUX
    for (unsigned dem = 0; dem < 16; dem++)
    {
        tmpFE->writeRegister(TransSensorCfg[loc][1], dem);
        while (!g_tx->isCmdEmpty()){;}
        std::this_thread::sleep_for(std::chrono::microseconds(100));
        // Readout VMUX
        VDR += tmpFE->ADCtoV(this->ReadADC(TransSensorVMUX[loc][type], false, tmpFE));
    }

    // Switch off sensor and bias
    tmpFE->writeRegister(TransSensorCfg[loc][0], 0);
    tmpFE->writeRegister(TransSensorCfg[loc][2], 0);
    while (!g_tx->isCmdEmpty()){;}
    std::this_thread::sleep_for(std::chrono::microseconds(100));

    float deltaV = (VDR - VD) / 16.;

    if (type == MOS)
        return tmpFE->readMOSTemp(deltaV, in_kelvin);
    else{
        // TODO: implement radiation sensor calibration
        return deltaV;
    }
}

void Rd53bReadRegLoop::init()
{
    SPDLOG_LOGGER_TRACE(logger, "");
    m_done = false;

    if (m_STDReg.size() == 1 && m_STDReg[0] == "All")
    {
        m_STDReg.clear();
        for (std::pair<std::string, Rd53bReg Rd53bGlobalCfg::*> tmpMap : keeper->globalFe<Rd53b>()->regMap)
        {
            m_STDReg.push_back(tmpMap.first);
        }
    }
}

void Rd53bReadRegLoop::execPart1()
{
    dynamic_cast<HwController *>(g_rx)->setupMode(); //This is need to make sure the global pulse doesn't refresh the ADCRegister

    //Currently for Rd53b, each board needs to be configured alone due to a bug with the read out of the Rd53b. This can be changed in rd53b.
    for (auto *fe : keeper->feList)
    {
        if (fe->getActive())
        {
            std::string feName = dynamic_cast<FrontEndCfg *>(fe)->getName();
            int feChannel = dynamic_cast<FrontEndCfg *>(fe)->getRxChannel();
            Rd53b *feRd53b = dynamic_cast<Rd53b *>(fe);

            logger->info("Measuring for FE {} on Rx {}", feName, feChannel);

            // Reading Standard Registers
            for (auto Reg : m_STDReg)
            {
                if (keeper->globalFe<Rd53b>()->regMap.find(Reg) != keeper->globalFe<Rd53b>()->regMap.end())
                {
                    uint16_t RegisterVal = (feRd53b->*(feRd53b->regMap[Reg])).applyMask(ReadRegister(keeper->globalFe<Rd53b>()->regMap[Reg], feRd53b));
                    logger->info("[{}][{}] REG: {}, Value: {}", feChannel, feName, Reg, RegisterVal);

                    uint16_t StoredVal = (feRd53b->*(feRd53b->regMap[Reg])).read();

                    // Compare the Register with the stored value, it's a safety mechanism.
                    if (StoredVal != RegisterVal)
                    {
                        logger->warn("[{}][{}] For Reg: {}, the stored register value ({}) doesn't match the one on the chip ({}).", feChannel, feName, Reg, StoredVal, RegisterVal);
                    }
                }
                else
                    logger->warn("[{}][{}] Requested Register {} not found, please check your runcard", feChannel, feName, Reg);
            }

            // Reading Voltage  ADC
            for (auto Reg : m_VoltMux)
            {
                uint16_t ADCVal = ReadADC(Reg, false, feRd53b);
                logger->info("[{}][{}] MON MUX_V: {}, Value: {} => {} V", feChannel, feName, Reg, ADCVal, dynamic_cast<Rd53b *>(fe)->ADCtoV(ADCVal));
            }

            // Reading Temperature sensors from the ADC
            for (auto Reg : m_TempSensors)
            {
                if (Reg == "NTC")
                {
                    float TempVal = ReadNTCTemp(feRd53b);
                    logger->info("[{}][{}] MON NTC: {} C", feChannel, feName, TempVal);
                }
                else if (Reg == "Resistor")
                {
                    float TempVal = ReadResistTemp(feRd53b);
                    logger->info("[{}][{}] MON poly resistor temperature sensor chip top minus bottom: {} C", feChannel, feName, TempVal);
                }
                else if (Reg == "MOS")
                {
                    float TempValDSLDO = ReadTransSensor(feRd53b, DSLDO, MOS);
                    float TempValASLDO = ReadTransSensor(feRd53b, ASLDO, MOS);
                    float TempValACB = ReadTransSensor(feRd53b, ACB, MOS);
                    logger->info("[{}][{}] MON MOS temperature sensors digital SLDO: {} C, analog SLDO: {} C, ACB: {} C", feChannel, feName, TempValDSLDO, TempValASLDO, TempValACB);
                }
            }

            for (auto Reg : m_RadSensors)
            {
                if (Reg == "BJT")
                {
                    float RadValDSLDO = ReadTransSensor(feRd53b, DSLDO, BJT);
                    float RadValASLDO = ReadTransSensor(feRd53b, ASLDO, BJT);
                    float RadValACB = ReadTransSensor(feRd53b, ACB, BJT);
                    logger->info("[{}][{}] MON BJT radiation sensors digital SLDO: {}, analog SLDO: {}, ACB: {}", feChannel, feName, RadValDSLDO, RadValASLDO, RadValACB);
                }
            }

            // Reading Current ADC
            for (auto Reg : m_CurMux)
            {
                uint16_t ADCVal = ReadADC(Reg, true, feRd53b);
                logger->info("[{}][{}] MON MUX_C: {} Value: {} => {} uA", feChannel, feName, Reg, ADCVal, dynamic_cast<Rd53b *>(fe)->ADCtoI(ADCVal)/1e-6);
            }

            // Need to run bank A and bank B separately. Global pulse can only drive one bank at a time
            // There are 42 oscillators in total, 8 from bank A, 34 from bank B
            double RingValuesSumA[8] = {0};
            double RingValuesSumSquaredA[8] = {0};

            if (m_EnblRingOscA > 0)
                logger->info("Starting Ring Osc in bank A ({} repetitions)", m_RingOscRep);

            // Enable Ring Osicilator in bank A
            // In m_EnblRingOsc, [0:7] are for bank A, and [8:12] are for bank B
            feRd53b->writeRegister(&Rd53b::RingOscAEn, (m_EnblRingOscA & 0xff));

            while (!g_tx->isCmdEmpty()){;}
            std::this_thread::sleep_for(std::chrono::microseconds(100));

            for (unsigned i = 0; i < m_RingOscRep; i++)
            {
                // Read bank A register values: there are 8 in total
                for (uint16_t tmpCount = 0; tmpCount < 8; tmpCount++)
                {
                    if (((m_EnblRingOscA >> tmpCount) & 0x1) == 0)
                        continue;
                    feRd53b->writeRegister(&Rd53b::RingOscARoute, tmpCount);
                    // Reset bank A counters
                    feRd53b->writeRegister(&Rd53b::RingOscAClear, 1);
                    feRd53b->writeRegister(&Rd53b::RingOscAClear, 0);
                    while (!g_tx->isCmdEmpty()){;}
                    std::this_thread::sleep_for(std::chrono::microseconds(100));

                    // Run oscillators for some time
                    feRd53b->runRingOsc(m_RingOscDur, false);

                    double value = ReadRegister(&Rd53b::RingOscAOut, dynamic_cast<Rd53b *>(fe)) & 0xFFF;
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

                logger->info("[{}][{}] Bank A Ring Buffer: {} Values: {} +- {}", feChannel, feName, tmpCount,
                             RingValuesSumA[tmpCount], RingValuesSumSquaredA[tmpCount]);
                logger->info("[{}][{}] Frequency: {} +- {} MHz", feChannel, feName,
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
                    feRd53b->writeRegister(RingOscBEn[tmpCount], 1);
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
                    feRd53b->writeRegister(&Rd53b::RingOscBRoute, tmpCount);
                    // Reset bank B counters
                    feRd53b->writeRegister(&Rd53b::RingOscBClear, 1);
                    feRd53b->writeRegister(&Rd53b::RingOscBClear, 0);
                    while (!g_tx->isCmdEmpty()){;}
                    std::this_thread::sleep_for(std::chrono::microseconds(100));

                    // Run oscillators for some time
                    feRd53b->runRingOsc(m_RingOscDur, true);

                    double value = ReadRegister(&Rd53b::RingOscBOut, dynamic_cast<Rd53b *>(fe)) & 0xFFF;
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

                logger->info("[{}][{}] Bank B Ring Buffer: {} Values: {} +- {}", feChannel, feName, tmpCount,
                            RingValuesSumB[tmpCount], RingValuesSumSquaredB[tmpCount]);
                logger->info("[{}][{}] Frequency: {} +- {} MHz", feChannel, feName,
                            convertRingOscCntToMHz(RingValuesSumB[tmpCount]), convertRingOscCntToMHz(RingValuesSumSquaredB[tmpCount]));
            }
        }
    }
    dynamic_cast<HwController *>(g_rx)->runMode(); //This is needed to revert back the setupMode
}

void Rd53bReadRegLoop::execPart2()
{
    m_done = true;
}

void Rd53bReadRegLoop::end()
{
    // Reset to min
}

void Rd53bReadRegLoop::writeConfig(json &config)
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

void Rd53bReadRegLoop::loadConfig(const json &config)
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
