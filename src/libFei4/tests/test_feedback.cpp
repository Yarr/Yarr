#include "catch.hpp"

#include <thread>

#include "AllChips.h"
#include "AllStdActions.h"
#include "FeedbackBase.h"
#include "ScanFactory.h"

#include "logging.h"
auto logger = logging::make_log("test_feedback");

#include "EmptyHw.h"

class MyHardware : public EmptyHw {
  uint32_t trigEnable;
public:
  MyHardware() : trigEnable(0) {}

  void setTrigEnable(uint32_t value) override { trigEnable = value;}
  uint32_t getTrigEnable() override { return trigEnable; }
};

// Just create scan engine side
TEST_CASE("FeedbackTestEmpty", "[Feedback]") {
    MyHardware empty;
    Bookkeeper bookie(&empty, &empty);
    ScanFactory scan(&bookie, nullptr);

    auto g_fe = StdDict::getFrontEnd("FEI4B");
    g_fe->makeGlobal();
    g_fe->init(&empty, FrontEndConnectivity(0,0));
    bookie.initGlobalFe(g_fe.release());

    json js;
    js["scan"]["name"] = "TestScan";
    js["scan"]["loops"][0]["loopAction"] = "Fei4GlobalFeedback";
    js["scan"]["loops"][0]["config"]["parameter"] = "Vthin_Fine";
    js["scan"]["loops"][1]["loopAction"] = "Fei4TriggerLoop";
    js["scan"]["loops"][2]["loopAction"] = "StdDataLoop";

    scan.loadConfig(js);

    scan.init();

    // Skip pre/post scan
    scan.run();
}

// Just create scan engine side
TEST_CASE("FeedbackTestGlobal", "[Feedback]") {
    MyHardware empty;
    Bookkeeper bookie(&empty, &empty);
    FeedbackClipboardMap fb;
    ScanFactory scan(&bookie, &fb);

    auto fe = StdDict::getFrontEnd("FEI4B").release();
    fe->setActive(true);

    unsigned rx_channel = 0;
    FrontEndConnectivity fe_conn(0,rx_channel);
    fe->init(&empty, fe_conn);
    bookie.addFe(fe, fe_conn);
    unsigned feUid = bookie.getId(fe);

    auto g_fe = StdDict::getFrontEnd("FEI4B");
    g_fe->makeGlobal();
    g_fe->init(&empty, FrontEndConnectivity(0,0));
    bookie.initGlobalFe(g_fe.release());

    json js;
    js["scan"]["name"] = "TestScan";
    js["scan"]["loops"][0]["loopAction"] = "Fei4GlobalFeedback";
    js["scan"]["loops"][0]["config"]["parameter"] = "Vthin_Fine";
    js["scan"]["loops"][1]["loopAction"] = "Fei4TriggerLoop";
    js["scan"]["loops"][2]["loopAction"] = "StdDataLoop";

    scan.loadConfig(js);

    scan.init();

    GlobalFeedbackSender send(&fb[feUid]);

    uint32_t feedback_count = 0;

    const int max_loops = 10;

    bool thread_failure = true;

    std::thread t([&]() {
        int loop_count = 0;
        while(feedback_count < 2) {
          std::shared_ptr<RawDataContainer> data = std::make_shared<RawDataContainer>(LoopStatus({1, 1, 2}, {LOOP_STYLE_DATA, LOOP_STYLE_TRIGGER, LOOP_STYLE_GLOBAL_FEEDBACK}));
          logger->debug("Received data");

          const LoopStatus &stat = data->stat;

          REQUIRE (stat.size() == 3);
          logger->trace("Current loop status: {} {} {}",
                        stat.get(0), stat.get(1), stat.get(2));

          // As there's no inner loop, send feedback as soon as data arrives
          send.feedbackBinary(feUid, 1, true);
          feedback_count ++;

          logger->debug("Sent feedback at iteration {}", loop_count);
          loop_count ++;

        }
        logger->warn("Finish unlocking thread after {} loop", max_loops);
        thread_failure = false;

      });

    // Skip pre/post scan
    scan.run();

    bookie.getEntry(feUid).fe->clipRawData.finish();

    t.join();

    REQUIRE (feedback_count == 2);

    REQUIRE (!thread_failure);
}

// Just create scan engine side
TEST_CASE("FeedbackTestPixel", "[Feedback]") {
    MyHardware empty;
    Bookkeeper bookie(&empty, &empty);
    FeedbackClipboardMap fb;
    ScanFactory scan(&bookie, &fb);

    unsigned rx_channel = 0;

    FrontEnd* fe = StdDict::getFrontEnd("FEI4B").release();
    fe->setActive(true);
    FrontEndConnectivity fe_conn(0,rx_channel);
    fe->init(&empty, fe_conn);
    bookie.addFe(fe, fe_conn);
    unsigned feUid = bookie.getId(fe);

    auto g_fe = StdDict::getFrontEnd("FEI4B");
    g_fe->makeGlobal();
    g_fe->init(&empty, FrontEndConnectivity(0,0));
    bookie.initGlobalFe(g_fe.release());

    json js;
    js["scan"]["name"] = "TestScan";
    js["scan"]["loops"][0]["loopAction"] = "Fei4PixelFeedback";
    js["scan"]["loops"][0]["config"]["parameter"] = "FDAC_FB";
    js["scan"]["loops"][0]["config"]["step"] = 4;
    js["scan"]["loops"][1]["loopAction"] = "Fei4TriggerLoop";
    js["scan"]["loops"][2]["loopAction"] = "StdDataLoop";

    scan.loadConfig(js);

    scan.init();

    PixelFeedbackSender send(&fb[feUid]);

    uint32_t feedback_count = 0;

    const int max_loops = 10;

    bool thread_failure = true;

    std::thread t([&]() {
        int loop_count = 0;
        while(feedback_count<4) {
          std::shared_ptr<RawDataContainer> data = std::make_shared<RawDataContainer>(LoopStatus({1, 1, 4}, {LOOP_STYLE_DATA, LOOP_STYLE_TRIGGER, LOOP_STYLE_PIXEL_FEEDBACK}));

          logger->debug("Received data");

          const LoopStatus &stat = data->stat;

          REQUIRE (stat.size() == 3);
          logger->trace("Current loop status: {} {} {}",
                        stat.get(0), stat.get(1), stat.get(2));

          // As there's no inner loop, send feedback as soon as data arrives
          auto h = std::make_unique<Histo2d>("Test", 80, 0, 20, 336, 0, 20);
          send.feedback(feUid, std::move(h));
          feedback_count ++;

          logger->debug("Sent feedback at iteration {}", loop_count);
          loop_count ++;

        }
        logger->warn("Finish unlocking thread after {} loop", max_loops);
        thread_failure = false;
        return;
      });

    // Skip pre/post scan
    scan.run();

    bookie.getEntry(feUid).fe->clipRawData.finish();

    t.join();

    // 4,2,1,1
    REQUIRE (feedback_count == 4);

    REQUIRE (!thread_failure);
}
