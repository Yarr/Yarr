#ifndef YARR_ANALYSIS_CONSOLE_H
#define YARR_ANALYSIS_CONSOLE_H

#include <string>
#include <vector>

struct AnalysisOpts {
    std::string defaultLogPattern = "[%T:%e]%^[%=8l][%=15n][%t]:%$ %v";
    std::string logCfgPath;
    std::string scanFile;
    std::string chipConfigPath;
    std::string chipType;
    std::string outputDir = "./data/reanalysis/";
    bool doPlots = false;
    int target_charge{-1};
    int target_tot{-1};
    int mask_opt{-1};
    bool doOutput = true;
    std::vector<std::string> histogramFiles;

    std::string commandLineStr;
    std::string progName;
};

class AnalysisConsoleImpl;

class AnalysisConsole {
    AnalysisConsoleImpl *pimpl;

  public:
    static int parseOptions(int argc, char *argv[], AnalysisOpts &anOpts);

    AnalysisConsole(const AnalysisOpts &opts);

    /// Light-weight initialisation (setup loggers)
    int init();

    int loadConfig();

    void setupAnalysis();
    void loadHistograms();
    void run();
    void saveAndPlot();
};

#endif
