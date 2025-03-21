/*
 */

#include "include/StarRegDump.h"

#include <iostream>
#include <iomanip>

#include "logging.h"

#include "AbcNames.h"
#include "HccNames.h"

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

    for (unsigned id=0; id<keeper->getNumOfEntries(); id++) {
        auto fe = keeper->getFe(id);
        if (!fe->isActive()) {continue;}

        if (m_addr == -1) { //Default to looping over all regs

            logger->trace("Dumping all regs");

            auto &abcList = AbcNames::listRegs();
            for (size_t index = 0; index < abcList.size(); ++index) {
                auto &reg = abcList[index];
                logger->trace(AbcNames::regToString(reg));
                ((StarChips*) fe)->sendCmd( ((StarChips*) fe)->read_abc_register((int)reg));
            }

            auto &hccList = HccNames::listRegs();
            for (size_t index = 0; index < hccList.size(); ++index) {
                auto &reg = hccList[index];
                logger->trace(HccNames::regToString(reg));
                ((StarChips*) fe)->sendCmd( ((StarChips*) fe)->read_hcc_register((int)reg));
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

void StarRegDump::loadConfig(const json &config) {

        if (config.contains("addr"))
                m_addr = config["addr"];
}



