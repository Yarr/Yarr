// #################################
// # Author: Ismet Siral
// # Email: ismet.siral at cern.ch
// # Project: Yarr
// # Description: Read Register Loop for RD53A
// # Date: 03/2018
// ################################

#include "Rd53aReadRegLoop.h"

#include "logging.h"

namespace {
    auto logger = logging::make_log("Rd53aReadRegLoop");
}

Rd53aReadRegLoop::Rd53aReadRegLoop() : LoopActionBase(LOOP_STYLE_NOP) {
    loopType = typeid(this);
    m_EnblRingOsc=0;
    m_RingOscDur=0;
    m_RingOscRep=1;
}

uint16_t Rd53aReadRegLoop::ReadRegister(Rd53Reg Rd53aGlobalCfg::*ref, Rd53a *tmpFE = NULL) {

    if(tmpFE==NULL)
        tmpFE= keeper->globalFe<Rd53a>();

    g_rx->flushBuffer();

    g_tx->setCmdEnable(dynamic_cast<FrontEndCfg*>(tmpFE)->getTxChannel());
    tmpFE->readRegister(ref);
    std::this_thread::sleep_for(std::chrono::microseconds(500));
    g_tx->setCmdEnable(keeper->getTxMask());

    std::vector<RawDataPtr> dataVec = g_rx->readData();
    RawDataPtr data;
    if (dataVec.size() > 0) {
        data = dataVec[0];
    }
    
    if(data != nullptr)
    {
        logger->warn("Warning!!!, No Word Recieved in ReadRegister");
        return 65535;
    }

    unsigned size =  data->getSize();
    logger->debug("Word size is: {}", size);
    for(unsigned c=0; c<size/2; c++)
    {
        if (c*2+1<size) {
            std::pair<uint32_t, uint32_t> readReg = Rd53a::decodeSingleRegRead(data->get(c*2),data->get(c*2+1));	    
            if ( readReg.first==(tmpFE->*ref).addr()) {
                return readReg.second;
            }
        }
        else {
            logger->warn("Warning!!! Halfword recieved in ADC Register Read {}", data->get(c*2));
            return 65535;
        }
    }

    logger->warn("Warning!!! Requested Register {} not found in received words", (tmpFE->*ref).addr()); 
    return 65535;
}

//Configures the ADC, reads the register returns the first recieved register.
uint16_t Rd53aReadRegLoop::ReadADC(unsigned short Reg,  bool doCur=false,  Rd53a *tmpFE = NULL) {

    if(tmpFE==NULL)
        tmpFE= keeper->globalFe<Rd53a>();

    g_tx->setCmdEnable(dynamic_cast<FrontEndCfg*>(tmpFE)->getTxChannel());
    tmpFE->confADC(Reg,doCur);
    g_tx->setCmdEnable(keeper->getTxMask());

    uint16_t RegVal =  ReadRegister((Rd53Reg Rd53aGlobalCfg::*) &Rd53a::AdcRead, dynamic_cast<Rd53a*>(tmpFE));


    return RegVal;

}

//Runs readADC twice, for two difference bias configurations in the temp/rad sensors. Returns the difference to the user. 
std::pair<uint16_t,uint16_t> Rd53aReadRegLoop::ReadTemp(unsigned short Reg, Rd53a *tmpFE = NULL) {

    //Sensor Config
    if(tmpFE==NULL)
        tmpFE= keeper->globalFe<Rd53a>();



    uint16_t ADCVal[2] = {0,0};
    for(unsigned curConf=0; curConf<2; curConf++)
    {


        uint16_t SensorConf99 = (0x20 + 1*curConf) * ( Reg==3 || Reg==4) //Tmp/Rad Sensor 1
            + (0x800 + curConf*0x40) * (Reg==5 || Reg==6); //Tmp/Rad Sensor 2
        uint16_t SensorConf100 = (0x20 + 1*curConf) * ( Reg==14 || Reg==15) //Tmp/Rad Sensor 3
            + (0x800 + curConf*0x40) * (Reg==7 || Reg==8);  //Tmp/Rad Sensor 4

        g_tx->setCmdEnable(dynamic_cast<FrontEndCfg*>(tmpFE)->getTxChannel());    
        if(Reg==3 || Reg==4 || Reg==5 || Reg==6)
            tmpFE->writeRegister(&Rd53a::SensorCfg0, SensorConf99);
        else if(Reg==7 || Reg==8 || Reg==14 || Reg==15)
            tmpFE->writeRegister(&Rd53a::SensorCfg1, SensorConf100);
        g_tx->setCmdEnable(keeper->getTxMask());
        tmpFE->idle();
        g_tx->setCmdEnable(dynamic_cast<FrontEndCfg*>(tmpFE)->getTxChannel());    


        ADCVal[curConf]=this->ReadADC(Reg,false,dynamic_cast<Rd53a*>(tmpFE));

    }

    return std::pair<uint16_t,uint16_t> (ADCVal[0],ADCVal[1]);
}

void Rd53aReadRegLoop::init() {
    SPDLOG_LOGGER_TRACE(logger, "");
    m_done = false;

    //Enables Ring Oscillators to be triggered later
    keeper->globalFe<Rd53a>()->writeRegister(&Rd53a::RingOscEn, m_EnblRingOsc);

    if(m_STDReg.size()==1 && m_STDReg[0]=="All"){
        m_STDReg.clear();
        for (std::pair<std::string, Rd53Reg Rd53aGlobalCfg::*> tmpMap : keeper->globalFe<Rd53a>()->regMap) {
            m_STDReg.push_back(tmpMap.first);
        }
    }
}

void Rd53aReadRegLoop::execPart1() {
    dynamic_cast<HwController*>(g_rx)->setupMode(); //This is need to make sure the global pulse doesn't refresh the ADCRegister

    //Currently for RD53A, each board needs to be configured alone due to a bug with the read out of the rd53a. This can be changed in rd53b. 
    
    for(unsigned id=0; id<keeper->getNumOfEntries(); id++) {
        auto fe = keeper->getFe(id);
        if(fe->getActive()) {
            std::string feName = dynamic_cast<FrontEndCfg*>(fe)->getName();
            Rd53a *feRd53a = dynamic_cast<Rd53a*>(fe);

            logger->info("Measuring for FE {} on Id {}", feName, id);

            // Reading Standard Registers
            for (auto Reg : m_STDReg) {
                if (keeper->globalFe<Rd53a>()->regMap.find(Reg) != keeper->globalFe<Rd53a>()->regMap.end()) {
                    uint16_t RegisterVal =  (feRd53a->*(feRd53a->regMap[Reg])).applyMask( ReadRegister( keeper->globalFe<Rd53a>()->regMap[Reg] , feRd53a));
                    logger->info("[{}][{}] REG: {}, Value: {}", id, feName, Reg, RegisterVal);

                    uint16_t StoredVal = (feRd53a->*(feRd53a->regMap[Reg])).read();

                    // Compare the Register with the stored value, it's a safety mechanism. 
                    if ( StoredVal!=RegisterVal) {
                       logger->warn("[{}][{}] For Reg: {}, the stored register value ({}) doesn't match the one on the chip ({}).", id, feName, Reg, StoredVal, RegisterVal);
                    }
                    
                }
                else
                    logger->warn("[{}][{}] Requested Register {} not found, please check your runcard", id, feName, Reg);

            }

            // Reading Voltage  ADC 
            for( auto Reg : m_VoltMux) {
                uint16_t ADCVal = ReadADC(Reg, false, dynamic_cast<Rd53a*>(fe));
                logger->info("[{}][{}] MON MUX_V: {}, Value: {} => {} V", id, feName, Reg, ADCVal,dynamic_cast<Rd53a*>(fe)->ADCtoV(ADCVal));
            }

            // Reading Temp or Rad sensors from the ADC
            for( auto Reg : m_TempMux) {
                std::pair<uint16_t, uint16_t> TempVal = ReadTemp(Reg, dynamic_cast<Rd53a*>(fe));

                uint16_t Sensor=0;
                bool isRadSensor=false;
                switch(Reg) {
                    case 3:
                        Sensor=0;
                        isRadSensor=false;
                        break;
                    case 4:
                        Sensor=0;
                        isRadSensor=true;
                        break;
                    case 5:
                        Sensor=1;
                        isRadSensor=false;
                        break;
                    case 6:
                        Sensor=1;
                        isRadSensor=true;
                        break;
                    case 7:
                        Sensor=2;
                        isRadSensor=false;
                        break;
                    case 8:
                        Sensor=2;
                        isRadSensor=true;
                        break;
                    case 14:
                        Sensor=3;
                        isRadSensor=true;
                        break;
                    case 15:
                        Sensor=3;
                        isRadSensor=false;
                        break;
                    default :
                        Sensor=0;
                        isRadSensor=false;
                        break;
                }

                float Volt1 = dynamic_cast<Rd53a*>(fe)->ADCtoV(TempVal.first);
                float Volt2 = dynamic_cast<Rd53a*>(fe)->ADCtoV(TempVal.second);

                logger->info("[{}][{}] MON MUX_V: {} Bias 0 {} -> {} V Bias 1 {} -> {} V, Temperature {} -> {} C", id, feName, Reg, TempVal.first, Volt1, TempVal.second, Volt2, Volt2-Volt1, (feRd53a->VtoTemp(Volt2-Volt1, Sensor, isRadSensor)));      
            }

            // Reading Current ADC
            for( auto Reg : m_CurMux)	{
                uint16_t ADCVal = ReadADC(Reg, true, dynamic_cast<Rd53a*>(fe));
                logger->info("[{}][{}] MON MUX_C: {} Value: {}", id, feName, Reg, ADCVal);
            }

            // Running the Ring Oscillators
            double RingValuesSum[8] = {0};
            double RingValuesSumSquared[8] = {0};


            if (m_EnblRingOsc > 0)
                logger->info("Starting Ring Osc ({} repetitions)", m_RingOscRep);
            
            for (unsigned i=0; i<m_RingOscRep; i++) { 
                //Reset Ring Osicilator
                for(uint16_t tmpCount = 0; tmpCount<8; tmpCount++) {
                    if ((m_EnblRingOsc >> tmpCount) & 0x1) {    
                        keeper->globalFe<Rd53a>()->writeRegister(OscRegisters[tmpCount],0);
                    }
                }
                while(!g_tx->isCmdEmpty()) {}

                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                
                // Run oscillators for some time
                keeper->globalFe<Rd53a>()->runRingOsc(m_RingOscDur);
                while(!g_tx->isCmdEmpty()) {}

                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                
                // Read value after running them
                for(uint16_t tmpCount = 0; tmpCount<8; tmpCount++) {    
                    if ((m_EnblRingOsc >> tmpCount) & 0x1) {
                        double value = ReadRegister(OscRegisters[tmpCount],dynamic_cast<Rd53a*>(fe)) & 0xFFF ;
                        RingValuesSum[tmpCount] += value;
                        RingValuesSumSquared[tmpCount] += pow(value, 2);
                    } 
                }
            }
            
            for(uint16_t tmpCount = 0; tmpCount<8; tmpCount++) {    
                if ((m_EnblRingOsc >> tmpCount) & 0x1) {
                    // Calculate average
                    RingValuesSum[tmpCount] = RingValuesSum[tmpCount]/(double)m_RingOscRep;
                    // Calculate std dev
                    if (m_RingOscRep > 1) {
                        RingValuesSumSquared[tmpCount] = sqrt((RingValuesSumSquared[tmpCount] - m_RingOscRep*pow(RingValuesSum[tmpCount], 2))/(double)(m_RingOscRep -1));
                    } else {
                        RingValuesSumSquared[tmpCount] = 0;
                    }

                    logger->info("[{}][{}] Ring Buffer: {} Values: {} +- {}", id, feName, tmpCount, 
                            RingValuesSum[tmpCount], RingValuesSumSquared[tmpCount]);  
                    logger->info("[{}][{}] Frequency: {} +- {} MHz", id, feName, 
                            RingValuesSum[tmpCount]/(1<<m_RingOscDur)/6.25e-3, RingValuesSumSquared[tmpCount]/(1<<m_RingOscDur)/6.25e-3);
                }
            }
        }
    }
    dynamic_cast<HwController*>(g_rx)->runMode(); //This is needed to revert back the setupMode
}

