add_executable(testYarr
        test_analysis_throttle.cpp
        test_event_data.cpp
        test_front_end.cpp
        test_histogrammer.cpp
        test_histogram_load.cpp
        test_hits_per_event.cpp
        test_json_access.cpp       
        test_l1dist.cpp
        test_l13d.cpp
        test_occupancy.cpp
        test_raw_data.cpp
        test_tot3d.cpp
        test_tot_dist.cpp
        test_analysis_chain.cpp
        test_analysis_poiloop.cpp
        test_poiloop.cpp
        test_npointgain.cpp
)

target_link_libraries(testYarr PUBLIC Scan Yarr)
target_link_libraries(testYarr PRIVATE mainTest)

add_test(NAME testYarr COMMAND testYarr)
post_build_debug_executable(testYarr)

install(TARGETS testYarr
    EXPORT YarrTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}  # For DLLs on Windows
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )
