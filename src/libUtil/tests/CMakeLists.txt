add_executable(testUtil
        main.cpp
        test_misc.cpp
        test_histo1d.cpp
)

# Need to be able to include HistogramBase
target_include_directories(testUtil PRIVATE ../../libYarr/include)

target_link_libraries(testUtil Util)
add_test(testUtil ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/testUtil)
post_build_debug_executable(testUtil)
