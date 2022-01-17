#define CATCH_CONFIG_MAIN
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
    g_fe->init(&empty, 0, 0);
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

    fe->init(&empty, 0, 0);
    bookie.addFe(fe, 0, rx_channel);

    auto g_fe = StdDict::getFrontEnd("FEI4B");
    g_fe->makeGlobal();
    g_fe->init(&empty, 0, 0);
    bookie.initGlobalFe(g_fe.release());

    json js;
    js["scan"]["name"] = "TestScan";
    js["scan"]["loops"][0]["loopAction"] = "Fei4GlobalFeedback";
    js["scan"]["loops"][0]["config"]["parameter"] = "Vthin_Fine";
    js["scan"]["loops"][1]["loopAction"] = "Fei4TriggerLoop";
    js["scan"]["loops"][2]["loopAction"] = "StdDataLoop";

    scan.loadConfig(js);

    scan.init();

    GlobalFeedbackSender send(&fb[rx_channel]);

    uint32_t feedback_count = 0;

    const int max_loops = 10;

    bool thread_failure = true;

    std::thread t([&]() {
        int loop_count = 0;
        while(1) {
          bookie.rawData.waitNotEmptyOrDone();
          auto data = bookie.rawData.popData();
          if(!data) {
            // Return due to finish call
            thread_failure = false;
            return;
          }

          logger->debug("Received data");

          const LoopStatus &stat = data->stat;

          REQUIRE (stat.size() == 3);
          logger->trace("Current loop status: {} {} {}",
                        stat.get(0), stat.get(1), stat.get(2));

          // As there's no inner loop, send feedback as soon as data arrives
          send.feedbackBinary(rx_channel, 1, true);
          feedback_count ++;

          logger->debug("Sent feedback at iteration {}", loop_count);
          loop_count ++;

          if(loop_count > max_loops) {
            logger->warn("Finish unlocking thread after {} loop", max_loops);
            thread_failure = true;
            return;
          }
        }

        logger->warn("Somehow finished loop in thread {}", loop_count);
        thread_failure = true;
      });

    // Skip pre/post scan
    scan.run();

    bookie.rawData.finish();

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

    auto fe = StdDict::getFrontEnd("FEI4B").release();
    fe->setActive(true);
    fe->init(&empty, 0, 0);
    bookie.addFe(fe, 0, rx_channel);

    auto g_fe = StdDict::getFrontEnd("FEI4B");
    g_fe->makeGlobal();
    g_fe->init(&empty, 0, 0);
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

    PixelFeedbackSender send(&fb[rx_channel]);

    uint32_t feedback_count = 0;

    const int max_loops = 10;

    bool thread_failure = true;

    std::thread t([&]() {
        int loop_count = 0;
        while(1) {
          bookie.rawData.waitNotEmptyOrDone();
          auto data = bookie.rawData.popData();
          if(!data) {
            // Return due to finish call
            thread_failure = false;
            return;
          }

          logger->debug("Received data");

          const LoopStatus &stat = data->stat;

          REQUIRE (stat.size() == 3);
          logger->trace("Current loop status: {} {} {}",
                        stat.get(0), stat.get(1), stat.get(2));

          // As there's no inner loop, send feedback as soon as data arrives
          auto h = std::make_unique<Histo2d>("Test", 80, 0, 20, 336, 0, 20);
          send.feedback(rx_channel, std::move(h));
          feedback_count ++;

          logger->debug("Sent feedback at iteration {}", loop_count);
          loop_count ++;

          if(loop_count > max_loops) {
            logger->warn("Finish unlocking thread after {} loop", max_loops);
            thread_failure = true;
            return;
          }
        }

        logger->warn("Somehow finished loop in thread {}", loop_count);
        thread_failure = true;
      });

    // Skip pre/post scan
    scan.run();

    bookie.rawData.finish();

    t.join();

    // 4,2,1,1
    REQUIRE (feedback_count == 4);

    REQUIRE (!thread_failure);
}
