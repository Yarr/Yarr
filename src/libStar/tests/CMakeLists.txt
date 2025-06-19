add_executable(testStar
        test_analysis_strobe_delay.cpp
        test_analysis_trim.cpp
        test_analysis_scurvefitter.cpp
        test_analysis_npointgain.cpp
        test_analysis_throttle_trigger.cpp
        test_chips.cpp
        test_config.cpp
        test_data_processor.cpp
        test_histo_map.cpp
        test_json.cpp
        test_lcb.cpp
        test_mask_loop.cpp
        test_parameter_loop.cpp
        test_parser.cpp
        test_trigger_loop.cpp
        test_conversion_tool.cpp
        test_felix_trigger_loop.cpp
        )
target_link_libraries(testStar PUBLIC Yarr)
target_link_libraries(testStar PRIVATE mainTest)

add_test(NAME testStar COMMAND testStar)
post_build_debug_executable(testStar)

install(TARGETS testStar
    EXPORT YarrTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}  # For DLLs on Windows
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )
