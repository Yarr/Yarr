add_executable(testEmu
        test_queues.cpp
)
target_link_libraries(testEmu PUBLIC Yarr)
target_link_libraries(testEmu PRIVATE mainTest)

add_test(NAME testEmu COMMAND testEmu)
post_build_debug_executable(testEmu)

install(TARGETS testEmu
    EXPORT YarrTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}  # For DLLs on Windows
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )
