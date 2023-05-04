// #################################
// # Author: Timon Heim, Magne Lauritzen and Simon Huiberts
// # Email: timon.heim at cern.ch, magne.eik.laurizen at cern.ch, simon.kristian.huiberts at cern.ch
// # Project: Yarr
// # Description: Two Trigger Loop for RD53A
// # Date: 05/2020
// ################################

#include "Rd53a2TriggerLoop.h"
#include <bitset>
#include <algorithm>
#include <cmath>


#include "logging.h"

namespace {
  auto logger = logging::make_log("Rd53a2TriggerLoop");
}

Rd53a2TriggerLoop::Rd53a2TriggerLoop() : LoopActionBase(LOOP_STYLE_TRIGGER) {
    setTrigCnt(50);
    m_trigDelay = 48;
    m_trigDelay2 = m_trigDelay;
    m_trigFreq = 1e3;
    m_trigTime = 10;
    m_trigWordLength = 32;
    m_doubleDelay = 80;
    m_Ntrig1 = 16;
    m_Ntrig2 = 16;
    m_sendEcr = false;
    m_trigWord.fill(0x69696969);
    m_trigPulses.fill(0x00);
    m_noInject = false;
    m_noInject2 = false;
    m_extTrig = false;
    m_synPulse = false; //Whether to add a synchronous autozero pulse to the end of the buffer

    min = 0;
    max = 0;
    step = 1;

    isInner = false;
    loopType = typeid(this);
}

 void Rd53a2TriggerLoop::setNoInject() {
     m_trigWord.fill(0x69696969);
  }

void Rd53a2TriggerLoop::singleCmdInject(){
    //Performs double injection with a single inject command. Works up to doubleDelay = 15.75 bunch crossings.
    //The user have to type in half number values for double delay values < 16 (fix is to floor m_doubleDelay)
    uint8_t of = 3+(std::max(float(m_trigDelay+16-CMDDEL + m_Ntrig1), m_doubleDelay + m_trigDelay2+16-CMDDEL + m_Ntrig2)-1)/8; //Calculate trigWord offset, i.e. at which index the header cmd is to be inserted.
    m_trigWord[of] = 0x69696363; //Populate header
    m_edgeDuration = int(m_doubleDelay*4); //cal_edge duration (160MHz) is 4 times doubledelay (40MHz)
    if(m_noInject2){
        m_trigWord[of-1] = Rd53aCmd::genCal(8, 0, 9-CMDDEL, 1, 0, 1); // Inject cmd
        flexibleTrigger(of, 8+std::round(float(m_edgeDuration)/4), m_Ntrig2, 0); //Trigger on injection #2

    }else{
        m_trigWord[of-1] = Rd53aCmd::genCal(8, 1, 9-CMDDEL, m_edgeDuration, 1, 4*(9-CMDDEL)+1); // Inject cmd
        flexibleTrigger(of, 8+std::round(float(m_edgeDuration)/4), m_Ntrig2, m_trigDelay2); //Trigger on injection #2

    }
    flexibleTrigger(of, 8, m_Ntrig1, m_trigDelay); //Trigger on injection #1
}


void Rd53a2TriggerLoop::doubleCmdInject(){
    //Performs double injection with two inject commands
    //**Second calibration command population**
    uint8_t of = (m_trigDelay2 + m_Ntrig2)/8;
    if(m_noInject2){
        flexibleTrigger(2.5 + of, 0, m_Ntrig2, 0);
    }else{
        m_trigWord[3 + of] = 0x69696363; //Second calibration command header
        m_trigWord[2 + of] = Rd53aCmd::genCal(8, 1, 0, 0, 1, 0);
        flexibleTrigger(3.5 + of, 0, m_Ntrig2, m_trigDelay2); //Trigger on injection #2
    }


    //**First calibration command population**
    m_edgeDelay = 7-(int(m_doubleDelay)-(9-CMDDEL))%8;
    m_auxDelay = m_edgeDelay*4 + 2;
    of = 5 + (int(m_doubleDelay)-(9-CMDDEL))/8 + of; //Calculate offset, i.e. at which index the header cmd is to be inserted.
    if(of > m_trigWordLength-1-m_synPulse){
      SPDLOG_LOGGER_ERROR(logger, "DoubleDelay is too large (Burst buffer words: "+std::to_string(of + 1 + m_synPulse)+").");
    }

    flexibleTrigger(of, m_edgeDelay + CMDDEL-1, m_Ntrig1, m_trigDelay); //Flexible trigger for first injection of double injection scheme
    m_trigWord[of] = 0x69696363; //First calibration command header //PRINTED OUT ()COMES FIRST
    if(m_noInject2){
        m_trigWord[of-1] = Rd53aCmd::genCal(8, 0, m_edgeDelay, 1, 1, m_auxDelay); // first calibration command
    }else{
        m_trigWord[of-1] = Rd53aCmd::genCal(8, 0, m_edgeDelay, 1, 1, m_auxDelay); // first calibration command
    }
}


void Rd53a2TriggerLoop::verifyParameters(){
    if(m_doubleDelay>=16){
        if (!((m_trigDelay >= 8) && (m_trigDelay2 >=8) && (m_trigDelay + m_trigDelay2 <= 168))) {
          SPDLOG_LOGGER_ERROR(logger, "Both delay and delay2 must be a value equal or greater than 8, and their sum must not exceed 168.");
        }

        uint8_t minimumDelay = findSmallestDelay();
        if (minimumDelay>m_doubleDelay){
          SPDLOG_LOGGER_ERROR(logger, "For your choice of Ntrig and delay, doubleDelay ("+std::to_string(m_doubleDelay)+") cannot be less than " + std::to_string(minimumDelay));
        }

        uint8_t maxDelay = greatestDelay();
        if (maxDelay<m_doubleDelay){
          SPDLOG_LOGGER_ERROR(logger, "For your choice of Ntrig2 and delay2, doubleDelay ("+std::to_string(m_doubleDelay)+") cannot be greater than " + std::to_string(maxDelay));
        }
    }

    if (!((m_Ntrig2<=16) && (m_Ntrig1<=16))){
      SPDLOG_LOGGER_ERROR(logger, "Number of triggers (Ntrig1 and Ntrig2) must both be equal or less than 16.");
    }
}


void Rd53a2TriggerLoop::flexibleTrigger(uint8_t offset, int injDelay, int triggers, int delay){
    //Calculates and sets a train of trigger pulses that can begin and end at arbitrary locations.
    //injDelay : Delta T in bunch crossings between beginning of trigWord frame containing injection command and the pulse itself.
    //Offset: Index of the frame containing the header of the calibration command the triggers are related to.
    //triggers: Number of trigger pulses in bunch crossings
    //delay: delay between injection and beginning of trigger pulse train.
    if(triggers==0) return;
    uint16_t empty_trigs = delay + injDelay-8; //
    uint8_t empty_frames = empty_trigs/8; //Number of empty trigword frames to pad
    uint8_t trig_start_i = empty_trigs%8; //Index of the first trigger pulse in first frame containing trigger pulses
    int full_frames = floor(float(triggers-(8-trig_start_i)-1)/8); //Number of trigword frames filled with 8 trigger pulses
    uint8_t trig_end_i = (empty_trigs+triggers-1)%8; //Index of the last trigger pulse in the last trigword frame containing trigger pulses
    for(uint8_t n=0; n<full_frames+2; n++){
        uint8_t trig_pattern = 0xFF;
        if(n==0) trig_pattern = trig_pattern << trig_start_i;
        if(n==full_frames+1) trig_pattern = trig_pattern & (0xFF >> (7-trig_end_i));
        uint8_t framei = offset-2-empty_frames-n;
        trig_pattern = m_trigPulses[framei] | trig_pattern;
        m_trigWord[framei] = Rd53aCmd::genTrigger(trig_pattern&0xF, 2*n+1, trig_pattern>>4, 2*n+2);
        m_trigPulses[framei] = trig_pattern;
    }
}

uint8_t Rd53a2TriggerLoop::findSmallestDelay() const{
    //The smallest allowable doubledelay in a two-command scheme depends on several variables and is not trivial to express analytically.
    //This function emulates setFlexibleTrigger to find the smallest doubledelay that will not cause overlap between the first trigger frames and the second injection header frame.
    uint8_t empty_frames;
    int8_t full_frames;
    int8_t k = -10; //Start with a low value
    if(m_Ntrig1==0){return 8-CMDDEL;}
    int injDelay;
    for(int n=0; n<8; n++){
        injDelay = n + CMDDEL-1;
        empty_frames = (m_trigDelay + injDelay-9)/8;
        full_frames = (m_Ntrig1-(8-(m_trigDelay + injDelay-9)%8)-1)/8;
        if(empty_frames+full_frames>k){
            k = empty_frames+full_frames;
        }
    }
    return (k+4)*8 - injDelay;
}

uint8_t Rd53a2TriggerLoop::greatestDelay() const{
    //Calculates greatest allowed delay between the two injections
    return (m_trigWordLength-1-m_synPulse - 5)*8-((m_trigDelay2+m_Ntrig2)/8)*8-CMDDEL+9+7;
}

void Rd53a2TriggerLoop::setAutozeroPulse(){
    //Shift array by 1
    for(uint n=1; n<=m_trigWordLength-1; n++){
        m_trigWord[m_trigWordLength-n] = m_trigWord[m_trigWordLength-n-1];
    }
    m_trigWord[0] = 0x5c5c0000 + (Rd53aCmd::encode5to8(0x8<<1)<<8) + (Rd53aCmd::encode5to8(8<<1)); // global pulse for sync FE
}
//Replance all _Pretty_Functions
void Rd53a2TriggerLoop::init(){
    SPDLOG_LOGGER_TRACE(logger, "");
    m_done = false;
    this->verifyParameters();
    m_trigWord.fill(0x69696969);
    m_trigPulses.fill(0x00);

    //Inject
    if(m_doubleDelay<16){
        this->singleCmdInject();
    }else{
        this->doubleCmdInject();
    }
    //rearm
    m_trigWord[1] = 0x69696363;
    m_trigWord[0] = Rd53aCmd::genCal(8, 1, 0, 0, 0, 0); // Arm inject

    if(m_synPulse){
        this->setAutozeroPulse();
    }
    if (m_extTrig) {
      g_tx->setTrigConfig(EXT_TRIGGER);
    } else if (getTrigCnt() > 0) {
      g_tx->setTrigConfig(INT_COUNT);
    } else {
      g_tx->setTrigConfig(INT_TIME);
    }
    if (m_noInject) {
      setNoInject();
    }

    g_tx->setTrigFreq(m_trigFreq);
    g_tx->setTrigCnt(getTrigCnt());
    g_tx->setTrigWord(&m_trigWord[0], m_trigWordLength);
    g_tx->setTrigWordLength(m_trigWordLength);
    g_tx->setTrigTime(m_trigTime);

    g_tx->setCmdEnable(keeper->getTxMask());
    while(!g_tx->isCmdEmpty());
    //std::this_thread::sleep_for(std::chrono::milliseconds(1));
}

//Replance all _Pretty_Functions
void Rd53a2TriggerLoop::execPart1() {
    SPDLOG_LOGGER_TRACE(logger, "");
    dynamic_cast<Rd53a*>(g_fe)->ecr();
    dynamic_cast<Rd53a*>(g_fe)->idle();
    dynamic_cast<Rd53a*>(g_fe)->idle();
    dynamic_cast<Rd53a*>(g_fe)->idle();
    dynamic_cast<Rd53a*>(g_fe)->idle();
    std::this_thread::sleep_for(std::chrono::microseconds(200));
    g_rx->flushBuffer();
    while(!g_tx->isCmdEmpty());
    g_tx->setTrigEnable(0x1);

}
//Replance all _Pretty_Functions
void Rd53a2TriggerLoop::execPart2() {
  SPDLOG_LOGGER_TRACE(logger, "");
    // Should be finished, lets wait anyway
    while(!g_tx->isTrigDone());
    // Disable Trigger
    g_tx->setTrigEnable(0x0);
    m_done = true;
}

//Replance all _Pretty_Functions
void Rd53a2TriggerLoop::end() {
  SPDLOG_LOGGER_TRACE(logger, "");
    //Nothing to do
}

void Rd53a2TriggerLoop::writeConfig(json &config) {
    config["count"] = getTrigCnt();
    config["frequency"] = m_trigFreq;
    config["time"] = m_trigTime;
    config["delay"] = m_trigDelay;
    config["delay2"] = m_trigDelay2;
    config["noInject"] = m_noInject;
    config["extTrigger"] = m_extTrig;
    config["doubleDelay"] = m_doubleDelay;
    config["Ntrig"] = m_Ntrig1;
    config["Ntrig2"] = m_Ntrig2;
}

void Rd53a2TriggerLoop::loadConfig(const json &config) {
    if (config.contains("count"))
        setTrigCnt(config["count"]);
    if (config.contains("frequency"))
        m_trigFreq = config["frequency"];
    if (config.contains("time"))
        m_trigTime = config["time"];
    if (config.contains("delay"))
        m_trigDelay = int(config["delay"]); //Actual trigger delay between first injection and first trigger pulse train.
    if (config.contains("delay2")){
        m_trigDelay2 = int(config["delay2"]); //Actual trigger delay between second injection and second trigger pulse train.
    }else{
        m_trigDelay2 = m_trigDelay; //If not specified, use delay
    }
    if (config.contains("noInject"))
        m_noInject = config["noInject"];
    if (config.contains("noInject2"))
        m_noInject2 = config["noInject2"];
    if (config.contains("extTrig"))
        m_extTrig = config["extTrig"];
    if (config.contains("doubleDelay"))
        m_doubleDelay = config["doubleDelay"];
    if (config.contains("Ntrig"))
        m_Ntrig1 = config["Ntrig"];
    if (config.contains("Ntrig2")){
        m_Ntrig2 = config["Ntrig2"];
    }else{
        m_Ntrig2 = m_Ntrig1; //If not specified, use Ntrig1
    }
}
