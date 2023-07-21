add_executable(testRd53b
        ../../libUtil/tests/main.cpp
)
target_link_libraries(testRd53b Yarr)
add_test(testRd53b ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/testRd53b)
post_build_debug_executable(testRd53b)
