add_executable(testStar
        ../../libUtil/tests/main.cpp
        test_analysis_strobe_delay.cpp
        test_chips.cpp
        test_config.cpp
        test_data_processor.cpp
        test_histo_map.cpp
        test_json.cpp
        test_lcb.cpp
        test_mask_loop.cpp
        test_parser.cpp
        test_trigger_loop.cpp
        test_conversion_tool.cpp
        test_felix_trigger_loop.cpp
        )
target_link_libraries(testStar Yarr)
add_test(testStar ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/testStar)
post_build_debug_executable(testStar)
