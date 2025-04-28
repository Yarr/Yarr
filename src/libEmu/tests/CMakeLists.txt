add_executable(testEmu
        test_queues.cpp
)
target_link_libraries(testEmu PUBLIC Yarr)
target_link_libraries(testEmu PRIVATE mainTest)
add_test(testEmu ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/testEmu)
post_build_debug_executable(testEmu)
