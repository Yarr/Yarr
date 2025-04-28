add_executable(testItkpixv2        
        test_data_processor.cpp
        test_data_processor_edgecases.cpp
        test_data_processor_errors.cpp
)
target_link_libraries(testItkpixv2 PUBLIC Yarr)
target_link_libraries(testItkpixv2 PRIVATE mainTest)
add_test(testItkpixv2 ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/testItkpixv2)
post_build_debug_executable(testItkpixv2)
