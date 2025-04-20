list(APPEND tools
        analyseRawData
        createConfig
        dbAccessor
        hw_read_test
        hw_write_test
        makeFei4PixelModelCfg
        makeRd53aPixelModelCfg
        replot
        scan_cli
        analysis_test
        scurve_test
        test_json
        dumpRawDataToText
        checkTriggerTags
        )

if (NOT "${TARGET_ARCH}" MATCHES "arm*")
    list(APPEND tools write-register read-register read-adc reset-all revert-chip-configs)
endif()

add_executable(scanConsole scan_cli.cpp)
post_build_debug_executable(scanConsole)
target_link_libraries(scanConsole Yarr)
target_link_libraries(scanConsole Scan)
 
foreach (target IN LISTS tools)
    add_executable(${target} ${target}.cpp)
    post_build_debug_executable(${target})
    target_link_libraries(${target} Yarr)
    target_link_libraries(${target} Scan)
endforeach ()

foreach (target hw_read_test hw_write_test)
    target_link_libraries(${target} Util)
endforeach()

install(TARGETS ${tools}
    EXPORT YarrTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}  # For DLLs on Windows
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )

