/*
 */

#include <unistd.h>
#include <bitset>
#include "include/StarTriggerLoop.h"
#include <iostream>
#include <iomanip>

StarTriggerLoop::StarTriggerLoop() : LoopActionBase() {

	m_trigCnt = 50; // Maximum number of triggers to send
	m_trigDelay = 45; // L0_delay 34
	m_trigFreq = 1e3; // 1kHz
	m_trigTime = 10; // 10s
	m_noInject = false;
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
// latency (unit::1 BC),   1 LCB::frame (16 bits) covers 4 BCs, 1 trigWord (32 bits) covers 8 BCs

	// Last 32-bit word goes first in buffer.
	// High 16 bits are sent before the low 16 bits.
	m_trigWord[0] = (LCB::l0a_mask(1, 0, false) << 16) + LCB::IDLE;

	unsigned int full_words = m_trigDelay / 8;
	if(full_words > m_trigWord.size() - 2) {
		std::cerr << __PRETTY_FUNCTION__ << " : Trigger delay is either too large for pattern buffer!\n";
	}

	//TODO verify setting of trigger delay
	for (unsigned i = 0; i<full_words; i++) {
		m_trigWord[i+1] = (LCB::IDLE << 16) + LCB::IDLE;
	}

	unsigned int remainder = m_trigDelay - (full_words * 8);

	// Final word in buffer goes first
	auto cmd_word = LCB::fast_command(LCB::ABC_CAL_PULSE, 3-(remainder%4));
	//   Or LCB::ABC_DIGITAL_PULSE
	if(remainder < 4) {
		// Send idle then cmd_word (then everything else)
		m_trigWord[full_words + 1] = (LCB::IDLE << 16) + cmd_word;
	} else {
		// Send cmd_word then idle (then everything else)
		m_trigWord[full_words + 1] = (cmd_word << 16) | LCB::IDLE;
	}

	// Words of delay + trigger and pulse
	m_trigWordLength = full_words+2;


//	78557855785578554766713c715959595959595959595959474b00007855785578557855 --- read ABC reg32
//	m_trigWord[0] = 0x78557855;
//	m_trigWord[1] = 0x78557855;
//	m_trigWord[2] = 0x4766713c;
//	m_trigWord[3] = 0x71595959;
//	m_trigWord[4] = 0x59595959;
//	m_trigWord[5] = 0x59595959;
//	m_trigWord[6] = 0x474b0000;
//	m_trigWord[7] = 0x78557855;
//	m_trigWord[8] = 0x78557855;
//	m_trigWordLength = 9;


}


void StarTriggerLoop::setTrigDelay(unsigned int delay) {

}

void StarTriggerLoop::setNoInject() {
	m_trigWord[0] = (LCB::IDLE << 16) + LCB::IDLE;
	m_trigWord[1] = (LCB::IDLE << 16) + LCB::IDLE;
	m_trigWord[2] = (LCB::IDLE << 16) + LCB::IDLE;
	m_trigWord[3] = (LCB::l0a_mask(4, 10, false) << 16) | LCB::IDLE;
	m_trigWordLength = 4;
}


void StarTriggerLoop::writeConfig(json &config) {
	config["trig_count"] = m_trigCnt;
	config["trig_frequency"] = m_trigFreq;
	config["trig_time"] = m_trigTime;
	config["l0_latency"] = m_trigDelay;
	config["noInject"] = m_noInject;
}

void StarTriggerLoop::loadConfig(json &config) {

	if (!config["trig_count"].empty())
		m_trigCnt = config["trig_count"];

	if (!config["trig_frequency"].empty())
		m_trigFreq = config["trig_frequency"];

	if (!config["trig_time"].empty())
		m_trigTime = config["trig_time"];

	if (!config["l0_latency"].empty())
		m_trigDelay = config["l0_latency"];

	if (!config["noInject"].empty())
		m_noInject = config["noInject"];

	if (!config["verbose"].empty())
		verbose = config["verbose"];


	std::cout << "------trig_count: " <<  m_trigCnt
			  << "------trig_frequency: " << m_trigFreq
			  << "------l0_delay: " << m_trigDelay
			  <<std::endl;


}



