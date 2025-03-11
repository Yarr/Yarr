add_executable(testFelix
  ../../libUtil/tests/main.cpp
  test_elinkcontrol.cpp
)

target_link_libraries(testFelix Yarr)

add_test(testFelix ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/testFelix)

post_build_debug_executable(testFelix)