// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Trigger Loop for RD53A
// # Date: 02/2018
// ################################

#include "Rd53aTriggerLoop.h"
#include "Rd53aCmd.h"

#include <chrono>
#include <array>
//#include <thread>

class Rd53aTriggerLoop::Impl {
public:
    Impl();
    ~Impl();
    
    uint32_t trigCnt;
    uint32_t trigDelay;
    double   trigTime;
    double   trigFreq;
    std::array<uint32_t, 16> trigWord;
    uint32_t trigWordLength;
    bool     noInject;
    bool     edgeMode;
    uint32_t edgeDuration;
    uint32_t pulseDuration;
    bool     isInner;
};


Rd53aTriggerLoop::Impl::Impl()
    : trigCnt        ( 50 )
    , trigDelay      ( 48 )
    , trigTime       ( 10.0 )
    , trigFreq       ( 1e3 )
    , trigWordLength ( 16 )
    , noInject       ( false )    
    , edgeMode       ( false )
    , edgeDuration   ( 10 )
    , pulseDuration  ( 9 )
    , isInner          ( false )
{
    trigWord.fill(0x69696969);
    trigWord[15] = 0x69696363;
    trigWord[14] = Rd53aCmd::genCal(8, 0, 0, 1, 0, 0); // Inject
    trigWord[8]  = Rd53aCmd::genTrigger(0xF, 1, 0xF, 2); // Trigger
    trigWord[7]  = Rd53aCmd::genTrigger(0xF, 3, 0xF, 4); // Trigger
    trigWord[2]  = 0x5a5a6363; // ECR + header
    trigWord[1]  = Rd53aCmd::genCal(8, 1, 0, 0, 0, 0); // Arm inject
    trigWord[0]  = 0x5c5c0000 + (Rd53aCmd::encode5to8(0x8<<1)<<8) + (Rd53aCmd::encode5to8(pulseDuration<<1)); // global pulse for sync FE
}


Rd53aTriggerLoop::Impl::~Impl() {}


Rd53aTriggerLoop::Rd53aTriggerLoop()
    : LoopActionBase()
    , m_impl( new Rd53aTriggerLoop::Impl() )
{


    min = 0;
    max = 0;
    step = 1;

    loopType = typeid(this);
    verbose = false;
}


uint32_t Rd53aTriggerLoop::getTrigCnt() const { return m_impl->trigCnt; }

void Rd53aTriggerLoop::setTrigCnt(uint32_t cnt) { m_impl->trigCnt = cnt;}

void Rd53aTriggerLoop::setTrigTime(double time) { m_impl->trigTime = time;}

void Rd53aTriggerLoop::setTrigFreq(double freq) { m_impl->trigFreq = freq;}


void Rd53aTriggerLoop::setTrigDelay(uint32_t delay) {
    m_impl->trigWord.fill(0x69696969);
    // Inject
    m_impl->trigWord[15] = 0x69696363;
    m_impl->trigWord[14] = Rd53aCmd::genCal(8, 0, 0, 1, 0, 0); // Inject
    // Rearm
    m_impl->trigWord[2] = 0x69696363; // TODO might include ECR?
    m_impl->trigWord[1] = Rd53aCmd::genCal(8, 1, 0, 0, 0, 0); // Arm inject
    // Pulse
    m_impl->trigWord[0] = 0x5c5c0000 + (Rd53aCmd::encode5to8(0x8<<1)<<8) + (Rd53aCmd::encode5to8(m_impl->pulseDuration<<1)); // global pulse for sync FE
    if ((delay >= 16) && (delay <= 88)) {
        m_impl->trigDelay = delay;
        m_impl->trigWord[(13-(delay/8)+1)] = Rd53aCmd::genTrigger(0xF, 1, 0xF, 2); // Trigger
        m_impl->trigWord[(13-(delay/8))] = Rd53aCmd::genTrigger(0xF, 3, 0xF, 4); // Trigger
    } else {
        std::cerr << __PRETTY_FUNCTION__ << " : Delay is either too small or too large!" << std::endl;
    }
}

void Rd53aTriggerLoop::setEdgeMode(uint32_t duration) {
    // Assumes CAL command to be in index 14
    m_impl->trigWord[14] = Rd53aCmd::genCal(8, 1, 0, 10, 0, 0); // Inject
}

void Rd53aTriggerLoop::setNoInject() {
    m_impl->trigWord[15] = 0x69696969;
    m_impl->trigWord[14] = 0x69696969;
    m_impl->trigWord[2] = 0x69696969;
    m_impl->trigWord[1] = 0x69696969;
    m_impl->trigWord[0] = 0x5c5c0000 + (Rd53aCmd::encode5to8(0x8<<1)<<8) + (Rd53aCmd::encode5to8(m_impl->pulseDuration<<1)); // global pulse for sync FE

}

void Rd53aTriggerLoop::init() {
    m_done = false;
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;

    this->setTrigDelay(m_impl->trigDelay);
    if (m_impl->edgeMode)
        this->setEdgeMode(m_impl->edgeDuration);
    if (m_impl->trigCnt > 0) {
        g_tx->setTrigConfig(INT_COUNT);
    } else {
        g_tx->setTrigConfig(INT_TIME);
    }
    if (m_impl->noInject) {
        setNoInject();
    }
    g_tx->setTrigFreq       ( m_impl->trigFreq);
    g_tx->setTrigCnt        ( m_impl->trigCnt);
    g_tx->setTrigWord       (&m_impl->trigWord[0], 16);
    g_tx->setTrigWordLength ( m_impl->trigWordLength);
    g_tx->setTrigTime       ( m_impl->trigTime);

    g_tx->setCmdEnable(keeper->getTxMask());
    while(!g_tx->isCmdEmpty());
}

void Rd53aTriggerLoop::execPart1() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    //std::this_thread::sleep_for(std::chrono::milliseconds(200));
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
    config["count"]     = m_impl->trigCnt;
    config["frequency"] = m_impl->trigFreq;
    config["time"]      = m_impl->trigTime;
    config["delay"]     = m_impl->trigDelay;
    config["noInject"]  = m_impl->noInject;
}

void Rd53aTriggerLoop::loadConfig(json &config) {
    if (!config["count"].empty())
        m_impl->trigCnt = config["count"];
    if (!config["frequency"].empty())
        m_impl->trigFreq = config["frequency"];
    if (!config["time"].empty())
        m_impl->trigTime = config["time"];
    if (!config["delay"].empty())
        m_impl->trigDelay = config["delay"];
    if (!config["noInject"].empty())
        m_impl->noInject = config["noInject"];
    if (!config["edgeMode"].empty())
        m_impl->edgeMode = config["edgeMode"];
    
    this->setTrigDelay(m_impl->trigDelay);
}
