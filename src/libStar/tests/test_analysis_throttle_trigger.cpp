#include "catch.hpp"

#include "AllAnalyses.h"
#include "Bookkeeper.h"
#include "FrontEndClipBoards.h"
#include "Histo1d.h"
#include "ScanFactory.h"
#include "StdTriggerAction.h"

#include "EmptyHw.h"

#include "logging.h"

namespace {
auto logger = logging::make_log("test_analysis_throttle_trigger");

struct TestSetup {
  int chip_count{1};

  // Description of occupancy in each of a sequence of input histograms
  std::vector<int> occ_sequence{2};

  json analysisCfg;

  // What feedback vals to expect
  std::vector<uint32_t> feedback_sequence{1};
};

/// Trivial implementation of scan loop info
class TestScanInfo : public ScanLoopInfo {
        struct TestLoopInfo
          : public LoopActionBaseInfo,
            public StdTriggerAction
        {
          TestLoopInfo(LoopStyle s, int mn, int mx, unsigned st)
            : LoopActionBaseInfo(s) {
            min = mn;
            max = mx;
            step = st;
          }
        };

        std::vector<TestLoopInfo> loops;

    public:
        TestScanInfo(const std::vector<TestLoopInfo> &in)
          : loops(in) {}
        unsigned size() const override { return loops.size(); }

        const LoopActionBaseInfo *getLoop(unsigned n) const override {
            return &loops[n];
        }
};

} // End anonymous namespace

/**
   Test of trigger throttle analysis.

   Generate sequence of histograms (from OccupancyMap), and check expected
   output.
*/
TEST_CASE("StarTriggerThrottleAnalysis", "[Analysis][Star][Throttle]")
{
    TestSetup info = GENERATE
      (
       TestSetup{},
       TestSetup{2, {}}
       );

    unsigned histo_count = info.occ_sequence.size();

    CAPTURE (histo_count);
    CAPTURE (info.chip_count);

    EmptyHw empty;
    Bookkeeper bookie(&empty, &empty);

    const unsigned RX_CHANNEL = 0;

    // This is for one FE
    AnalysisProcessor analysis(RX_CHANNEL);

    int nCol = 128 * info.chip_count;
    int nRow = 2;

    {
      auto ana = StdDict::getAnalysis("StarTriggerThrottleAnalysis");

      REQUIRE (ana);

      ana->loadConfig(info.analysisCfg);

      ana->setMapSize(nCol, nRow);

      analysis.addAlgorithm(std::move(ana));
    }

    // Need scan loops to lookup trigger loop
    FeedbackClipboardMap fb;
    // ScanFactory scan(&bookie, &fb);

    // Setup the loops that analysis needs to know about
    TestScanInfo scan{{
        {LOOP_STYLE_TRIGGER_FEEDBACK, 0, int(histo_count - 1), 1}
    }};

    // Input to analysis
    ClipBoard<HistogramBase> input;
    // Output from  analysis
    ClipBoard<HistogramBase> output;

    FeedbackClipboard fbcp;
    
    analysis.connect(&scan, &input, &output, &fbcp);

    analysis.init();

    // Runs until input is done
    analysis.run();

    for(unsigned i=0; i<histo_count; i++) {
        LoopStatus stat{{i}, {LOOP_STYLE_TRIGGER_FEEDBACK}};
        auto hist = std::make_unique<Histo2d>("OccupancyMap",
                                                nCol, 0.5, nCol + 0.5,
                                                nRow, 0.5, nRow + 0.5, stat);

        CHECK(hist->getYbins() == nRow);
        CHECK(hist->getXbins() == nCol);

        // Based on occ_sequence
        auto occ = info.occ_sequence[i];

        for(int c=0; c<nCol; c++) {
          for(int r=0; r<nRow; r++) {
            hist->fill(c + 1, r + 1, occ);
          }
        }

        input.pushData(std::move(hist));

        if (!output.empty()) {
            logger->debug("Exit histo loop as have output to check");
            break;
        }
    }
    // PixelFeedbackSender send(&fb[feUid]);

    input.finish();
    analysis.join();

    REQUIRE (!output.empty());

    int out_histo_count = 0;

    // Do some very basic checks on output
    while(!output.empty()) {
        std::unique_ptr<HistogramBase> result = output.popData();

        auto output_name = result->getName();

        CAPTURE (output_name);

        histo_count ++;

        if(output_name.find("NumTriggers") == 0) {
            CHECK (result->getXaxisTitle() == "x");
            CHECK (result->getYaxisTitle() == "y");
            CHECK (result->getZaxisTitle() == "z");

            auto hh = dynamic_cast<Histo1d*>(result.get());
            REQUIRE (hh != nullptr);

            CHECK (hh->size() == histo_count);
            CHECK (hh->getBin(0) == 2);
        } else {
            // Info about any other data
            CAPTURE (result->getXaxisTitle());
            CAPTURE (result->getYaxisTitle());
            CAPTURE (result->getZaxisTitle());

            logger->error("Extra histogram {}: {}", output_name,
                          [&]() -> std::string {
                json j; result->toJson(j);
                std::stringstream ss; ss << j; return ss.str(); }());

            CHECK (false);
        }
    }

    CHECK (out_histo_count == 1);
}
