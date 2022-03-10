list(APPEND tools
        analyseRawData
        config-GBT-FMC
        createConfig
        dbAccessor
        makeFei4PixelModelCfg
        makeRd53aPixelModelCfg
        replot
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
endforeach ()
