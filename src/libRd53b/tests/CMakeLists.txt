add_executable(testRd53b
        test_data_processor.cpp
        test_data_processor_edgecases.cpp
        test_data_processor_bugs.cpp
)
target_link_libraries(testRd53b PUBLIC Yarr)
target_link_libraries(testRd53b PRIVATE mainTest)

add_test(NAME testRd53b COMMAND testRd53b)
post_build_debug_executable(testRd53b)

install(TARGETS testRd53b
    EXPORT YarrTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}  # For DLLs on Windows
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )
