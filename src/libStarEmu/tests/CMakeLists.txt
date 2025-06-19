add_executable(testStarEmu
        test_emulator.cpp
        test_broadcast.cpp
        test_packets.cpp
        )
target_link_libraries(testStarEmu PUBLIC Yarr)
target_link_libraries(testStarEmu PRIVATE mainTest)

add_test(NAME testStarEmu COMMAND testStarEmu)
post_build_debug_executable(testStarEmu)

install(TARGETS testStarEmu
    EXPORT YarrTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}  # For DLLs on Windows
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )
