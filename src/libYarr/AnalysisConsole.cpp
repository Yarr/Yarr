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

void printHelp();

class BasicScanInfo : public ScanLoopInfo {
        std::vector<std::unique_ptr<LoopActionBaseInfo>> loops;

    public:
        unsigned size() const { return loops.size(); }
        LoopActionBaseInfo *getLoop(unsigned n) const {
            return loops[n].get();
        }

        void loadScanConfg(const json &scanCfg);
};

class AnalysisConsoleImpl {
    AnalysisOpts options;

    std::unique_ptr<FrontEnd> frontEnd;

    ClipBoard<HistogramBase> clipHistoInput;
    std::vector<std::unique_ptr<ClipBoard<HistogramBase>> > clipResultOutput;

    std::vector<std::unique_ptr<DataProcessor>> analysisProcessors;

  public:
    AnalysisConsoleImpl(const AnalysisOpts &opts);

    int init();

    int loadConfig();

    void loadHistograms();

    void buildAnalyses(std::vector<std::unique_ptr<DataProcessor>> &analyses,
                       ClipBoard<HistogramBase> &input,
                       std::vector<std::unique_ptr<ClipBoard<HistogramBase>>> &output_vector,
                       const json& scanCfg,
                       const ScanLoopInfo &s,
                       int mask_opt, std::string outputDir,
                       int target_tot, int target_charge);

    int loadConfigFile(const AnalysisOpts &anOpts, bool writeConfig, json &config);

    void setupAnalysis();
    void pushHisto(std::unique_ptr<HistogramBase> h);
    void run();
    void saveAndPlot();
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
    pimpl->setupAnalysis();
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
        c = getopt_long(argc, argv, "hn:s:n:m:c:t:pgo:W:y:d:u:i:l:QIz", long_options, &opt_index);
        int count = 0;
        if(c == -1) break;
        switch (c) {
        case 'h':
            printHelp();
            return 0;
        case 's':
            // anOpts.scan_config_provided = true;
            anOpts.scanFile = std::string(optarg);
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
            } else if (optopt == 'g' || optopt == 'c') {
                spdlog::error("Option {} requires a parameter! Aborting... ", (char) optopt);
                return -1;
            } else {
                spdlog::error("Unknown parameter: {}", (char) optopt);
            }
            break;
        default:
            spdlog::critical("Error while parsing command line parameters!");
            std::cout << "Rerun with --help for more information\n";
            return -1;
        }
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
        //      = std::string(optarg);
    }

    return 1;
}

AnalysisConsoleImpl::AnalysisConsoleImpl(const AnalysisOpts &opts)
  : options(opts)
{
}

void AnalysisConsoleImpl::loadHistograms() {
    // Now processor is running, so we can push histograms
    for(auto &h: options.histogramFiles) {
        auto histo = HistogramBase::fromJson(h);
        pushHisto(std::move(histo));
    }
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

        // if(auto *fbGlobal = dynamic_cast<GlobalFeedbackReceiver*>(&*action)) {
        //     fbGlobal->connectClipboard(feedback);
        // }
        // if(auto *fbPixel = dynamic_cast<PixelFeedbackReceiver*>(&*action)) {
        //     fbPixel->connectClipboard(feedback);
        // }

        loops.push_back(std::move(action));

        // json tCfg;
        // action->writeConfig(tCfg);
        // if (!tCfg.empty()) {
        //     std::stringstream ss;
        //     ss << tCfg;
        //     std::string line;
        //     while (std::getline(ss, line)) logger->info("~~~ {}", line); //yes overkill, i know ..
        // } else {
        //     logger->warn("~~~ Config empty.");
        // }
    }
}

int AnalysisConsoleImpl::loadConfigFile(const AnalysisOpts &anOpts, bool writeConfig, json &config)
{
    // Compared to ScanHelper version, skip controller config completely

    // load single FE config
    json chipConfig=json::array();

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

int AnalysisConsoleImpl::loadConfig()
{
    json scanConsoleConfig;

    int result = loadConfigFile(options, true, scanConsoleConfig);
    if(result<0) {
        logger->error("Failed to read configs");
        return -1;
    }

    std::cout << scanConsoleConfig << std::endl;

    return 0;
}

void AnalysisConsoleImpl::buildAnalyses(std::vector<std::unique_ptr<DataProcessor>> &analyses,
                                        ClipBoard<HistogramBase> &input,
                                        std::vector<std::unique_ptr<ClipBoard<HistogramBase>>> &output_vector,
                                        const json& scanCfg,
                                        const ScanLoopInfo &s,
                                        int mask_opt, std::string outputDir,
                                        int target_tot, int target_charge)
{
    logger->info("Loading analyses ...");

    const json &anaCfg = scanCfg["scan"]["analysis"];

    // Parse scan config and build analysis hierarchy
    // Use a 2D vector to hold algorithm indices for all tiers of analysis processors
    ScanHelper::AlgoTieredIndex algoIndexTiers;
    try {
        ScanHelper::buildAnalysisHierarchy(algoIndexTiers, anaCfg);
    } catch (std::runtime_error &e) {
        logger->error("Building analysis hierarchy: {}", e.what());
        throw(std::runtime_error("buildAnalyses failure"));
    }

    bool indexed;

    // Is this an array of objects, or "n_count" + indexed by string "0"
    if (anaCfg.contains("n_count")) {
        indexed = true;
    } else {
        indexed = false;
    }

    auto get_algorithm = [indexed, &anaCfg](int index) {
        if(indexed) {
            return anaCfg[std::to_string(index)];
        } else {
            return anaCfg[index];
        }
    };

    // FrontEnd *fe = bookie.getEntry(id).fe;
    for (unsigned t=0; t<algoIndexTiers.size(); t++) {
        // Before adding new analyses
        bool hasUpstreamAnalyses = false;
        if (t > 0) { // ie. not analyses[fe].empty()
            auto& ana_prev = dynamic_cast<AnalysisProcessor&>( *(analyses.back()) );
            hasUpstreamAnalyses = not ana_prev.empty();
        }

        // Should be ignored?
        int test_id = 0;
        // Add analysis processors
        analyses.emplace_back( new AnalysisProcessor(test_id) );
        auto& ana = dynamic_cast<AnalysisProcessor&>( *(analyses.back()) );

        // Create the ClipBoard to store its output and establish connection
        output_vector.emplace_back(new ClipBoard<HistogramBase>());
        if (t==0) {
            ana.connect(&s, &input, (output_vector.back()).get(), nullptr);
        } else {
            ana.connect(&s, (*(output_vector.rbegin()+1)).get(),
                        (*(output_vector.rbegin())).get(),
                        nullptr, true);
        }

        auto add_analysis = [&](std::string algo_name, json& j) {
            auto analysis = StdDict::getAnalysis(algo_name);
            if(analysis) {
                logger->debug("  ... adding {}", algo_name);
                analysis->loadConfig(j);
                // If it requires dependency
                if (analysis->requireDependency() and not hasUpstreamAnalyses) {
                    logger->error("Analysis {} requires outputs from other analyses", algo_name);
                    throw("buildAnalyses failure");
                }

                logger->debug(" connecting feedback (if required)");

                // No point as it's just writing out again?
                // if(algo_name == "HistogramArchiver") {
                //     auto archiver = dynamic_cast<HistogramArchiver*>(analysis.get());
                //     archiver->setOutputDirectory(outputDir);
                // }

                // TODO, pass in correct FrontEnd to allow writing changes
                // analysis->setConfig(bookie.getFeCfg(id));
                analysis->setParams(target_tot, target_charge);

                ana.addAlgorithm(std::move(analysis));
            } else {
              logger->error("Error, Analysis Algorithm \"{} unknown, skipping!", algo_name);
            }
        };

        // Add all AnalysisAlgorithms of the t-th tier
        for (int aIndex : algoIndexTiers[t]) {
            std::string algo_name = get_algorithm(aIndex)["algorithm"];
            json algo_config = get_algorithm(aIndex)["config"];
            add_analysis(algo_name, algo_config);
        }

        // Disable masking of pixels
        if(mask_opt == 0) {
            logger->info("Disabling masking for this scan!");
            ana.setMasking(false);
        }

        // From the frontend
        FrontEndGeometry geo{100, 100};
        ana.setMapSize(geo.nCol, geo.nRow);
    }
}

void AnalysisConsoleImpl::setupAnalysis()
{
    json scanCfg;

    BasicScanInfo scanInfo;

    buildAnalyses(analysisProcessors,
                  clipHistoInput,
                  clipResultOutput,
                  scanCfg, scanInfo,
                  options.mask_opt, options.outputDir,
                  options.target_tot, options.target_charge);

    // Run from downstream to upstream
    logger->info("Starting analysis threads:");
    for (auto& ana : analysisProcessors) {
        ana->init();
        ana->run();
    }

    logger->info(" .. started threads");

    return;
}

void AnalysisConsoleImpl::pushHisto(std::unique_ptr<HistogramBase> h) {
    clipHistoInput.pushData(std::move(h));
}

void AnalysisConsoleImpl::run() {
    ScanHelper::banner(logger,"Run Analysis");

    // scan_start = std::chrono::steady_clock::now();

    // Join from upstream to downstream.
    clipHistoInput.finish();

    // scan_done = std::chrono::steady_clock::now();
    for (unsigned i=0; i<analysisProcessors.size(); i++) {
      logger->info("Waiting for analysis level to finish ...");

      // First level is taking from input
      analysisProcessors[i]->join();
      clipResultOutput[i]->finish();
    }

    // all_done = std::chrono::steady_clock::now();
    logger->info("All done!");

    // Joining is done.
    // ScanHelper::banner(logger,"Timing");
    // logger->info("-> Configuration: {} ms", std::chrono::duration_cast<std::chrono::milliseconds>(cfg_end-cfg_start).count());
    // logger->info("-> Scan:          {} ms", std::chrono::duration_cast<std::chrono::milliseconds>(scan_done-scan_start).count());
    // logger->info("-> Processing:    {} ms", std::chrono::duration_cast<std::chrono::milliseconds>(processor_done-scan_done).count());
    // logger->info("-> Analysis:      {} ms", std::chrono::duration_cast<std::chrono::milliseconds>(all_done-processor_done).count());

    // scanLog["stopwatch"]["config"] = (uint32_t) std::chrono::duration_cast<std::chrono::milliseconds>(cfg_end-cfg_start).count();
    // scanLog["stopwatch"]["scan"] = (uint32_t) std::chrono::duration_cast<std::chrono::milliseconds>(scan_done-scan_start).count();
    // scanLog["stopwatch"]["processing"] = (uint32_t) std::chrono::duration_cast<std::chrono::milliseconds>(processor_done-scan_done).count();
    // scanLog["stopwatch"]["analysis"] = (uint32_t) std::chrono::duration_cast<std::chrono::milliseconds>(all_done-processor_done).count();

    // scanLog["finishTime"] = (int)std::time(nullptr);

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
}

void printHelp() {
    std::cout << "Help:\n";
    std::cout << " -h: Shows this help.\n";
    std::cout << " -s <scan_type> : Scan config\n";
    std::cout << " -c fe_config.json : Provide front end configuration\n";
    std::cout << " -y fe_type : FrontEnd config type\n";
    std::cout << " -t <target_charge> [<tot_target>] : Set target values for threshold/charge (and tot).\n";
    std::cout << " -p: Enable plotting of results.\n";
    std::cout << " -o <dir> : Output directory. (Default ./data/)\n";
    std::cout << " -m <int> : 0 = pixel masking disabled, 1 = start with fresh pixel mask, default = pixel masking enabled\n";
    std::cout << " -l <log_cfg.json> : Provide logger configuration.\n";
}

int AnalysisConsoleImpl::init() {
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
