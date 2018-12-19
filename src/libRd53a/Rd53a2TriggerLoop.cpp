// #################################
// # Author: Timon Heim and Magne Lauritzen
// # Email: timon.heim at cern.ch, magne.eik.laurizen at cern.ch
// # Project: Yarr
// # Description: Two Trigger Loop for RD53A
// # Date: 12/2018
// ################################

#include "Rd53a2TriggerLoop.h"
#include <bitset>

Rd53a2TriggerLoop::Rd53a2TriggerLoop() : LoopActionBase() {
    m_trigCnt = 50;
    m_realTrigDelay = 48;
    m_trigDelay = m_realTrigDelay + 16 - CMDDEL;
    m_trigFreq = 1e3;
    m_trigTime = 10;
    m_trigWordLength = 32;
    m_doubleDelay = 72;
    m_Ntrig = 16;
    m_debugParamAdder = 0;
    
    m_trigWord.fill(0x69696969);
    m_trigPulses.fill(0x00);
    m_noInject = false;
    m_extTrig = false;

    min = 0;
    max = 0;
    step = 1;

    isInner = false;
    loopType = typeid(this);
    verbose = false;
}


void Rd53a2TriggerLoop::setNoInject() {
    m_trigWord.fill(0x69696969);
}

void Rd53a2TriggerLoop::singleCmdInject(){
    //Performs double injection with a single inject command. Works up to doubleDelay = 15.75 bunch crossings.
    m_doubleInjectOffset = 3+(m_doubleDelay + m_trigDelay + m_Ntrig - 1)/8; //Calculate trigWord offset, i.e. at which index the header cmd is to be inserted.
    m_trigWord[m_doubleInjectOffset] = 0x69696363; //Inject header
    m_edgeDuration = int(m_doubleDelay*4); //cal_edge duration (160MHz) is 4 times doubledelay (40MHz)
    m_trigWord[m_doubleInjectOffset-1] = Rd53aCmd::genCal(8, 1, 8-CMDDEL, m_edgeDuration, 1, 4*(8-CMDDEL)+1); // Inject
    setFlexibleTrigger(m_doubleInjectOffset,8); //Trigger on injection #1
    setFlexibleTrigger(m_doubleInjectOffset,8+std::round(float(m_edgeDuration)/4)); //Trigger on injection #2
}

void Rd53a2TriggerLoop::doubleCmdInject(){
    //Performs double injection with a two inject commands. Smallest delay depends on number of triggers and trigger delay.
    uint8_t minimumDelay = findSmallestDelay();
    std::cout << "Minimum allowable delay: " << signed(minimumDelay) << ", doubleDelay : " << m_doubleDelay << std::endl;
    if (minimumDelay>m_doubleDelay){
        throw std::runtime_error(std::string(__PRETTY_FUNCTION__) + " : doubleDelay ("+std::to_string(m_doubleDelay)+") cannot be less than " + std::to_string(minimumDelay));
    }
    
    //Inject
    if ((m_trigDelay >= 19) && (m_trigDelay <= 87)) {
        //**Second calibration command population**
        //Calculate empty frames between second injection and the two full trigger frames
        int8_t e = std::round(float(m_realTrigDelay + m_Ntrig/2)/8) - 1;
        if (e<0) e=0;
        for (uint k=0; k<2; k++){
            m_trigWord[2+k] = Rd53aCmd::genTrigger(0xF, 2*k+1, 0xF, 2*k+2); // Trigger for second calibration command
        }
        m_trigWord[5 + e] = 0x69696363; //Second calibration command header
        m_trigWord[4 + e] = Rd53aCmd::genCal(8, true, 0, 0, 1, 0); // Second calibration command
        
        //**First calibration command population**
        //Calculate parameters for the first injection
        m_edgeDelay = 7-(int(m_doubleDelay)-(9-CMDDEL))%8;
        std::cout << "m_edgeDelay : " << m_edgeDelay << std::endl;
        m_auxDelay = m_edgeDelay*4 + 2;
        m_doubleInjectOffset = 7 + (int(m_doubleDelay)-(9-CMDDEL))/8 + e; //Calculate trigWord offset, i.e. at which index the header cmd is to be inserted.
        if(m_doubleInjectOffset > m_trigWordLength-1){
            throw std::runtime_error(std::string(__PRETTY_FUNCTION__) + " : doubleDelay is too large. (Burst buffer words: "+std::to_string(m_doubleInjectOffset + 1)+").");
        }
        setFlexibleTrigger(m_doubleInjectOffset, m_edgeDelay + CMDDEL); //Flexible trigger for first injection of double injection scheme
        m_trigWord[m_doubleInjectOffset] = 0x69696363; //First calibration command header
        m_trigWord[m_doubleInjectOffset-1] = Rd53aCmd::genCal(8, 0, m_edgeDelay, 1, 1, m_auxDelay); // first calibration command
    } else {
        throw std::runtime_error(std::string(__PRETTY_FUNCTION__) + " : Delay must be a value between 8 and 76.");
    }

}

void Rd53a2TriggerLoop::setFlexibleTrigger(uint8_t offset, int injDelay){
    //Calculates and sets a train of trigger pulses that can begin and end at arbitrary locations.
    //injDelay : Delta T in bunch crossings between beginning of trigWord frame containing injection command and the pulse itself.
    uint16_t empty_trigs = m_realTrigDelay + injDelay-9; //
    uint8_t empty_frames = empty_trigs/8; //Number of empty trigword frames to pad
    uint8_t trig_start_i = empty_trigs%8; //Index of the first trigger pulse in first frame containing trigger pulses
    int8_t full_frames = (m_Ntrig-(8-trig_start_i)-1)/8; //Number of trigword frames filled with 8 trigger pulses
    uint8_t trig_end_i = (empty_trigs+m_Ntrig-1)%8; //Index of the last trigger pulse in the last trigword frame containing trigger pulses

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

uint8_t Rd53a2TriggerLoop::findSmallestDelay(){
    //The smallest allowable doubledelay in a two-command scheme depends on many variables and is not trivial to express with a model.
    //This function emulates setFlexibleTrigger to find the smallest doubledelay that will not cause overlap between the first trigger frames and the second injection header frame.s
    uint8_t empty_frames;
    int8_t full_frames;
    int8_t k = -10; //Start with a low value
    uint8_t n;
    if(m_Ntrig==0){return 8-CMDDEL;}
    int injDelay;
    for(n=0; n<8; n++){
        injDelay = n + CMDDEL;
        empty_frames = (m_realTrigDelay + injDelay-9)/8;
        full_frames = (m_Ntrig-(8-(m_realTrigDelay + injDelay-9)%8)-1)/8;
        if(empty_frames+full_frames>k){
            k = empty_frames+full_frames;
        }
    }

    return (k+4)*8 - injDelay;
}

void Rd53a2TriggerLoop::init(){
    m_done = false;
    if (verbose)
      std::cout << __PRETTY_FUNCTION__ << std::endl;
    m_trigWord.fill(0x69696969);
    m_trigPulses.fill(0x00);
    //m_doubleDelay += 1; //debug
    //m_realTrigDelay += 1; //debug
    //m_trigDelay += 1; //debug
    //m_Ntrig += 1;
    if(m_doubleDelay<16){
        this->singleCmdInject();
    }else{
        this->doubleCmdInject();
    }
    //rearm
    m_trigWord[1] = 0x69696363;
    m_trigWord[0] = Rd53aCmd::genCal(8, true, 0, 0, 0, 0); // Arm inject

    if (m_extTrig) {
      g_tx->setTrigConfig(EXT_TRIGGER);
    } else if (m_trigCnt > 0) {
      g_tx->setTrigConfig(INT_COUNT);
    } else {
      g_tx->setTrigConfig(INT_TIME);
    }
    if (m_noInject) {
      setNoInject();
    }

    g_tx->setTrigFreq(m_trigFreq);
    g_tx->setTrigCnt(m_trigCnt);
    g_tx->setTrigWord(&m_trigWord[0], m_trigWordLength);
    g_tx->setTrigWordLength(m_trigWordLength);
    g_tx->setTrigTime(m_trigTime);

    g_tx->setCmdEnable(keeper->getTxMask());
    while(!g_tx->isCmdEmpty());
    //std::this_thread::sleep_for(std::chrono::milliseconds(1));
}

void Rd53a2TriggerLoop::execPart1() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
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

void Rd53a2TriggerLoop::execPart2() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    // Should be finished, lets wait anyway
    while(!g_tx->isTrigDone());
    // Disable Trigger
    g_tx->setTrigEnable(0x0);
    m_done = true;
}

void Rd53a2TriggerLoop::end() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    //Nothing to do
}

void Rd53a2TriggerLoop::writeConfig(json &config) {
    config["count"] = m_trigCnt;
    config["frequency"] = m_trigFreq;
    config["time"] = m_trigTime;
    config["delay"] = m_realTrigDelay;
    config["noInject"] = m_noInject;
    config["extTrigger"] = m_extTrig;
    config["doubleDelay"] = m_doubleDelay;
    config["Ntrig"] = m_Ntrig;
}

void Rd53a2TriggerLoop::loadConfig(json &config) {
    if (!config["count"].empty())
        m_trigCnt = config["count"];
    if (!config["frequency"].empty())
        m_trigFreq = config["frequency"];
    if (!config["time"].empty())
        m_trigTime = config["time"];
    if (!config["delay"].empty()){
        m_realTrigDelay = int(config["delay"]); //Actual trigger delay between injection and first trigger pulse.
        m_trigDelay = m_realTrigDelay+16-CMDDEL; //A value used for many calculations.
    }
    if (!config["noInject"].empty())
        m_noInject = config["noInject"];
    if (!config["extTrig"].empty())
        m_extTrig = config["extTrig"];
    if (!config["doubleDelay"].empty())
        m_doubleDelay = config["doubleDelay"];
    if (!config["Ntrig"].empty())
        m_Ntrig = config["Ntrig"];

}
