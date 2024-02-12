#include "AnalysisConsole.h"

int main(int argc, char *argv[])
{
    AnalysisOpts options;

    int res = AnalysisConsole::parseOptions(argc, argv, options);

    if(res<=0) return res;

    AnalysisConsole ac(options);

    // Setup loggers
    ac.init();

    // ScanConsole con;
    // con.init(argc, argv);
    // if(res<=0) return res;

    res=ac.loadConfig();

    if(res!=0) {
      exit(res);
    }

    ac.setupAnalysis();

    ac.loadHistograms();

    ac.run();

    ac.saveAndPlot();

    return 0;
}
