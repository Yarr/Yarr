/*
 */

#include "include/StarRegDump.h"

#include <iostream>
#include <iomanip>

#include "logging.h"

#include "AbcNames.h"
#include "Bookkeeper.h"
#include "HccNames.h"
#include "TxCore.h"

namespace {
    auto logger = logging::make_log("StarRegDump");
}

StarRegDump::StarRegDump() : LoopActionBase(LOOP_STYLE_NOP) {

    m_addr = -1;
    m_ABCaddrList = {};
    m_HCCaddrList = {};
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

        // Dealing with one register
        if (m_addr!=-1) {
                logger->trace(m_addr);
                ((StarChips*) fe)->sendCmd( ((StarChips*) fe)->read_abc_register(m_addr));
        }

        // Dealing with multiple registers
        if (!m_ABCaddrList.empty()) { //Dump ABC registers, if any specified
            logger->trace("Dumping specified ABC regs");

            for (size_t index = 0; index < m_ABCaddrList.size(); ++index) {
               logger->trace(m_ABCaddrList[index]);
               ((StarChips*) fe)->sendCmd( ((StarChips*) fe)->read_abc_register(m_ABCaddrList[index]));
           }
        }
        if (!m_HCCaddrList.empty()) { //Dump HCC registers, if any specified
            logger->trace("Dumping specified HCC regs");

            for (size_t index = 0; index < m_HCCaddrList.size(); ++index) {
               logger->trace(m_HCCaddrList[index]);
               ((StarChips*) fe)->sendCmd( ((StarChips*) fe)->read_hcc_register(m_HCCaddrList[index]));
           }
        }

        // Dump everything if nothing is specified (default)
        if (m_addr == -1 && m_ABCaddrList.empty() && m_HCCaddrList.empty()) { //Default to looping over all regs if nothing is specified

            logger->trace("Nothing specified: dumping all regs");

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
        config["ABCaddrList"] = m_ABCaddrList;
        config["HCCaddrList"] = m_HCCaddrList;
}

void StarRegDump::loadConfig(const json &config) {

        if (config.contains("addr"))
                m_addr = config["addr"];

        if (config.contains("ABCaddrList")){
            for (unsigned i=0; i<config["ABCaddrList"].size(); i++) {
                m_ABCaddrList.push_back(config["ABCaddrList"][i]);
            }
        }

        if (config.contains("HCCaddrList")){
            for (unsigned i=0; i<config["HCCaddrList"].size(); i++) {
                m_HCCaddrList.push_back(config["HCCaddrList"][i]);
            }
        }
}



