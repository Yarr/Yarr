add_executable(testFelix
        test_elinkcontrol.cpp
)

target_link_libraries(testFelix PUBLIC Yarr)
target_link_libraries(testFelix PRIVATE mainTest)

add_test(NAME testFelix COMMAND testFelix)
post_build_debug_executable(testFelix)

install(TARGETS testFelix
    EXPORT YarrTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}  # For DLLs on Windows
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )
