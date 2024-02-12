#include "AnalysisConsole.h"

#include "AllAnalyses.h"
#include "AllChips.h"
#include "AllStdActions.h"
#include "ClipBoard.h"
#include "ScanHelper.h"

#include "logging.h"
#include "LoggingConfig.h"

#include <getopt.h>

// Compare for example
// bin/scanConsole -r configs/controller/emuCfg_star.json -c configs/connectivity/example_star_setup.json -s configs/scans/star/std_nmask.json

// Drop hardware:
// bin/analysis_test -c configs/connectivity/example_star_setup.json -s configs/scans/star/std_nmask.json
// Change connectivity (multichips) to one config:
// bin/analysis_test -c configs/defaults/default_star.json -s configs/scans/star/std_nmask.json

namespace {
    auto logger = logging::make_log("AnalysisTest");
}

using clk = std::chrono::steady_clock;

void printHelp();

/// Trivial implementation providing enough scan info for analysis
class BasicScanInfo : public ScanLoopInfo {
        std::vector<std::unique_ptr<LoopActionBaseInfo>> loops;

    public:
        unsigned size() const { return loops.size(); }
        LoopActionBaseInfo *getLoop(unsigned n) const {
            return loops[n].get();
        }

        void loadScanConfg(const json &scanCfg);

        void addLoop(std::unique_ptr<LoopActionBaseInfo> action) {
            loops.push_back(std::move(action));
        }

        LoopStatus loopTemplate() {
           std::vector<LoopStyle> styleVec;
           std::vector<unsigned> statVec;
           for (unsigned int i=0; i<loops.size(); i++) {
               styleVec.push_back((LoopStyle)loops[i]->getStyle());
               // Template, so use 0
               statVec.push_back(0);
           }

           return {std::move(statVec), styleVec};
        }
};

class AnalysisConsoleImpl {
    AnalysisOpts options;

    std::unique_ptr<FrontEnd> frontEnd;

    ClipBoard<HistogramBase> clipHistoInput;
    std::vector<std::unique_ptr<ClipBoard<HistogramBase>> > clipResultOutput;

    std::vector<std::unique_ptr<AnalysisDataProcessor>> analysisProcessors;

    LoopStatus loopTemplate;

    clk::time_point start_time;
    clk::time_point config_load;
    clk::time_point histogram_load;
    std::vector<clk::time_point> tier_done;
    clk::time_point all_done;

  public:
    AnalysisConsoleImpl(const AnalysisOpts &opts);

    int init();

    int loadConfig();

    void loadHistograms();

    const LoopStatus &loopStatus() const { return loopTemplate; }

    void buildAnalyses(std::vector<std::unique_ptr<AnalysisDataProcessor>> &analyses,
                       ClipBoard<HistogramBase> &input,
                       std::vector<std::unique_ptr<ClipBoard<HistogramBase>>> &output_vector,
                       const json& scanCfg,
                       const ScanLoopInfo &s,
                       int mask_opt,
                       int target_tot, int target_charge);

    int loadConfigFile(const AnalysisOpts &anOpts, bool writeConfig, json &config);

    int setupAnalysis();
    void pushHisto(std::unique_ptr<HistogramBase> h);
    void run();
    void saveAndPlot();

    void reportTimings() const;
};

AnalysisConsole::AnalysisConsole(const AnalysisOpts &opts)
  : pimpl(new AnalysisConsoleImpl(opts))
{}

void AnalysisConsole::loadHistograms() {
    pimpl->loadHistograms();
}

int AnalysisConsole::loadConfig()
{
    return pimpl->loadConfig();
}

void AnalysisConsole::setupAnalysis()
{
  (void)    pimpl->setupAnalysis();
}

void AnalysisConsole::run() {
    pimpl->run();
}

void AnalysisConsole::saveAndPlot() {
    pimpl->saveAndPlot();
}

int AnalysisConsole::init() {
    return pimpl->init();
}

