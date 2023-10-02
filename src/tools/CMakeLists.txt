list(APPEND tools
        analyseRawData
        createConfig
        dbAccessor
        makeFei4PixelModelCfg
        makeRd53aPixelModelCfg
        replot
        scan_cli
        scurve_test
        test_json
        dumpRawDataToText
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
