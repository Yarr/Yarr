//
// Created by wittgen on 3/29/22.
//
#include "ScanConsole.h"
#include "ScanConsoleImpl.h"

ScanConsole::ScanConsole() : pimpl(std::make_unique<ScanConsoleImpl>()) {}

ScanConsole::~ScanConsole() = default;


void ScanConsole::setupLogger(const char* config) {
    ScanConsoleImpl::setupLogger(config);
}

std::string ScanConsole::parseConfig(const std::vector<std::string> &args) {
     return ScanConsoleImpl::parseConfig(args);
}

int ScanConsole::init(const ScanOpts &options) {
    return pimpl->init(options);
}

int ScanConsole::init(int argc, char *argv[]) {
    return pimpl->init(argc, argv);
}

int ScanConsole::init(const std::vector<std::string> &args) {
    return pimpl->init(args);
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

std::vector<std::string> ScanConsole::getLog(std::size_t lim) {
    return ScanConsoleImpl::getLog(lim);
}
