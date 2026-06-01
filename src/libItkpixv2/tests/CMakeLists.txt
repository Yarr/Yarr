add_executable(testItkpixv2
        test_data_processor.cpp
        test_data_processor_edgecases.cpp
        test_data_processor_errors.cpp
        test_data_processor_bugs.cpp
)
target_link_libraries(testItkpixv2 PUBLIC Yarr)
target_link_libraries(testItkpixv2 PRIVATE mainTest)

add_test(NAME testItkpixv2 COMMAND testItkpixv2)
post_build_debug_executable(testItkpixv2)

install(TARGETS testItkpixv2
    EXPORT YarrTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}  # For DLLs on Windows
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )
