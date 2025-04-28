add_library(mainTest OBJECT
        main.cpp
)

target_link_libraries(mainTest Util)

add_executable(testUtil
        test_misc.cpp
        test_histo1d.cpp
        test_histo2d.cpp
        test_histo3d.cpp
)


target_link_libraries(testUtil PUBLIC Util)
# Need to be able to include HistogramBase
target_link_libraries(testUtil PRIVATE mainTest Yarr_interface)

add_test(testUtil ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/testUtil)
post_build_debug_executable(testUtil)
