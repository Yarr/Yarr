add_library(Star SHARED
        AbcCfg.cpp
        AbcNames.cpp
        AllStarActions.cpp
        HccCfg.cpp
        HccNames.cpp
        StarCfg.cpp
        StarChannelFeedback.cpp
        StarChips.cpp
        StarChipsBroadcast.cpp
        StarCmd.cpp
        StarConversionTools.cpp
        StarCounterLoop.cpp
        StarDataProcessor.cpp
        StarFelixTriggerLoop.cpp
        StarJsonData.cpp
        StarMaskLoop.cpp
        StarNPointGainAnalysis.cpp
        StarParamFeedback.cpp
        StarRegDump.cpp
        StarSeqGenerator.cpp
        StarStrobeDelayAnalysis.cpp
        StarTriggerLoop.cpp
        StarTriggerThrottleAnalysis.cpp
        StarTrimDacAnalysis.cpp
)

post_build_debug_library(Star)
target_include_directories(Star PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(Star PUBLIC Scan)
target_link_libraries(Star PRIVATE Yarr_interface)

LIST(APPEND apps test_star makeLCBSequence benchmark_data_processor_star printStarSubRegisters)
foreach (target IN LISTS apps)
    add_executable(${target} app/${target}.cpp)
    target_link_libraries(${target} Yarr)
    post_build_debug_executable(${target})
endforeach ()


if (BUILD_TESTS)
    add_subdirectory(tests)
endif ()

install(TARGETS Star ${apps}
    EXPORT Yarr_StarTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}  # For DLLs on Windows
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )

install(EXPORT Yarr_StarTargets
  NAMESPACE Yarr::
  DESTINATION ${CMAKE_CONFIG_INSTALL_PATH}/Yarr
)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING
        PATTERN "*.h"
)
