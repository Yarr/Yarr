#include "catch.hpp"

#include "AllAnalyses.h"
#include "Bookkeeper.h"
#include "FeedbackBase.h"
#include "FrontEndClipBoards.h"
#include "Histo1d.h"
#include "ScanFactory.h"
#include "StdTriggerAction.h"

#include "EmptyHw.h"

#include "logging.h"

namespace {
auto logger = logging::make_log("test_analysis_throttle_trigger");

struct TestSetup {
  // Description of occupancy in each of a sequence of input histograms
  std::vector<int> occ_sequence{};

  std::vector<int> trig_count_sequence{};

  // What feedback vals to expect
  std::vector<uint32_t> feedback_sequence{};

  // Configuration for StarTriggerThrottleAnalysis
  json analysisCfg;
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
       // High occupancy so immediately complete
       TestSetup{{200}, {300}, {unsigned(-1)}, {}},
       // Low trigger rate so request more
       TestSetup{{40, 40, 40, 40}, {80, 80, 80, 80},
                 {1, 1, unsigned(-1), unsigned(-1)}, {}},

       // Default params (last, without a comma)
       TestSetup{}
       );

    unsigned histo_count = info.occ_sequence.size();

    CAPTURE (histo_count);

    EmptyHw empty;
    Bookkeeper bookie(&empty, &empty);

    const unsigned RX_CHANNEL = 0;

    // This is for one FE
    AnalysisProcessor analysis(RX_CHANNEL);

    int chip_count = 1;
    int nCol = 128 * chip_count;
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

    // Record what analysis feeds back to trigger
    FeedbackClipboard fbcp;

    analysis.connect(&scan, &input, &output, &fbcp);

    analysis.init();

    // Runs until input is done
    analysis.run();

    unsigned trig_count = 0;
    unsigned occ_count = 0;

    for(unsigned i=0; i<histo_count; i++) {
        if(!info.trig_count_sequence.empty()) {
            while(!input.empty()) {
              // logger->debug("Wait for histo to be processed before setting trigger count");
            }

            auto l = scan.getLoop(0);
            auto tc = dynamic_cast<const StdTriggerAction*>(l);
            auto t = const_cast<StdTriggerAction*>(tc);
            auto ntrigs = info.trig_count_sequence[i];
            t->setTrigCnt(ntrigs);
            trig_count += ntrigs;
        }

        LoopStatus stat{{i}, {LOOP_STYLE_TRIGGER_FEEDBACK}};
        auto hist = std::make_unique<Histo2d>("OccupancyMap",
                                                nCol, 0.5, nCol + 0.5,
                                                nRow, 0.5, nRow + 0.5, stat);

        CHECK(hist->getYbins() == nRow);
        CHECK(hist->getXbins() == nCol);

        // Based on occ_sequence
        auto occ = info.occ_sequence[i];

        occ_count += occ;

        for(int c=0; c<nCol; c++) {
          for(int r=0; r<nRow; r++) {
            hist->fill(c + 1, r + 1, occ);
          }
        }

        input.pushData(std::move(hist));
    }

    input.finish();
    analysis.join();

    if(histo_count) {
      CHECK (!fbcp.empty());
    }

    for(unsigned fb_it = 0; fb_it < info.feedback_sequence.size(); fb_it ++) {
      CAPTURE(fb_it);
      auto &fb_exp = info.feedback_sequence[fb_it];
      CHECK(!fbcp.empty());
      std::unique_ptr<FeedbackParams> fb = fbcp.popData();
      CHECK(fb->trigger().info == fb_exp);
    }

    // Checked contents, should now be empty
    CHECK (fbcp.empty());

    REQUIRE (!output.empty());

    int out_histo_count = 0;

    // Do some very basic checks on output
    while(!output.empty()) {
        std::unique_ptr<HistogramBase> result = output.popData();

        auto output_name = result->getName();

        CAPTURE (output_name);

        out_histo_count ++;

        if(output_name.find("NumTriggers") == 0) {
            CHECK (result->getXaxisTitle() == "x");
            CHECK (result->getYaxisTitle() == "y");
            CHECK (result->getZaxisTitle() == "z");

            auto hh = dynamic_cast<Histo1d*>(result.get());
            REQUIRE (hh != nullptr);

            // Only one bin
            CHECK (hh->size() == 1);
            CHECK (hh->getBin(0) == trig_count);
        } else if(output_name.find("OccupancyMapAllBunches") == 0) {
            CHECK (result->getXaxisTitle() == "Column");
            CHECK (result->getYaxisTitle() == "Row");
            CHECK (result->getZaxisTitle() == "Hits");

            auto hh = dynamic_cast<Histo2d*>(result.get());
            REQUIRE (hh != nullptr);

            // Strips in one ASIC
            CHECK (hh->size() == 256);
            CAPTURE (occ_count, trig_count);
            CHECK (hh->getBin(0) == Catch::Approx(occ_count / double(trig_count)));
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

    // OccupancyMapAllBunches only created if there are histos
    CHECK (out_histo_count == 1 + (histo_count != 0));
}
