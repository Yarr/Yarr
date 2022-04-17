//
// Created by wittgen on 3/29/22.
//

#ifndef YARR_SCANCONSOLE_H
#define YARR_SCANCONSOLE_H
#include <memory>
#include <string>
#include <vector>
#include <ScanOpts.h>
#include "storage.hpp"

class ScanConsoleImpl;
class ScanConsole {
public:
    ScanConsole();
    void init(const ScanOpts options);
    std::vector<std::string> getLog(unsigned n = 0);
    int loadConfig();
    int loadConfig(const json &config);
    int loadConfig(const char *config);
    unsigned getRunNumber();
    int setupScan();
    int configure();
    void plot();
    int initHardware();
    void cleanup();
    std::string getResults();
    void getResults(json &result);
    void run();
    void dump();
    ~ScanConsole();
private:
    std::unique_ptr<ScanConsoleImpl> pimpl;
};


#endif //YARR_SCANCONSOLE_H
