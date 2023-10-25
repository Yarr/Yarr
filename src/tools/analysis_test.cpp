#include "AllAnalyses.h"
#include "ClipBoard.h"
#include "ScanHelper.h"
#include "ScanOpts.h"

#include "logging.h"
#include "LoggingConfig.h"

class BasicScanInfo : public ScanLoopInfo {
    public:
        unsigned size() const { return loops.size(); }
        LoopActionBaseInfo *getLoop(unsigned n) const {
            return loops[n].get();
        }

        std::vector<std::unique_ptr<LoopActionBaseInfo>> loops;
};

namespace {
    auto logger = logging::make_log("AnalysisTest");
}

class AnalysisConsole {
    ScanOpts options;

    ClipBoard<HistogramBase> clipHistoInput;
    std::vector<std::unique_ptr<ClipBoard<HistogramBase>> > clipResultOutput;

    std::vector<std::unique_ptr<DataProcessor>> analysisProcessors;

  public:
    AnalysisConsole(const ScanOpts &opts);

    int loadConfig();

    void buildAnalyses(std::vector<std::unique_ptr<DataProcessor>> &analyses,
                       ClipBoard<HistogramBase> &input,
                       std::vector<std::unique_ptr<ClipBoard<HistogramBase>>> &output_vector,
                       const json& scanCfg,
                       const ScanLoopInfo &s,
                       int mask_opt, std::string outputDir,
                       int target_tot, int target_charge);

    void setupAnalysis();
};

int AnalysisConsole::loadConfig()
{
    json scanConsoleConfig;

    int result = ScanHelper::loadConfigFile(options, true, scanConsoleConfig);
    if(result<0) {
        logger->error("Failed to read configs");
        return -1;
    }

    std::cout << scanConsoleConfig << std::endl;

    return 0;
}

void AnalysisConsole::buildAnalyses(std::vector<std::unique_ptr<DataProcessor>> &analyses,
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

void AnalysisConsole::setupAnalysis()
{
    json scanCfg;

    ScanOpts scanOpts;

    BasicScanInfo scanInfo;

    buildAnalyses(analysisProcessors,
                  clipHistoInput,
                  clipResultOutput,
                  scanCfg, scanInfo,
                  scanOpts.mask_opt, scanOpts.outputDir,
                  scanOpts.target_tot, scanOpts.target_charge);

    // Run from downstream to upstream
    logger->info("Starting analysis threads:");
    for (auto& ana : analysisProcessors) {
        ana->init();
        ana->run();
    }

    logger->info(" .. started threads");

    return;
}

int main(int argc, char *argv[])
{
    ScanOpts options;

    int res = ScanHelper::parseOptions(argc, argv, options);

    if(res<=0) return res;

    // ScanConsole con;
    // con.init(argc, argv);
    // if(res<=0) return res;

    AnalysisConsole ac(options);

    res=ac.loadConfig();

    if(res!=0) {
      exit(res);
    }

    ac.setupAnalysis();

    // Now processor is running, so we can push histograms
    // ac.pushHisto();

    // ac.run();
    // // con.cleanup();
    // con.plot();

    return 0;
}
