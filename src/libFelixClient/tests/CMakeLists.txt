add_executable(testFelix
        test_elinkcontrol.cpp
)

target_link_libraries(testFelix PUBLIC Yarr)
target_link_libraries(testFelix PRIVATE mainTest)

add_test(testFelix ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/testFelix)

post_build_debug_executable(testFelix)
