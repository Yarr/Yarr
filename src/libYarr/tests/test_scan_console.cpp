#include "catch.hpp"

#include "logging.h"

#include "ScanConsole.h"

namespace {
auto logger = logging::make_log("test_scan_console");


ScanOpts emptyOptions() {
    ScanOpts options;

    options.scanType = "configs/sw_tests/sw_test_scan_empty.json";
    options.scan_config_provided = true;
    options.cConfigPaths = {"configs/sw_tests/sw_test_connectivity_single_empty.json"};
    options.ctrlCfgPath = "configs/sw_tests/sw_test_controller_empty.json";

    return options;
}

} // End anon namespace

// Run trivial scan to test scan console mechanism
TEST_CASE("SimpleScanConsole", "[scan_console]") {
    ScanConsole con;
    ScanOpts options = emptyOptions();

    int res=con.init(options);
    REQUIRE (res > 0);

    res=con.loadConfig();
    REQUIRE (res == 0);

    res=con.initHardware();
    REQUIRE (res == 0);

    res=con.configure();
    REQUIRE (res == 0);

    res=con.setupScan();
    REQUIRE (res == 0);

    con.run();
    con.cleanup();
    con.plot();
}
