add_executable(testUtil
        main.cpp
        test_misc.cpp
)

target_link_libraries(testUtil Util)
add_test(testUtil ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/testUtil)
