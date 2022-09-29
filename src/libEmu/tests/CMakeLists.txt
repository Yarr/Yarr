add_executable(testEmu
        ../../libUtil/tests/main.cpp
        test_queues.cpp
        )
target_link_libraries(testEmu Yarr)
add_test(testEmu ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/testEmu)
post_build_debug_executable(testEmu)
