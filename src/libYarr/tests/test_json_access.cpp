#include "catch.hpp"

#include <iostream>
#include <filesystem>

#include "LoadJson.h"

TEST_CASE("CheckPath", "[Json][JsonHelper]") {
    // Lookup below are done via YARR_CONFIG_PATH, which should find in src dir

    auto cfg_path = getenv("YARR_CONFIG_PATH");
    REQUIRE (cfg_path);
    REQUIRE (std::filesystem::exists(cfg_path));
}

//! Test a few json operations (related to file opening, including fragments)
TEST_CASE("OpenJsonFiles", "[Json][JsonHelper]") {
    // Should throw if the file does not exist
    CHECK_THROWS ( JsonHelper::openJsonFile("configs/scans/not_a_scan.json") );

    auto nm = JsonHelper::openJsonFile("configs/scans/star/std_nmask.json");

    CHECK (nm.contains("scan"));

    // Test if the json file can be opened with a fragment (json pointer)
    auto nm_p = JsonHelper::openJsonFile("configs/scans/star/std_nmask.json#/scan");

    CHECK (nm_p.contains("analysis"));

    // Check it throws if the fragment is not valid
    CHECK_THROWS (
      JsonHelper::openJsonFile("configs/scans/star/std_nmask.json#/not_a_scan")
    );

    auto fe = JsonHelper::openJsonFile("configs/defaults/default_rd53b.json");

    CHECK (fe.contains("RD53B"));

    // Access an array via json pointer
    auto fe_p = JsonHelper::openJsonFile("configs/defaults/default_rd53b.json#/RD53B/PixelConfig/0/Enable");

    CHECK (fe_p.size() == 384);
}
