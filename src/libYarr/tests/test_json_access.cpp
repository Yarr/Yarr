#include "catch.hpp"

#include "ScanHelper.h"

//! Test a few json operations (related to file opening, including fragments)
TEST_CASE("OpenJsonFiles", "[Json][ScanHelper]") {
    // Should throw if the file does not exist
    CHECK_THROWS ( ScanHelper::openJsonFile("configs/scans/not_a_scan.json") );

    auto nm = ScanHelper::openJsonFile("configs/scans/star/std_nmask.json");

    CHECK (nm.contains("scan"));

    // Test if the json file can be opened with a fragment (json pointer)
    auto nm_p = ScanHelper::openJsonFile("configs/scans/star/std_nmask.json#/scan");

    CHECK (nm_p.contains("analysis"));

    // Check it throws if the fragment is not valid
    CHECK_THROWS (
      ScanHelper::openJsonFile("configs/scans/star/std_nmask.json#/not_a_scan")
    );

    auto fe = ScanHelper::openJsonFile("configs/defaults/default_rd53b.json");

    CHECK (fe.contains("RD53B"));

    // Access an array via json pointer
    auto fe_p = ScanHelper::openJsonFile("configs/defaults/default_rd53b.json#/RD53B/PixelConfig/0/Enable");

    CHECK (fe_p.size() == 384);
}
