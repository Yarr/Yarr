#include "catch.hpp"

#include "RawData.h"

TEST_CASE("SimpleRawData", "[raw_data]") {
    // Run through heap-allocated RawData
    RawData pre_size(4, 100);
    CHECK(pre_size.getAdr() == 4);
    CHECK(pre_size.getSize() == 100);
    for(int i=0; i<100; i++) {
        CHECK(pre_size[i] == 0);
    }

    std::vector<uint32_t> move_vector(100);
    auto move_data_ptr = move_vector.data();
    RawData raw_move(4, std::move(move_vector));
    CHECK(raw_move.getAdr() == 4);
    CHECK(raw_move.getSize() == 100);
    for(int i=0; i<100; i++) {
        CHECK(raw_move[i] == 0);
    }
    // Should still be pointing to the data in the original vector
    CHECK(raw_move.getBuf() == move_data_ptr);

    std::vector<uint32_t> copy_vector(100);
    auto copy_data_ptr = copy_vector.data();
    RawData raw_copy(4, copy_vector);
    copy_vector[10] = 123; // Make sure the copy can't be optimised away
    CHECK(raw_copy.getAdr() == 4);
    CHECK(raw_copy.getSize() == 100);
    for(int i=0; i<100; i++) {
        CHECK(raw_copy[i] == 0);
    }
    // Should be pointing to a copy of the data, not the original vector
    CHECK(raw_copy.getBuf() != copy_data_ptr);
}
