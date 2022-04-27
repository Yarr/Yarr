list(APPEND tools
        analyseRawData
        createConfig
        dbAccessor
        makeFei4PixelModelCfg
        makeRd53aPixelModelCfg
        replot
        scan_cli
        scanConsole
        scurve_test
        test_json
        )

if (NOT ${TARGET_ARCH} MATCHES "arm*")
    list(APPEND tools write-register)
endif()
 
foreach (target IN LISTS tools)
    add_executable(${target} ${target}.cpp)
    post_build_debug_executable(${target})
    target_link_libraries(${target} Yarr)
    target_link_libraries(${target} Scan)
endforeach ()
