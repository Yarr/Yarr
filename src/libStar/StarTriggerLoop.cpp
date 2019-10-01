/*
 */

//#include "NetioTxCore.h"
#include "BitStream.h"
#include <unistd.h>
#include <bitset>
#include "include/StarTriggerLoop.h"

StarTriggerLoop::StarTriggerLoop() : LoopActionBase() {

	m_trigCnt = 50; // Maximum number of triggers to send
	m_trigDelay = 45; // L0_delay 34
	m_trigFreq = 1e3; // 1kHz
	m_trigTime = 10; // 10s
	//    m_trigWord[0] = 0x00;
	//    m_trigWord[1] = TRIG_CMD;
	//    m_trigWord[2] = 0x00;
	//    m_trigWord[3] = CAL_CMD;
	m_noInject = false;
	m_extTrigger = false;
	isInner = false;
	min = 0;
	max = 0;
	step = 1;
	loopType = typeid(this);
}

void StarTriggerLoop::init() {
	m_done = false;
	if (verbose) std::cout << __PRETTY_FUNCTION__ << std::endl;
	// Setup Trigger
	this->setTrigWord();
	this->setTrigDelay(m_trigDelay);
	if (m_trigCnt > 0) {
		g_tx->setTrigConfig(INT_COUNT); //use internal charge injection
	} else {
		g_tx->setTrigConfig(INT_TIME);  //external trigger
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
}


void StarTriggerLoop::execPart1() {
	if (verbose)
		std::cout << __PRETTY_FUNCTION__ << std::endl;
	// Enable Trigger
	g_tx->setTrigEnable(0x1);
}

void StarTriggerLoop::execPart2() {
	if (verbose)
		std::cout << __PRETTY_FUNCTION__ << std::endl;
	while(!g_tx->isTrigDone());
	// Disable Trigger
	g_tx->setTrigEnable(0x0);
	m_done = true;
}


void StarTriggerLoop::end() {
	if (verbose)
		std::cout << __PRETTY_FUNCTION__ << std::endl;

	// Go back to general state of FE, do something here (if needed)
	while(!g_tx->isCmdEmpty());
}

void StarTriggerLoop::setTrigWord() {
	m_trigWord[0] = (LCB::IDLE << 16) +LCB::fast_command(LCB::ABC_CAL_PULSE, 1);
	m_trigWord[1] = (LCB::IDLE << 16) + LCB::IDLE;
	m_trigWord[2] = (LCB::IDLE << 16) + LCB::IDLE;
	m_trigWord[3] = LCB::l0a_mask(4, 10, false);
	m_trigWordLength = 4;

}


void StarTriggerLoop::setTrigDelay(unsigned int delay) {


//	unsigned pos = (delay-1)%32; // subtract 8 bit long trig cmd
//	    unsigned word = (delay-1)/32; // Select word in array
//
//	    if ((word < 3 && pos <= 27) || word < 2) {
//	        m_trigWord[2-word] = (TRIG_CMD>>pos);
//	        if (pos > 27) // In case we shifted over word border
//	            m_trigWord[2-1-word] = (TRIG_CMD<<(5-(32-pos)));
//	        m_trigDelay = delay;
//	    }
//	m_trigWordLength = 32 + delay;
}

void StarTriggerLoop::setNoInject() {
	m_trigWord[0] = 0;
	m_trigWord[1] = 0;
	m_trigWord[2] = 0;
	m_trigWord[3] = 0;
	m_trigWordLength = 4;
}




void StarTriggerLoop::writeConfig(json &config) {
	config["trig_count"] = m_trigCnt;
	config["trig_frequency"] = m_trigFreq;
	config["trig_time"] = m_trigTime;
	config["l0_delay"] = m_trigDelay;
	config["noInject"] = m_noInject;
	config["extTrigger"] = m_extTrigger;
}

void StarTriggerLoop::loadConfig(json &config) {

	if (!config["trig_count"].empty())
		m_trigCnt = config["trig_count"];

	if (!config["trig_frequency"].empty())
		m_trigFreq = config["trig_frequency"];

	if (!config["trig_time"].empty())
		m_trigTime = config["trig_time"];

	if (!config["l0_delay"].empty())
		m_trigDelay = config["l0_delay"];

	if (!config["noInject"].empty())
		m_noInject = config["noInject"];

	if (!config["extTrigger"].empty())
		m_extTrigger = config["extTrigger"];

	if (!config["verbose"].empty())
		verbose = config["verbose"];


	std::cout << "------trig_count: " <<  m_trigCnt
			  << "------trig_frequency: " << m_trigFreq
			  << "------l0_delay: " << m_trigDelay
			  <<std::endl;


}



