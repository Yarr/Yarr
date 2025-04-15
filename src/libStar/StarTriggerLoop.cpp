/*
 */

#include "include/StarTriggerLoop.h"

#include <iostream>
#include <iomanip>

#include "Bookkeeper.h"

#include "logging.h"

namespace {
    auto logger = logging::make_log("StarTriggerLoop");
}

StarTriggerLoop::StarTriggerLoop()
  : LoopActionBase(LOOP_STYLE_TRIGGER),
	m_trigDelay(45), // L0_delay 34
	m_trigFreq(1e3), // 1kHz
	m_trigTime(10), // 10s
	m_noInject(false),
	m_digital(false),
	m_trigWord{}
{
	setTrigCnt(50); // Maximum number of triggers to send
	min = 0;
	max = 0;
	step = 1;
	loopType = typeid(this);
}

void StarTriggerLoop::init() {
	m_done = false;
	SPDLOG_LOGGER_DEBUG(logger, "init");
	// Setup Trigger
	if (not m_seqGen.empty()) {
		this->setTrigWordFromFile();
	} else if (m_noInject) {
		this->setNoInject();
	} else {
		this->setTrigWord();
	}

	if (m_cmdCnt > 0) {
		g_tx->setTrigConfig(INT_COUNT); //use internal charge injection
	} else {
		g_tx->setTrigConfig(INT_TIME);  //external trigger
	}

	g_tx->setTrigFreq(m_trigFreq);
	g_tx->setTrigCnt(m_cmdCnt);
	g_tx->setTrigWord(m_trigWord.data(), m_trigWord.size());
	g_tx->setTrigWordLength(m_trigWord.size());
	g_tx->setTrigTime(m_trigTime);

        g_tx->setCmdEnable(keeper->getTxMask());

	logger->trace("Built trigger words {}:", m_trigWord.size());
        if(logger->should_log(spdlog::level::trace)) {
          for(size_t i=0; i<m_trigWord.size(); i++) {
            logger->trace("{:08x}", m_trigWord[i]);
          }
        }

	while(!g_tx->isCmdEmpty());
}


void StarTriggerLoop::execPart1() {
	SPDLOG_LOGGER_DEBUG(logger, "");
	// Enable Trigger
	g_tx->setTrigEnable(0x1);
}

void StarTriggerLoop::execPart2() {
	SPDLOG_LOGGER_DEBUG(logger, "");
	while(!g_tx->isTrigDone());
	// Disable Trigger
	g_tx->setTrigEnable(0x0);
	m_done = true;
}


void StarTriggerLoop::end() {
	SPDLOG_LOGGER_DEBUG(logger, "");

	// Go back to general state of FE, do something here (if needed)
	while(!g_tx->isCmdEmpty());
}

void StarTriggerLoop::setTrigWord() {
// latency (unit::1 BC),   1 LCB::frame (16 bits) covers 4 BCs, 1 trigWord (32 bits) covers 8 BCs

	unsigned int full_words = m_trigDelay / 8;

	// Words of delay + trigger and pulse
	m_trigWord.resize(full_words+2);

	// Last 32-bit word goes first in buffer.
	// High 16 bits are sent before the low 16 bits.
	m_trigWord[0] = (LCB::l0a_mask(1, 0, false) << 16) + LCB::IDLE;

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
        if (m_digital) {
                cmd_word = LCB::fast_command(LCB::ABC_DIGITAL_PULSE, 3-(remainder%4));
        }

	if(remainder < 4) {
		// Send idle then cmd_word (then everything else)
		m_trigWord[full_words + 1] = (LCB::IDLE << 16) + cmd_word;
	} else {
		// Send cmd_word then idle (then everything else)
		m_trigWord[full_words + 1] = (cmd_word << 16) | LCB::IDLE;
	}

}

void StarTriggerLoop::setNoInject() {
	m_trigWord.resize(4);
	m_trigWord[0] = (LCB::IDLE << 16) + LCB::IDLE;
	m_trigWord[1] = (LCB::IDLE << 16) + LCB::IDLE;
	m_trigWord[2] = (LCB::IDLE << 16) + LCB::IDLE;
	m_trigWord[3] = (LCB::l0a_mask(4, 10, false) << 16) | LCB::IDLE;
}

void StarTriggerLoop::setTrigWordFromFile() {
	std::vector<uint8_t> commands = m_seqGen.getSequence();
	// The first byte is sent out first

	// The commands are expected to be of even size
	if (commands.size() % 2) {
		std::stringstream seq_ss;
		m_seqGen.dump(seq_ss);
		logger->error("The command sequence loaded from file are not of even number of bytes: {}", seq_ss.str());
		logger->error("No trigger will be sent!");
		m_trigWord.clear();
		return;
	} else {
		// Number of 32-bit words
		// Resize m_trigWord with IDLEs
		m_trigWord.resize((commands.size() + 3 ) / 4);
	}

	// Last 32-bit word goes first in buffer.
	// High 16 bits are sent before the low 16 bits.
	for (size_t i = 0; i < m_trigWord.size(); i++) {
		// Form 16-bit LCB frame from two bytes of commands at a time
		uint16_t frame1 = (commands[4*i] << 8) + commands[4*i+1];

		uint16_t frame2 = LCB::IDLE;
		if (4*i+3 < commands.size()) {
			frame2 = (commands[4*i+2] << 8) + commands[4*i+3];
		}

		size_t w = m_trigWord.size() - 1 - i; // index of m_trigWord
		m_trigWord[w] = (frame1 << 16) + frame2;
	}
}

void StarTriggerLoop::writeConfig(json &config) {
	config["trig_count"] = m_cmdCnt;
	config["trig_frequency"] = m_trigFreq;
	config["trig_time"] = m_trigTime;
	config["l0_latency"] = m_trigDelay;
	config["noInject"] = m_noInject;
	config["digital"] = m_digital;
	config["fpath_sequence"] = m_fpathSeq;
}

void StarTriggerLoop::loadConfig(const json &config) {

	if (config.contains("trig_count"))
		m_cmdCnt = config["trig_count"];

	if (config.contains("trig_frequency"))
		m_trigFreq = config["trig_frequency"];

	if (config.contains("trig_time"))
		m_trigTime = config["trig_time"];

	if (config.contains("l0_latency"))
		m_trigDelay = config["l0_latency"];

	if (config.contains("noInject"))
		m_noInject = config["noInject"];

	if (config.contains("digital"))
		m_digital = config["digital"];

	if (config.contains("fpath_sequence")) {
		m_fpathSeq = config["fpath_sequence"];
		if (not m_fpathSeq.empty()) {
			m_seqGen.load(m_fpathSeq);
		}
	}

	// Set the expected number of triggers for analysis:
	// number of times to send the trigger sequence * number of triggers in the sequence
	uint32_t ntrigs_per_seq = 1;
	if (not m_seqGen.empty()) {
		ntrigs_per_seq = m_seqGen.count_triggers();
	}
	this->setTrigCnt(m_cmdCnt * ntrigs_per_seq);

	logger->info("Configured trigger loop: trig_count: {} trig_frequency: {} l0_delay: {}",
                      getTrigCnt(), ntrigs_per_seq, m_trigFreq, m_trigDelay);
}



