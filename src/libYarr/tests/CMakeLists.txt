add_executable(testYarr
        ../../libUtil/tests/main.cpp
        test_front_end.cpp
        test_histogrammer.cpp
        test_hits_per_event.cpp
        test_l1dist.cpp
        test_l13d.cpp
        test_occupancy.cpp
        test_tot3d.cpp
        test_tot_dist.cpp
        test_analysis_chain.cpp
        test_analysis_poiloop.cpp
        test_poiloop.cpp
)

target_link_libraries(testYarr Yarr)
add_test(testYarr ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/testYarr)
post_build_debug_executable(testYarr)
