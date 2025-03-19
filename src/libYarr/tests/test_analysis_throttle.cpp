#include "catch.hpp"

#include "AllAnalyses.h"
#include "Bookkeeper.h"
#include "FeedbackBase.h"
#include "Histo1d.h"
#include "ScanFactory.h"

#include "EmptyHw.h"

#include "logging.h"

namespace {
  auto logger = logging::make_log("test_analysis_throttle");

  /// Wrapper so we can manually add loops
  class MyScan : public ScanFactory {
  public:
    MyScan(Bookkeeper &k, FeedbackClipboardMap &fb) : ScanFactory(&k, &fb){}

    void addLoop(std::shared_ptr<LoopActionBase> l) {
      ScanFactory::addLoop(l);
    }
  };
// class GlobalFeedbackBase {
//     public:
//         virtual ~GlobalFeedbackBase() = default;
//         virtual void feedback(unsigned channel, double sign, bool last) = 0;
//         virtual void feedbackBinary(unsigned channel, double sign, bool last) = 0; // TODO Algorithm should be selected in scan
//         virtual void feedbackStep(unsigned channel, double sign, bool last) {}
// };

  class MyReceiver : public GlobalFeedbackReceiver {
    public:
      MyReceiver(FeedbackClipboardMap &fe) {
        connectClipboard(&fe);
      }

      void feedback(unsigned channel, double sign, bool last) override {}
      void feedbackBinary(unsigned channel, double sign, bool last) override {}
      void feedbackStep(unsigned channel, double sign, bool last) override {}
    //     /// Wait for feedback to be received and apply it
    //     void waitForFeedback(unsigned ch);

    // private:
    //     FeedbackClipboardMap *clip;
  };

}

TEST_CASE("AnalysisTriggerThrottle", "[Analysis][trigger_throttle]") {

    ClipBoard<HistogramBase> input;
    ClipBoard<HistogramBase> output;

    int max_histo_push = 10;
    int histo_occ = 10;

    json throttleCfg;

    SECTION ("Default") {
    }

    SECTION ("Small occ") {
      throttleCfg["target_occ"] = 4;
    }

    SECTION ("Small trigs") {
      throttleCfg["target_trigs"] = 200;
    }

    logger->debug("Throttle test with {}", [&]() -> std::string {
        std::stringstream ss; ss << throttleCfg; return ss.str(); }());

    EmptyHw empty;
    Bookkeeper bookie(&empty, &empty);

    int rx_channel = 0;

    // This is for one FE
    AnalysisProcessor analysis(rx_channel);

    {
      auto throttler = StdDict::getAnalysis("TriggerThrottleAnalysis");

      REQUIRE (throttler);

      throttler->loadConfig(throttleCfg);

      analysis.addAlgorithm(std::move(throttler));
    }

    FeedbackClipboardMap fbMap;

    // Need scan loops to lookup trigger loop
    ScanFactory scan(&bookie, &fbMap);

    for (unsigned id=0; id<bookie.getNumOfEntries(); id++ ) {
      logger->debug("Bookie has ID {}", id);
    }

    // Analysis will need to find some loops (eg trigger)    
    {
      json scanCfg;
      scanCfg["scan"]["name"] = "BasicAnalysis";

      // Create Loop objects so they're available to analysis
      scanCfg["scan"]["loops"][0]["loopAction"] = "StarThrottleLoop";
      // scanCfg["scan"]["loops"][0]["config"] = throttleCfg;
      scanCfg["scan"]["loops"][1]["loopAction"] = "StarTriggerLoop";
      scanCfg["scan"]["loops"][2]["loopAction"] = "StdDataLoop";

      scan.loadConfig(scanCfg);
    }

    auto &fb = fbMap[rx_channel];

    analysis.connect(&scan, &input, &output, &fb);

    MyReceiver recv(fbMap);

    analysis.init();
    analysis.run();

    int nCol = 4;
    int nRow = 3;

    for(int i=0; i<max_histo_push; i++) {
        LoopStatus stat{{1, 2}, {LOOP_STYLE_DATA, LOOP_STYLE_TRIGGER}};
        auto h = std::make_unique<Histo2d>("OccupancyMap", nCol, 0.5, nCol+0.5, nRow, 0.5, nRow+0.5, stat);

        for(int c=0; c<nCol; c++) {
            for(int r=0; r<nRow; r++) {
                h->fill(c, r, histo_occ);
            }
        }

        input.pushData(std::move(h));

        if (!output.empty()) {
          logger->debug("Exit histo loop as have output to check");
            break;
        }
    }

    input.finish();
    analysis.join();

    REQUIRE (!output.empty());

    REQUIRE (!fb.empty());

    // auto 

    std::unique_ptr<HistogramBase> result = output.popData();

    // Only one output histogram
    REQUIRE (output.empty());
    // output->pushData(std::move(outerOccMaps[ident]));

    CHECK (result->getXaxisTitle() == "Column");
    CHECK (result->getYaxisTitle() == "Row");
    CHECK (result->getZaxisTitle() == "Hits");
    CHECK (result->getName() == "OuterOccupancyMap-1");

    // int x = 2, y = 1, z = 3;
    // auto bin = histo_as_3d->binNum(x, y, z);
    // CAPTURE (x, y, z, bin);

    // float val = 1;
    // REQUIRE (histo_as_3d->getBin(bin) == val);
}
