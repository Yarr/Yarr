add_executable(testFei4
test_feedback.cpp
        )
target_link_libraries(testFei4 Yarr)
add_test(testFei4 ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/testFei4)
