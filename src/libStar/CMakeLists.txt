add_library(Star SHARED
        AbcCfg.cpp
        AllStarActions.cpp
        HccCfg.cpp
        StarCfg.cpp
        StarChannelFeedback.cpp
        StarChips.cpp
        StarChipsBroadcast.cpp
        StarCmd.cpp
        StarConversionTools.cpp
        StarCounterLoop.cpp
        StarDataProcessor.cpp
        StarJsonData.cpp
        StarMaskLoop.cpp
        StarRegDump.cpp
        StarStrobeDelayAnalysis.cpp
        StarFelixTriggerLoop.cpp
        StarTriggerLoop.cpp)

post_build_debug_library(Star)
target_include_directories(Star PRIVATE ../libYarr/include)
target_include_directories(Star PUBLIC ./include)
target_link_libraries(Star Scan)

LIST(APPEND apps test_star)
foreach (target IN LISTS apps)
    add_executable(${target} app/${target}.cpp)
    target_link_libraries(${target} Yarr)
    post_build_debug_executable(${target})
endforeach ()


if (BUILD_TESTS)
    add_subdirectory(tests)
endif ()