void Rd53aReadRegLoop::execPart2() {
    m_done = true;
}

void Rd53aReadRegLoop::end() {
    // Reset to min
}

void Rd53aReadRegLoop::writeConfig(json &config) {
    config["EnblRingOsc"] = m_EnblRingOsc;
    config["RingOscDur"] = m_RingOscDur;
    config["RingOscRep"] = m_RingOscRep;
    config["CurMux"] = m_CurMux;
    config["Registers"] = m_STDReg;

    //Just joining back the two STD vectors.
    std::vector<uint16_t> SendBack;
    SendBack.reserve(m_VoltMux.size()+m_TempMux.size());
    SendBack.insert(SendBack.end(),m_VoltMux.begin(),m_VoltMux.end());
    SendBack.insert(SendBack.end(),m_TempMux.begin(),m_TempMux.end());
    config["VoltMux"] = SendBack;
}

void Rd53aReadRegLoop::loadConfig(const json &config) {
    if (config.contains("EnblRingOsc"))
        m_EnblRingOsc = config["EnblRingOsc"];
    if (config.contains("RingOscRep"))
        m_RingOscRep = config["RingOscRep"];
    if (config.contains("RingOscDur")) {
        m_RingOscDur = config["RingOscDur"];
        if (m_RingOscDur > 9){
            logger->error("Global Max duration is 2^9 = 512 cycles, setting the RingOscDur to Max (9)");
            m_RingOscDur=9;
        }
    }

    if (config.contains("VoltMux"))
        for(auto Reg : config["VoltMux"])
        {
            if( ( int(Reg) >=3 && int(Reg)<=8) || (int(Reg)>=14 && int(Reg)<=15) )
                m_TempMux.push_back(Reg);
            else
                m_VoltMux.push_back(Reg);
        }
    if (config.contains("CurMux"))
        for(auto Reg : config["CurMux"])
            m_CurMux.push_back(Reg);

    if (config.contains("Registers"))
        for (auto Reg: config["Registers"]) {
            m_STDReg.push_back(Reg);

            // If Reg is ALL, instead loop over all registers
            if (Reg=="All"){
                m_STDReg.clear();
                m_STDReg.push_back(Reg);
                break;
            }
            if (Reg=="Null") {
                m_STDReg.clear();
                break;
            }
        }
}