int AnalysisConsole::parseOptions(int argc, char *argv[], AnalysisOpts &anOpts) {
    optind = 1; // this is a global libc variable to reset getopt
    for (int i=1;i<argc;i++) {
        anOpts.commandLineStr.append(std::string(argv[i]).append(" "));
    }
    anOpts.progName=argv[0];
    const struct option long_options[] =
      {
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}};

    int c;
    while (true) {
        int opt_index=0;
        c = getopt_long(argc, argv, "hs:y:m:c:t:po:W:d:u:i:l:QIz", long_options, &opt_index);
        int count = 0;
        if(c == -1) break;
        switch (c) {
        case 'h':
            printHelp();
            return 0;
        case 's':
            anOpts.scanFile = std::string(optarg);
            break;
        case 'l': // Logger config file
            anOpts.logCfgPath = std::string(optarg);
            break;
        case 'y':
            anOpts.chipType = std::string(optarg);
            break;
        case 'm':
            anOpts.mask_opt = atoi(optarg);
            break;
        case 'c':
            anOpts.chipConfigPath = std::string(optarg);
            break;
        case 'p':
            anOpts.doPlots = true;
            break;
        case 'o':
            anOpts.outputDir = std::string(optarg);
            if (anOpts.outputDir.back() != '/')
                anOpts.outputDir = anOpts.outputDir + "/";
            break;
        case 't':
            optind -= 1; //this is a bit hacky, but getopt doesn't support multiple
            //values for one option, so it can't be helped
            for (; optind < argc && *argv[optind] != '-'; optind += 1) {
                switch (count) {
                case 0:
                    anOpts.target_charge = atoi(argv[optind]);
                    break;
                case 1:
                    anOpts.target_tot = atoi(argv[optind]);
                    break;
                default:
                    spdlog::error("Can only receive max. 2 parameters with -t!!");
                    break;
                }
                count++;
            }
            break;
        case '?':
            if (optopt == 's') {
                spdlog::error("Option {} requires a parameter! (Proceeding with default)", (char) optopt);
            } else {
                spdlog::error("Unknown parameter: {}", (char) optopt);
            }
            break;
        default:
            spdlog::critical("Error while parsing command line parameters! {}", c);
            std::cout << "Rerun with --help for more information\n";
            return -1;
        }
    }

    if (optind >= argc) {
      logger->error("Expected list of histogram files after other options");
      return 1;
    }

    for(int i=optind; i<argc; i++) {
      logger->info("Will process histo file {}", argv[i]);
      anOpts.histogramFiles.push_back(argv[i]);
    }

    if(anOpts.chipConfigPath.empty()) {
        spdlog::critical("Error: no FE config file given, please specify config file name under -c option, even if file does not exist!");
        std::cout << "Rerun with --help for more information\n";
        return -1;
    }

    if(anOpts.chipType.empty()) {
        spdlog::critical("Error: no FE config file given, please specify config file name under -c option, even if file does not exist!");
        std::cout << "Rerun with --help for more information\n";
        return -1;
    }

    return 1;
}

AnalysisConsoleImpl::AnalysisConsoleImpl(const AnalysisOpts &opts)
  : options(opts)
{
}

void AnalysisConsoleImpl::loadHistograms() {
    if (options.histogramFiles.empty()) {
        logger->warn("No histograms provided nothing to analyse");
    }

    // Now processor is running, so we can push histograms
    for(auto &h: options.histogramFiles) {
        logger->info("Loading histogram from {}", h);
        try {
            auto histoJson = ScanHelper::openJsonFile(h);
            auto histo = HistogramBase::fromJson(histoJson, loopTemplate);
            pushHisto(std::move(histo));
        } catch (std::runtime_error &e) {
            logger->critical("#ERROR# opening histogram file ({}): {}",
                             h, e.what());
            abort();
        }
    }

    histogram_load = clk::now();
}

// Based on ScanHelper::buildChips
std::unique_ptr<FrontEnd> buildChip(const json &chip, const std::string &feType)
{
    logger->info("Chip type: {}", feType);

    auto fe = StdDict::getFrontEnd(feType);
    auto *feCfg = dynamic_cast<FrontEndCfg*>(fe.get());
    feCfg->loadConfig(chip);
    return fe;
}

// Load scan config for the loop topology only
// Based on ScanFactory::loadConfig
void BasicScanInfo::loadScanConfg(const json &scanCfg) {
    logger->info("Loading Scan:");

    std::string name = scanCfg["scan"]["name"];
    logger->info("  Name: {}", name);

    logger->info("  Number of Loops: {}", scanCfg["scan"]["loops"].size());

    for (unsigned int i=0; i<scanCfg["scan"]["loops"].size(); i++) {
        logger->info("  Loading Loop #{}", i);
        std::string loopAction = scanCfg["scan"]["loops"][i]["loopAction"];
        logger->info("   Type: {}", loopAction);

        auto action = StdDict::getLoopAction(loopAction);

        if (action == nullptr) {
            logger->error("Unknown Loop Action: {}  ... skipping!", loopAction);
            logger->warn("Known ScanLoop actions:");
            for(auto &la: StdDict::listLoopActions()) {
              logger->warn("   {}", la);
            }

            continue;
        }

        if (scanCfg["scan"]["loops"][i].contains("config")) {
            logger->info("   Loading loop config ... ");
            action->loadConfig(scanCfg["scan"]["loops"][i]["config"]);
        }

        addLoop(std::move(action));
    }
}

int AnalysisConsoleImpl::loadConfigFile(const AnalysisOpts &anOpts, bool writeConfig, json &config)
{
    // Compared to ScanHelper version, skip controller config completely

    // load single FE config
    json chipConfig;

    try {
        chipConfig = ScanHelper::openJsonFile(anOpts.chipConfigPath);
    } catch (std::runtime_error &e) {
        logger->critical("#ERROR# opening chip configs ({}): {}",
                         anOpts.chipConfigPath, e.what());
        return -1;
    }

    frontEnd = buildChip(chipConfig, anOpts.chipType);

    // Load scans
    json scan;
    try {
        if (!anOpts.scanFile.empty()) {
            scan = ScanHelper::openJsonFile(anOpts.scanFile);
        }
    } catch (std::runtime_error &e) {
        logger->critical("#ERROR# opening scan config: {}", e.what());
        return -1;
    }

    config["scanCfg"]=scan;
    config["chipConfig"]=chipConfig;

    return 0;
}

void createOutputDirectory(const std::string &scanType, std::string &outputDir) {
    // Generate output directory path
    std::size_t pathPos = scanType.find_last_of('/');
    std::size_t suffixPos = scanType.find_last_of('.');
    std::string strippedScan;

    if (pathPos != std::string::npos && suffixPos != std::string::npos) {
        strippedScan = scanType.substr(pathPos + 1, suffixPos - pathPos - 1);
    } else {
        strippedScan = scanType;
    }

    outputDir += (strippedScan + "/");
    logger->info("Creating output dir: {}", outputDir);
    std::string cmdStr = "mkdir -p "; //I am not proud of this ):
    cmdStr += outputDir;
    int sysExSt = system(cmdStr.c_str());
    if (sysExSt != 0) {
        logger->error("Error creating output directory - plots might not be saved!");
    }
}

int AnalysisConsoleImpl::loadConfig()
{
    json scanConsoleConfig;

    int result = loadConfigFile(options, true, scanConsoleConfig);
    if(result<0) {
        logger->error("Failed to read configs");
        return -1;
    }

    std::cout << scanConsoleConfig << std::endl;

    if(options.doOutput || options.doPlots) {
      logger->info("Creating output dir for plots: {}", options.outputDir);
      createOutputDirectory(options.scanFile, options.outputDir);
    }

    config_load = clk::now();

    return 0;
}

void AnalysisConsoleImpl::buildAnalyses(std::vector<std::unique_ptr<AnalysisDataProcessor>> &analyses,
                                        ClipBoard<HistogramBase> &input,
                                        std::vector<std::unique_ptr<ClipBoard<HistogramBase>>> &output_vector,
                                        const json& scanCfg,
                                        const ScanLoopInfo &scanInfo,
                                        int mask_opt,
                                        int target_tot, int target_charge)
{
    logger->info("Loading analyses ...");
    logger->error("Check error log works...");

    if(!scanCfg.contains("scan")) {
        logger->error("Scan config has no 'scan' property!");
        throw std::runtime_error("buildAnalyses failure");
    }

    if(!scanCfg["scan"].contains("analysis")) {
        logger->error("Scan config has no 'analysis' property in 'scan' property!");
        throw std::runtime_error("buildAnalyses failure");
    }

    const json &anaCfg = scanCfg["scan"]["analysis"];

    logger->error("Reading analysis configs...");

    // Parse scan config and build analysis hierarchy
    // Use a 2D vector to hold algorithm indices for all tiers of analysis processors
    ScanHelper::AlgoTieredIndex algoIndexTiers;
    try {
        ScanHelper::buildAnalysisHierarchy(algoIndexTiers, anaCfg);
    } catch (std::runtime_error &e) {
        logger->error("Building analysis hierarchy: {}", e.what());
        throw std::runtime_error("buildAnalyses failure");
    }

    // Tell analysis to skip HistogramArchiver
    std::string emptyDir = "";

    // ID should be ignored?
    int test_id = 0;
    ScanHelper::buildAnalysisForFrontEnd
        (analyses,
         test_id,
         dynamic_cast<FrontEndCfg*>(frontEnd.get()),
         anaCfg,
         frontEnd->geo,
         algoIndexTiers,
         nullptr, // No feedback
         output_vector,
         input,
         &scanInfo,
         mask_opt,
         emptyDir,
         target_tot,
         target_charge);
}

int AnalysisConsoleImpl::setupAnalysis()
{
    json scanCfg;

    BasicScanInfo scanInfo;

    try {
        scanCfg = ScanHelper::openJsonFile(options.scanFile);
    } catch (std::runtime_error &e) {
        logger->critical("#ERROR# opening chip configs ({}): {}",
                         options.scanFile, e.what());
        return -1;
    }

    scanInfo.loadScanConfg(scanCfg);
    loopTemplate = scanInfo.loopTemplate();

    try {
        buildAnalyses(analysisProcessors,
                      clipHistoInput,
                      clipResultOutput,
                      scanCfg, scanInfo,
                      options.mask_opt,
                      options.target_tot, options.target_charge);

    // Run from downstream to upstream
    logger->info("Starting analysis threads:");
    for (auto& ana : analysisProcessors) {
        ana->init();
        ana->run();
    }

    logger->info(" .. started threads");

    } catch (std::exception &e) {
        logger->error("Building analyses failed: {}", e.what());
        throw std::runtime_error("buildAnalyses failure");
    }
    return 0;
}

void AnalysisConsoleImpl::pushHisto(std::unique_ptr<HistogramBase> h) {
    clipHistoInput.pushData(std::move(h));
}

void AnalysisConsoleImpl::run() {
    ScanHelper::banner(logger,"Run Analysis");

    // Join from upstream to downstream.
    clipHistoInput.finish();

    for (unsigned i=0; i<analysisProcessors.size(); i++) {
      logger->info("Waiting for analysis level to finish ...");

      // First level is taking from input
      analysisProcessors[i]->join();
      tier_done.push_back(clk::now());

      clipResultOutput[i]->finish();
    }
}

void AnalysisConsoleImpl::saveAndPlot() {
    // Save (updated) config
    auto *feCfg = dynamic_cast<FrontEndCfg*>(frontEnd.get());

    if (options.doOutput) {
        const std::string filename = options.outputDir + "config_post_analysis.json";
        logger->info("Saving config of FE {} to {}",
                     feCfg->getName(), filename);
        ScanHelper::writeFeConfig(feCfg, filename);
    }

    // Save the output of the final level of output
    auto &output = *(clipResultOutput.back());

    std::string name = feCfg->getName();

    while(!output.empty()) {
        auto histo = output.popData();
        // only create the image files if asked to
        if(options.doPlots) {
            histo->plot(name, options.outputDir);
        }
        // always dump the data
        histo->toFile(name, options.outputDir);
    } // end while

    all_done = clk::now();
    logger->info("All done!");

    reportTimings();
}

void AnalysisConsoleImpl::reportTimings() const {
    auto to_ms = [](auto tt) {
      return std::chrono::duration_cast<std::chrono::milliseconds>(tt).count();
    };

    // Joining is done.
    ScanHelper::banner(logger,"Timing report");
    auto config_time = config_load-start_time;
    auto config_ms_count = to_ms(config_time);
    logger->info("-> Configuration: {} ms", config_ms_count);

    auto load_time = histogram_load-config_load;
    auto load_ms_count = to_ms(load_time);
    logger->info("-> Load histograms: {} ms", load_ms_count);

    auto prev_time = histogram_load;

    for(auto &td: tier_done) {
        auto tier_time = td-prev_time;
        auto tier_ms_count = to_ms(tier_time);
        if(tier_done.size() == 1) {
            logger->info("-> Analysis: {} ms", tier_ms_count);
        } else {
            logger->info("-> Analysis tier: {} ms", tier_ms_count);
        }
        prev_time = td;
    }

    auto plot_time = all_done - prev_time;
    auto plot_ms_count = to_ms(plot_time);
    logger->info("-> Plotting: {} ms", plot_ms_count);
}

void printHelp() {
    std::cout << "Analysis console help:\n";
    std::cout << " -h: Shows this help.\n";
    std::cout << " -s <scan_type> : Scan config\n";
    std::cout << " -c fe_config.json : Provide initial front end configuration\n";
    std::cout << " -y fe_type : FrontEnd config type\n";
    std::cout << " -t <target_charge> [<tot_target>] : Set target values for threshold/charge (and tot).\n";
    std::cout << " -p: Enable plotting of results.\n";
    std::cout << " -o <dir> : Output directory. (Default ./data/reanalysis)\n";
    std::cout << " -m <int> : 0 = pixel masking disabled, 1 = start with fresh pixel mask, default = pixel masking enabled\n";
    std::cout << " -l <log_cfg.json> : Provide logger configuration.\n";
}

int AnalysisConsoleImpl::init() {
    start_time = clk::now();

    json loggerConfig;

    if(!options.logCfgPath.empty()) {
        loggerConfig = ScanHelper::openJsonFile(options.logCfgPath);
        loggerConfig["outputDir"] = options.outputDir;
    } else {
        // default log setting
        loggerConfig["pattern"] = options.defaultLogPattern;
        loggerConfig["log_config"][0]["name"] = "all";
        loggerConfig["log_config"][0]["level"] = "info";
        loggerConfig["outputDir"]="";
    }
    spdlog::info("Configuring logger ...");
    logging::setupLoggers(loggerConfig);
    return 0;
}
