add_executable(testFei4
        test_feedback.cpp
)
target_link_libraries(testFei4 PUBLIC Yarr)
target_link_libraries(testFei4 PRIVATE mainTest)

add_test(NAME testFei4 COMMAND testFei4)
post_build_debug_executable(testFei4)

install(TARGETS testFei4
    EXPORT YarrTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}  # For DLLs on Windows
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )
