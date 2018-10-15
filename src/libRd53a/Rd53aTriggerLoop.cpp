// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Trigger Loop for RD53A
// # Date: 02/2018
// ################################

#include "Rd53aTriggerLoop.h"

Rd53aTriggerLoop::Rd53aTriggerLoop() : LoopActionBase() {
    m_trigCnt = 50;
    m_trigDelay = 48;
    m_trigFreq = 1e3;
    m_trigTime = 10;
    m_trigWordLength = 32;
    m_requiredTrigWordLength = 0;
    m_pulseDuration = 8;
    m_edgeDelay = 0;
    m_edgeDuration = 10;
    m_edgeMode = false;
    m_auxMode = 0;
    m_auxDelay = 0;
    m_doubleInject = false;
    m_doubleDelay = 0;
    m_BCReset = false;
    m_doubleInjectOffset = 0;

    
    m_trigWord.fill(0x69696969);
    //m_trigWord[15] = 0x69696363;
    //m_trigWord[14] = Rd53aCmd::genCal(8, 0, 0, 1, 0, 0); // Inject
    //m_trigWord[8] = Rd53aCmd::genTrigger(0xF, 1, 0xF, 2); // Trigger
    //m_trigWord[7] = Rd53aCmd::genTrigger(0xF, 3, 0xF, 4); // Trigger
    //m_trigWord[2] = 0x69696363; // Header
    //m_trigWord[1] = Rd53aCmd::genCal(8, 1, 0, 0, 0, 0); // Arm inject
    //m_trigWord[0] = 0x5c5c0000 + (Rd53aCmd::encode5to8(0x8<<1)<<8) + (Rd53aCmd::encode5to8(m_pulseDuration<<1)); // global pulse for sync FE
    m_noInject = false;
    m_extTrig = false;


    min = 0;
    max = 0;
    step = 1;

    isInner = false;
    loopType = typeid(this);
    verbose = false;
}

void Rd53aTriggerLoop::setTrigDelay(uint32_t delay, bool isSecond) {
  int offset;  
  if (isSecond){
      offset = 0;
    }else{
      offset = m_doubleInjectOffset;
    }
    int length = m_requiredTrigWordLength;
    if ((delay >= 16) && (delay <= 88)) {
        if(m_doubleInject == isSecond){length += 2;} //Add two frames for rearm commands
        m_trigDelay = delay;

        m_trigWord[length-2-(delay/8)+1+offset] = Rd53aCmd::genTrigger(0xF, 1, 0xF, 2); // Trigger
        m_trigWord[length-2-(delay/8)+offset] = Rd53aCmd::genTrigger(0xF, 3, 0xF, 4); // Trigger
    } else {
        std::cerr << __PRETTY_FUNCTION__ << " : Delay is either too small or too large!" << std::endl;
    }
    // Inject
    m_trigWord[length+offset] = 0x69696363; //Inject header
    if(isSecond){
      m_trigWord[length-1+offset] = Rd53aCmd::genCal(8, true, 0, 0, 1, 0); // Inject
    }else{
      m_trigWord[length-1+offset] = Rd53aCmd::genCal(8, m_edgeMode, m_edgeDelay, m_edgeDuration, m_auxMode, m_auxDelay); // Inject
    }
    // Rearm
    if(m_doubleInject == isSecond){
      m_trigWord[2+offset] = 0x69696363; // TODO might include ECR?
      m_trigWord[1+offset] = Rd53aCmd::genCal(8, true, 0, 0, 0, 0); // Arm inject
    }
    // Pulse
    m_trigWord[offset] = 0x69696969;
    //m_trigWord[0] = 0x5c5c0000 + (Rd53aCmd::encode5to8(0x8<<1)<<8) + (Rd53aCmd::encode5to8(m_pulseDuration<<1)); // global pulse for sync FE
}

void Rd53aTriggerLoop::setEdgeMode(uint32_t duration) {
    // Assumes CAL command to be in index 14
    m_trigWord[14] = Rd53aCmd::genCal(8, 1, 0, 40, 0, 0); // Inject
}

