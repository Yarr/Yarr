add_executable(testRd53b
        test_data_processor.cpp
        test_data_processor_edgecases.cpp
)
target_link_libraries(testRd53b PUBLIC Yarr)
target_link_libraries(testRd53b PRIVATE mainTest)
add_test(testRd53b ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/testRd53b)
post_build_debug_executable(testRd53b)
