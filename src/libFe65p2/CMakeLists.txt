add_library(Fe65p2 SHARED
        Fe65p2AnalogScan.cpp
        Fe65p2Cfg.cpp
        Fe65p2Cmd.cpp
        Fe65p2.cpp
        Fe65p2DataProcessor.cpp
        Fe65p2DigitalScan.cpp
        Fe65p2ExtTrigger.cpp
        Fe65p2GlobalCfg.cpp
        Fe65p2GlobalFeedback.cpp
        Fe65p2GlobalThresholdTune.cpp
        Fe65p2MaskLoop.cpp
        Fe65p2NoiseScan.cpp
        Fe65p2ParameterLoop.cpp
        Fe65p2PixelCfg.cpp
        Fe65p2PixelFeedback.cpp
        Fe65p2PixelThresholdTune.cpp
        Fe65p2QcLoop.cpp
        Fe65p2ThresholdScan.cpp
        Fe65p2TotScan.cpp
        Fe65p2TriggerLoop.cpp)

post_build_debug_library(Fe65p2)
target_include_directories(Fe65p2 PRIVATE ../libYarr/include)
target_include_directories(Fe65p2 PRIVATE ../libUtil/include)
target_include_directories(Fe65p2 PUBLIC ./include)
target_link_libraries(Fe65p2 Scan)

LIST(APPEND apps fe65p2ApplyMask fe65p2PulserDelayTest)
foreach (target IN LISTS apps)
    add_executable(${target} app/${target}.cpp)
    target_link_libraries(${target} Yarr)
    post_build_debug_executable(${target})
endforeach ()