void Rd53aTriggerLoop::setNoInject() {
    m_trigWord[2] = 0x69696969; //rearm reset
    m_trigWord[1] = 0x69696969; //rearm reset
    if (m_doubleInjectOffset!=0){
      m_trigWord[15+m_doubleInjectOffset] = 0x69696969;
      m_trigWord[14+m_doubleInjectOffset] = 0x69696969;
      m_trigWord[2+m_doubleInjectOffset] = 0x69696969;
      m_trigWord[1+m_doubleInjectOffset] = 0x69696969;
    }else{
      m_trigWord[m_requiredTrigWordLength+2] = 0x69696969; //single injection header reset
      m_trigWord[m_requiredTrigWordLength-1] = 0x69696969;//single injection cal reset
    }
    //m_trigWord[0] = 0x5c5c0000 + (Rd53aCmd::encode5to8(0x8<<1)<<8) + (Rd53aCmd::encode5to8(m_pulseDuration<<1)); // global pulse for sync FE

}

void Rd53aTriggerLoop::setDoubleInject(){
  //minimumDelay is sum of trigger delay, triggers, pulse, cal cmd, edge delay, and difference in execution time between the first edge and second edge, in bunch crossings.
  float minimumDelay = m_trigDelay+24+1-5.5-7;
  if (minimumDelay>m_doubleDelay){
    throw std::runtime_error("doubleDelay ("+std::to_string(m_doubleDelay)+") cannot be less than delay+12.5, which is " + std::to_string(minimumDelay));
  }
  int remainder = m_doubleDelay - minimumDelay;
  m_edgeDelay = 7 - remainder % 8;
  m_doubleInjectOffset = remainder/8 + 5 + m_trigDelay/8;
  if(m_requiredTrigWordLength + m_doubleInjectOffset > 31){
    throw std::runtime_error("doubleDelay is too large. (Burst buffer words: "+std::to_string(m_requiredTrigWordLength + m_doubleInjectOffset + 1)+").");
  }
  //First injection
  this->setTrigDelay(m_trigDelay, false);
  //Second injection
  this->setTrigDelay(m_trigDelay, true);
}
void Rd53aTriggerLoop::init(){
    m_done = false;
    if (verbose)
      std::cout << __PRETTY_FUNCTION__ << std::endl;
    
    m_trigWord.fill(0x69696969);
    //If doubleInject is true, we set edgeMode = false and auxMode = 1.
    //Maximise m_auxdelay to ensure aux edge occurs after cal edge.
    //Also, we create a certain amount of empty frames ending with an injection.
    //If doubleinject is false, a regular injection is created
    if(m_doubleInject){
      m_edgeMode = false;
      m_auxMode = 1;
      m_auxDelay = 31;
      this->setDoubleInject();
    }else{
      this->setTrigDelay(m_trigDelay,false);
    }

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

void Rd53aTriggerLoop::execPart1() {
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

void Rd53aTriggerLoop::execPart2() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    // Should be finished, lets wait anyway
    while(!g_tx->isTrigDone());
    // Disable Trigger
    g_tx->setTrigEnable(0x0);
    m_done = true;
}

void Rd53aTriggerLoop::end() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    //Nothing to do
}

void Rd53aTriggerLoop::writeConfig(json &config) {
    config["count"] = m_trigCnt;
    config["frequency"] = m_trigFreq;
    config["time"] = m_trigTime;
    config["delay"] = m_trigDelay;
    config["noInject"] = m_noInject;
    config["extTrigger"] = m_extTrig;
}

void Rd53aTriggerLoop::loadConfig(json &config) {
    if (!config["count"].empty())
        m_trigCnt = config["count"];
    if (!config["frequency"].empty())
        m_trigFreq = config["frequency"];
    if (!config["time"].empty())
        m_trigTime = config["time"];
    if (!config["delay"].empty())
        m_trigDelay = config["delay"];
    if (!config["noInject"].empty())
        m_noInject = config["noInject"];
    if (!config["edgeMode"].empty())
        m_edgeMode = config["edgeMode"];
    if (!config["edgeDelay"].empty())
        m_edgeDelay = config["edgeDelay"];
    if (!config["edgeDuration"].empty())
        m_edgeDuration = config["edgeDuration"];
    if (!config["auxMode"].empty())
        m_auxMode = config["auxMode"];
    if (!config["auxDelay"].empty())
        m_auxDelay = config["auxDelay"];
    if (!config["extTrig"].empty())
        m_extTrig = config["extTrig"];
    if (!config["doubleInject"].empty())
        m_doubleInject = config["doubleInject"];
    if (!config["doubleDelay"].empty())
        m_doubleDelay = config["doubleDelay"];
    if (!config["BCReset"].empty())
        m_BCReset = config["BCReset"];
    m_requiredTrigWordLength = 3 + m_trigDelay/8;
    this->setTrigDelay(m_trigDelay, false);

}
