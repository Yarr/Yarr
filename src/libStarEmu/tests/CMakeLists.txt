add_executable(testStarEmu
        test_emulator.cpp
        test_broadcast.cpp
        test_packets.cpp
        )
target_link_libraries(testStarEmu PUBLIC Yarr)
target_link_libraries(testStarEmu PRIVATE mainTest)
add_test(testStarEmu ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/testStarEmu)
post_build_debug_executable(testStarEmu)
