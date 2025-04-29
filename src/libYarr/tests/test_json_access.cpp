#include "catch.hpp"

#include <filesystem>

#include "ScanHelper.h"

//! Test a few json operations (related to file opening, including fragments)
TEST_CASE("OpenJsonFiles", "[Json][ScanHelper]") {
    // Look up in dir hierarchy so we can run from anywhere in YARR directory
    namespace fs = std::filesystem;
    fs::path baseDir{fs::current_path()};
    while(!fs::exists(baseDir/".git")) {
        if(!baseDir.has_parent_path()) {
            // Can't find file
            std::cout << "Can't find configs, no parent of " << baseDir << "\n";
            return;
        }
        if(baseDir == baseDir.parent_path()) {
            // Can't find file
            std::cout << "Can't find configs from " << baseDir << "\n";
            return;
        }
        baseDir = baseDir.parent_path();
    }

    auto cfg = baseDir/"configs";

    REQUIRE (fs::exists(cfg));

    // Should throw if the file does not exist
    CHECK_THROWS ( ScanHelper::openJsonFile(cfg/"scans/not_a_scan.json") );

    auto nm = ScanHelper::openJsonFile(cfg/"scans/star/std_nmask.json");

    CHECK (nm.contains("scan"));

    // Test if the json file can be opened with a fragment (json pointer)
    auto nm_p = ScanHelper::openJsonFile(cfg/"scans/star/std_nmask.json#/scan");

    CHECK (nm_p.contains("analysis"));

    // Check it throws if the fragment is not valid
    CHECK_THROWS (
      ScanHelper::openJsonFile(cfg/"scans/star/std_nmask.json#/not_a_scan")
    );

    auto fe = ScanHelper::openJsonFile(cfg/"defaults/default_rd53b.json");

    CHECK (fe.contains("RD53B"));

    // Access an array via json pointer
    auto fe_p = ScanHelper::openJsonFile(cfg/"defaults/default_rd53b.json#/RD53B/PixelConfig/0/Enable");

    CHECK (fe_p.size() == 384);
}
