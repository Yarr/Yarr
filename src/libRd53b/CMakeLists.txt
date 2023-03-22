add_library(Rd53b SHARED
        AllRd53bActions.cpp
        Rd53bAnalysis.cpp
        Rd53bCfg.cpp
        Rd53bCmd.cpp
        Rd53bCoreColLoop.cpp
        Rd53b.cpp
        Rd53bDataProcessor.cpp
        Rd53bGlobalCfg.cpp
        Rd53bGlobalFeedback.cpp
        Rd53bHistogrammer.cpp
        Rd53bMaskLoop.cpp
        Rd53bParMaskLoop.cpp
        Rd53bParameterLoop.cpp
        Rd53bPixelCfg.cpp
        Rd53bPixelFeedback.cpp
        Rd53bReadRegLoop.cpp
        Rd53bTriggerLoop.cpp)

post_build_debug_library(Rd53b)
target_include_directories(Rd53b PRIVATE ../libYarr/include)
target_include_directories(Rd53b PRIVATE ../libUtil/include)
target_include_directories(Rd53b PUBLIC ./include)
target_link_libraries(Rd53b Scan)

LIST(APPEND apps
        rd53bComTest
        rd53bDecodeTool
        rd53bReadRingosc
        rd53bRegSEETest
        rd53bTest
        rd53bEfuseCheck
        rd53bSendGlobalPulse)

foreach (target IN LISTS apps)
    add_executable(${target} app/${target}.cpp)
    target_link_libraries(${target} Yarr)
    post_build_debug_executable(${target})
endforeach ()

if (BUILD_TESTS)
    add_subdirectory(tests)
endif ()
