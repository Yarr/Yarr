add_executable(testItkpixv2        
        ../../libUtil/tests/main.cpp
        test_Itkpixv2_data_processor.cpp
)
target_link_libraries(testItkpixv2 Yarr)
add_test(testItkpixv2 ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/testItkpixv2)
post_build_debug_executable(testItkpixv2)
