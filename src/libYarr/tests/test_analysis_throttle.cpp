#include "catch.hpp"

#include "AllAnalyses.h"
#include "Bookkeeper.h"
#include "FeedbackBase.h"
#include "Histo1d.h"
#include "ScanFactory.h"

#include "EmptyHw.h"

#include "logging.h"

namespace {
  const int RX_CHANNEL = 0;
  auto logger = logging::make_log("test_analysis_throttle");

  /// Wrapper so we can manually add our test action
  class MyScan : public ScanFactory {
  public:
    MyScan(Bookkeeper &k, FeedbackClipboardMap &fb) : ScanFactory(&k, &fb){}

    void addLoop(std::shared_ptr<LoopActionBase> l) {
      ptr = l;
      ScanFactory::addLoop(l);
    }

    std::weak_ptr<LoopActionBase> ptr;
  };

  /// Loop action to receive trigger feedback (and record for testing)
  class MyReceiver : public LoopActionBase, public TriggerFeedbackReceiver {
    public:
      MyReceiver(FeedbackClipboardMap &fe, int stop_at)
        : LoopActionBase(LOOP_STYLE_TRIGGER_FEEDBACK),
          stop_at_value(stop_at)
      {
        connectClipboard(&fe);
      }

      void feedbackTrigger(unsigned channel, uint32_t code) override {
          logger->trace("Feedback trigger {}", code);
          feedback_count ++;
          latest_code = code;
      }

      void execPart2() override {
          logger->trace("End of loop wait for feedback");

          waitForFeedback(RX_CHANNEL);

          logger->warn("part2 Compare {} {}", latest_code, stop_at_value);

          if(latest_code == stop_at_value) {
              m_done = true;
          }
      }

      uint32_t latest_code{};
      size_t feedback_count = 0;
      int stop_at_value{};
  };

  /// A simple analysis algorithm that generates feedback
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

    void processHistogram(HistogramBase *h) override {
      if (h->getName() != "myHisto")
        return;

      histo_count ++;

      auto h1 = dynamic_cast<Histo1d*>(h);
      int info = h1->getBin(0);

      REQUIRE(m_feedback);

      logger->trace("Send feedback {}", info);
      m_feedback->feedbackTrigger(this->id, info);
    }

    unsigned histo_count;
  };
}

// Test generation of trigger feedback (no analysis is checked)
TEST_CASE("AnalysisTriggerThrottle", "[Analysis][trigger_feedback]") {
    ClipBoard<HistogramBase> input;
    ClipBoard<HistogramBase> output_unused;

    int max_histo_push = 10;

    json throttleCfg;

    SECTION ("Default") {
    }

    EmptyHw empty;
    Bookkeeper bookie(&empty, &empty);

    FeedbackClipboardMap fbMap;

    MyScan scan(bookie, fbMap);

    // Set up loop config with trigger feedback

    // Test action is not in the registry, add it by hand.
    {
      std::unique_ptr<LoopActionBase> r
        = std::make_unique<MyReceiver>(fbMap, max_histo_push - 1);
      scan.addLoop(std::move(r));
    }

    // This is for one FE
    AnalysisProcessor analysis(RX_CHANNEL);

    {
      auto proc = std::make_unique<MyAnalyzer>();

      REQUIRE (proc);

      proc->loadConfig(throttleCfg);

      analysis.addAlgorithm(std::move(proc));
    }

    auto &fb = fbMap[RX_CHANNEL];

    analysis.connect(&scan, &input, &output_unused, &fb);

    analysis.init();

    // Runs until the input signals it is complete
    analysis.run();

    int nCol = 1;

    std::thread scanner([&] { scan.run(); });

    for(int i=0; i<max_histo_push; i++) {
      LoopStatus stat{{2}, {LOOP_STYLE_TRIGGER_FEEDBACK}};
      auto h = std::make_unique<Histo1d>("myHisto", nCol, -0.5, nCol-0.5, stat);

      logger->trace("Send histo {}", i);
      for(int c=0; c<nCol; c++) {
        h->fill(c, i);
      }

      input.pushData(std::move(h));
    }

    // End of histograms sent from analysis algorithm
    input.finish();

    scanner.join();

    analysis.join();

    // All feedback should have been read by the receiver
    CHECK (fb.empty());

    // Both clipboards received the same amount of data
    CHECK (input.getNumDataIn() == max_histo_push);
    CHECK (fb.getNumDataIn() == max_histo_push);

    CHECK (input.getNumDataOut() == max_histo_push);
    CHECK (fb.getNumDataOut() == max_histo_push);

    // Check that the feedback receiver received the correct number of histograms
    REQUIRE (!scan.ptr.expired());
    auto sh = scan.ptr.lock();
    REQUIRE (sh.get());

    auto fb_loop = dynamic_cast<MyReceiver*>(sh.get());

    REQUIRE (fb_loop);

    CHECK (fb_loop->feedback_count == max_histo_push);
    CHECK (fb_loop->latest_code == max_histo_push - 1);
}
