add_library(Fei4 SHARED
        AllFei4Actions.cpp
        Fei4AnalogScan.cpp
        Fei4Cfg.cpp
        Fei4Cmd.cpp
        Fei4.cpp
        Fei4DataProcessor.cpp
        Fei4DcLoop.cpp
        Fei4DigitalScan.cpp
        Fei4GlobalCfg.cpp
        Fei4GlobalFeedback.cpp
        Fei4GlobalPreampRetune.cpp
        Fei4GlobalPreampTune.cpp
        Fei4GlobalThresholdTune.cpp
        Fei4MaskLoop.cpp
        Fei4NoiseScan.cpp
        Fei4ParameterLoop.cpp
        Fei4PixelCfg.cpp
        Fei4PixelPreampTune.cpp
        Fei4PixelThresholdTune.cpp
        Fei4Selftrigger.cpp
        Fei4ThresholdScan.cpp
        Fei4TotScan.cpp
        Fei4TriggerLoop.cpp)

add_dependencies(Fei4 variant)
post_build_debug_library(Fei4)
target_include_directories(Fei4 PRIVATE ../libYarr/include)
target_include_directories(Fei4 PRIVATE ../libUtil/include)
target_include_directories(Fei4 PUBLIC ./include)
target_link_libraries(Fei4 Scan)

LIST(APPEND apps fei4ReadRegister)
foreach (target IN LISTS apps)
    add_executable(${target} app/${target}.cpp)
    target_link_libraries(${target} Yarr)
    post_build_debug_executable(${target})
endforeach ()

if (BUILD_TESTS)
    add_subdirectory(tests)
endif ()