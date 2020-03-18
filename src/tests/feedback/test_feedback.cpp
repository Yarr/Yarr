#include "catch.hpp"

#include <thread>

#include "AllChips.h"
#include "AllStdActions.h"
#include "FeedbackBase.h"
#include "ScanFactory.h"

#include "../EmptyHw.h"

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
    ScanFactory scan(&bookie);

    auto g_fe = StdDict::getFrontEnd("FEI4B");
    g_fe->makeGlobal();
    g_fe->init(&empty, 0, 0);
    bookie.initGlobalFe(g_fe.release());

    // Stripped down version of OccGlobalThresholdTune
    json js;
    js["scan"]["name"] = "TestScan";
    js["scan"]["loops"][0]["loopAction"] = "Fei4GlobalFeedback";
    js["scan"]["loops"][0]["config"]["parameter"] = "Vthin_Fine";
    js["scan"]["loops"][1]["loopAction"] = "Fei4MaskLoop";
    js["scan"]["loops"][2]["loopAction"] = "Fei4DcLoop";
    js["scan"]["loops"][3]["loopAction"] = "Fei4TriggerLoop";
    js["scan"]["loops"][4]["loopAction"] = "StdDataLoop";

    scan.loadConfig(js);

    scan.init();

    // Skip pre/post scan
    scan.run();
}

// Just create scan engine side
TEST_CASE("FeedbackTestGlobal", "[Feedback]") {
    MyHardware empty;
    Bookkeeper bookie(&empty, &empty);
    ScanFactory scan(&bookie);

    auto fe = StdDict::getFrontEnd("FEI4B").release();
    fe->setActive(true);
    fe->init(&empty, 0, 0);
    bookie.addFe(fe, 0, 0);

    auto g_fe = StdDict::getFrontEnd("FEI4B");
    g_fe->makeGlobal();
    g_fe->init(&empty, 0, 0);
    bookie.initGlobalFe(g_fe.release());

    // Stripped down version of OccGlobalThresholdTune
    json js;
    js["scan"]["name"] = "TestScan";
    js["scan"]["loops"][0]["loopAction"] = "Fei4GlobalFeedback";
    js["scan"]["loops"][0]["config"]["parameter"] = "Vthin_Fine";
    js["scan"]["loops"][1]["loopAction"] = "Fei4MaskLoop";
    js["scan"]["loops"][2]["loopAction"] = "Fei4DcLoop";
    js["scan"]["loops"][3]["loopAction"] = "Fei4TriggerLoop";
    js["scan"]["loops"][4]["loopAction"] = "StdDataLoop";

    int mask_loops = 40;
    int dc_loops = 16;

    scan.loadConfig(js);

    scan.init();

    GlobalFeedbackBase *fb = nullptr;

    for (unsigned n=0; n<scan.size(); n++) {
        std::shared_ptr<LoopActionBase> l = scan.getLoop(n);

        auto maybe = dynamic_cast<GlobalFeedbackBase *>(l.get());
        if(maybe != nullptr) {
          fb = maybe;
          break;
        }
    }

    REQUIRE (fb != nullptr);

    uint32_t unlock_count = 0;

    int max_loops = 10;

    std::thread t([&]() {
        int loop_count = 0;
        while(1) {
          bookie.rawData.waitNotEmptyOrDone();
          auto data = bookie.rawData.popData();
          if(!data) {
            // Return due to finish call
            return;
          }

          std::cout << "Received data from Clipboard\n";

          LoopStatus &stat = data->stat;
          for(int i=0; i<stat.size(); i++) {
            std::cout << " " << stat.get(i);
          }
          std::cout << "\n";

          if((stat.get(1) != dc_loops-1) ||
             (stat.get(2) != mask_loops-1)) {
            continue;
          }

          fb->feedbackBinary(0, 1, true);
         
          std::cout << "Unlocking feedback at iteration " << loop_count++ << "\n";

          // Reset before releasing lock
          // empty.readDataCount = 0;
          bookie.mutexMap[0].unlock();
unlock_count ++;
        }

        std::cout << "Finish unlocking thread after " << max_loops << " loop\n";
      });

    // Skip pre/post scan
    scan.run();

    bookie.rawData.finish();

    t.join();

    REQUIRE (unlock_count == 2);
}
