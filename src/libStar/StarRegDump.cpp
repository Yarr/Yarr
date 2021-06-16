/*
 */

#include "include/StarRegDump.h"

#include <iostream>
#include <iomanip>

#include "logging.h"

namespace {
    auto logger = logging::make_log("StarRegDump");
}

StarRegDump::StarRegDump() : LoopActionBase(LOOP_STYLE_NOP) {

        m_addr = -1;
	loopType = typeid(this);
}

void StarRegDump::init() {
	m_done = false;
	SPDLOG_LOGGER_DEBUG(logger, "init");
	g_tx->setCmdEnable(keeper->getTxMask());

	while(!g_tx->isCmdEmpty());
}


void StarRegDump::execPart1() {
    SPDLOG_LOGGER_DEBUG(logger, "");
    logger->trace("Executing Register Dump");

    for ( FrontEnd* fe : keeper->feList ) {
        if (!fe->isActive()) {continue;}
	
        auto readReg = [&](auto words) {
            g_tx->writeFifo((words[0] << 16) + words[1]);
            g_tx->writeFifo((words[2] << 16) + words[3]);
            g_tx->writeFifo((words[4] << 16) + words[5]);
            g_tx->writeFifo((words[6] << 16) + words[7]);
            g_tx->writeFifo((words[8] << 16) + LCB::IDLE);

            // Could have this once for all regs though...
            g_tx->releaseFifo();
        };

        if (m_addr == -1) { //Default to looping over all regs

            logger->trace("Dumping all regs");
          
            for (int index = 0; index < ABCStarRegs::_size(); ++index) {
                logger->trace(ABCStarRegs::_names()[index]);
                ((StarChips*) fe)->sendCmd( ((StarChips*) fe)->read_abc_register(ABCStarRegs::_values()[index]));
            }

            for (int index = 0; index < HCCStarRegister::_size(); ++index) {
                logger->trace(HCCStarRegister::_names()[index]);
                ((StarChips*) fe)->sendCmd( ((StarChips*) fe)->read_hcc_register(HCCStarRegister::_values()[index]));
            }
        } else {
                logger->trace(m_addr);
                ((StarChips*) fe)->sendCmd( ((StarChips*) fe)->read_abc_register(m_addr));
        }
    }
}

void StarRegDump::execPart2() {
	SPDLOG_LOGGER_DEBUG(logger, "");
	m_done = true;

}


void StarRegDump::end() {
	SPDLOG_LOGGER_DEBUG(logger, "");

	// Go back to general state of FE, do something here (if needed)
	while(!g_tx->isCmdEmpty());
}

void StarRegDump::writeConfig(json &config) {
        config["addr"] = m_addr;
}

void StarRegDump::loadConfig(json &config) {

        if (!config["addr"].empty())
                m_addr = config["addr"];
}



