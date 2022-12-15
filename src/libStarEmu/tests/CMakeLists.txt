add_executable(testStarEmu
        ../../libUtil/tests/main.cpp
        test_emulator.cpp
        test_broadcast.cpp
        )
target_link_libraries(testStarEmu Yarr)
add_test(testStarEmu ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/testStarEmu)
post_build_debug_executable(testStarEmu)
