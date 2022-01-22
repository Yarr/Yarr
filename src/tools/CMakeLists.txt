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

foreach (target IN LISTS tools)
    add_executable(${target} ${target}.cpp)
    target_link_libraries(${target} Yarr)
endforeach ()

# write-register needs special link libraries
add_executable(write-register write-register.cpp)
target_link_libraries(write-register Yarr)
if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
    target_link_libraries(write-register stdc++fs)
endif ()