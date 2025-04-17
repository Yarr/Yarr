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

  class MyReceiver : public TriggerFeedbackReceiver {
    public:
      MyReceiver(FeedbackClipboardMap &fe) {
        connectClipboard(&fe);
      }

      void feedbackTrigger(unsigned channel, uint32_t code) override {}

    // private:
    //     FeedbackClipboardMap *clip;
  };

  // A simple analysis algorithm that generates feedback
  class MyAnalyzer : public AnalysisAlgorithm {
    std::unique_ptr<TriggerFeedbackSender> m_feedback;

  public:
    MyAnalyzer() : AnalysisAlgorithm() {}
    ~MyAnalyzer() {}

    void init(const ScanLoopInfo *s) override {
      // n_count = 1;
      for (unsigned n=0; n<s->size(); n++) {
        auto l = s->getLoop(n);
        if (l->isTriggerFeedbackLoop()) {
          m_feedback.reset(new TriggerFeedbackSender(feedback));
        }
      }
    }

    // void loadConfig(const json &j) override {
    //   for (unsigned i=0; i<j["parametersOfInterest"].size(); i++) {
    //     m_parametersOfInterest.push_back(j["parametersOfInterest"][i]);
    //   }
    // }

    void processHistogram(HistogramBase *h) override {
      if (h->getName() != "myHisto")
        return;

      auto h1 = dynamic_cast<Histo1d*>(h);
      int info = h1->getBin(0);

      m_feedback->feedbackTrigger(this->id, info);
    }

  // private:
  //   std::vector<unsigned> loops;
  //   std::vector<unsigned> loopMax;
  //   unsigned n_count;

  //   std::map<unsigned, std::unique_ptr<Histo1d>> hMap;
  //   std::map<unsigned, unsigned> innerCnt;
  };
}

// Test generation of trigger feedback
TEST_CASE("AnalysisTriggerThrottle", "[Analysis][trigger_feedback]") {

    ClipBoard<HistogramBase> input;
    ClipBoard<HistogramBase> output;

    int max_histo_push = 10;
    // int histo_occ = 10;

    json throttleCfg;

    SECTION ("Default") {
    }

    // SECTION ("Small occ") {
    //   throttleCfg["target_occ"] = 4;
    // }

    // SECTION ("Small trigs") {
    //   throttleCfg["target_trigs"] = 200;
    // }

    logger->debug("Throttle test with config {}", [&]() -> std::string {
      std::stringstream ss; ss << throttleCfg; return ss.str(); }());

    EmptyHw empty;
    Bookkeeper bookie(&empty, &empty);

    int rx_channel = 0;

    // This is for one FE
    AnalysisProcessor analysis(rx_channel);

    {
      auto proc = std::make_unique<MyAnalyzer>();

      REQUIRE (proc);

      proc->loadConfig(throttleCfg);

      analysis.addAlgorithm(std::move(proc));
    }

    FeedbackClipboardMap fbMap;

    // Need scan loops to lookup trigger loop
    ScanFactory scan(&bookie, &fbMap);

    for (unsigned id=0; id<bookie.getNumOfEntries(); id++ ) {
      logger->debug("Bookie has ID {}", id);
    }

    auto &fb = fbMap[rx_channel];

    analysis.connect(&scan, &input, &output, &fb);

    MyReceiver recv(fbMap);

    analysis.init();
    analysis.run();

    int nCol = 1;

    for(int i=0; i<max_histo_push; i++) {
      LoopStatus stat{{1, 2}, {LOOP_STYLE_DATA, LOOP_STYLE_TRIGGER_FEEDBACK}};
      auto h = std::make_unique<Histo1d>("myHisto", nCol, 0.5, nCol+0.5, stat);

      for(int c=0; c<nCol; c++) {
        h->fill(c, i);
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

    // int x = 2, y = 1, z = 3;
    // auto bin = histo_as_3d->binNum(x, y, z);
    // CAPTURE (x, y, z, bin);

    // float val = 1;
    // REQUIRE (histo_as_3d->getBin(bin) == val);
}
