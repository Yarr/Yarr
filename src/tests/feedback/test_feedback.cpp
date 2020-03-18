#include "catch.hpp"

#include "AllStdActions.h"
#include "ScanFactory.h"

// Just create scan engine side
TEST_CASE("FeedbackTests", "[Feedback]") {
    Bookkeeper bookie(nullptr, nullptr);
    ScanFactory scan(&bookie);

    json js;
    js["scan"]["name"] = "TestScan";
    js["scan"]["loops"][0]["loopAction"] = "Fei4GlobalFeedback";
    js["scan"]["loops"][1]["loopAction"] = "Fei4MaskLoop";
    js["scan"]["loops"][2]["loopAction"] = "Fei4DcLoop";
    js["scan"]["loops"][3]["loopAction"] = "Fei4TriggerLoop";
    js["scan"]["loops"][4]["loopAction"] = "StdDataLoop";

    scan.loadConfig(js);

    scan.init();

    // s->preScan();
    scan.run();
    // s->postScan();
}
