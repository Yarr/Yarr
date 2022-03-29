//
// Created by wittgen on 3/29/22.
//
#include "ScanConsole.h"
#include "ScanConsoleImpl.h"

ScanConsole::ScanConsole() : pimpl(std::make_unique<ScanConsoleImpl>()) {}

ScanConsole::~ScanConsole() = default;
void ScanConsole::init(const ScanOpts options) {
    pimpl->init(options);
}
std::vector<std::string> ScanConsole::getLog(unsigned n) {
    return pimpl->getLog(n);
}
int ScanConsole::loadConfig() {
    return pimpl->loadConfig();
}

int ScanConsole::loadConfig(const json &config) {
    return pimpl->loadConfig(config);
}
int ScanConsole::loadConfig(const char *config) {
    return pimpl->loadConfig(config);
}

unsigned ScanConsole::getRunNumber() {
    return pimpl->getRunNumber();
}

int ScanConsole::setupScan() {
    return pimpl->setupScan();
}

int ScanConsole::configure() {
    return pimpl->configure();
}

void ScanConsole::plot() {
    pimpl->plot();
}

int ScanConsole::initHardware() {
    return pimpl->initHardware();
}

void ScanConsole::cleanup() {
    pimpl->cleanup();
}

std::string ScanConsole::getResults() {
    return pimpl->getResults();
}

void ScanConsole::getResults(json &result) {
   pimpl->getResults(result) ;
}

void ScanConsole::run() {
    pimpl->run();
}

void ScanConsole::dump() {
    pimpl->dump();
}
