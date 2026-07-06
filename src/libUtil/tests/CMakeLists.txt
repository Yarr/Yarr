add_library(mainTest OBJECT
        main.cpp
        ../catch_amalgamated.cpp
)

target_link_libraries(mainTest Util)

target_compile_definitions(
    mainTest
  PRIVATE
    CATCH_AMALGAMATED_CUSTOM_MAIN=1
)

add_executable(testUtil
        test_misc.cpp
        test_histo1d.cpp
        test_histo2d.cpp
        test_histo3d.cpp
)


target_link_libraries(testUtil PUBLIC Util)
# Need to be able to include HistogramBase
target_link_libraries(testUtil PRIVATE mainTest Yarr_interface)

add_test(NAME testUtil COMMAND testUtil)
post_build_debug_executable(testUtil)

install(TARGETS testUtil
    EXPORT YarrTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}  # For DLLs on Windows
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )
