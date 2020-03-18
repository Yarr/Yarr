#include "catch.hpp"

#include "AllChips.h"
#include "AllStdActions.h"
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
