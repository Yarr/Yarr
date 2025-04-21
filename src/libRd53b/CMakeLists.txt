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
        Rd53bTriggerLoop.cpp
        Rd53bChipMap.cpp
        Rd53bEncodingTool.cpp
        Rd53bEncoder.cpp)

post_build_debug_library(Rd53b)
target_include_directories(Rd53b PRIVATE ../libYarr/include)
target_include_directories(Rd53b PRIVATE ../libUtil/include)
target_include_directories(Rd53b PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(Rd53b Scan)

LIST(APPEND apps
        rd53bConnectivityScan
        rd53bComTest
	rd53bColumnTest
        rd53bDecodeTool
        rd53bReadRingosc
        rd53bRegSEETest
        rd53bTest
        rd53bEfuseCheck
        rd53bSendGlobalPulse
        rd53bDataMergingCheck_4to1
        rd53bDataMergingCheck_2to1)

foreach (target IN LISTS apps)
    add_executable(${target} app/${target}.cpp)
    target_link_libraries(${target} Yarr)
    post_build_debug_executable(${target})
endforeach ()

if (BUILD_TESTS)
    add_subdirectory(tests)
endif ()

install(TARGETS Rd53b ${apps}
    EXPORT Yarr_Rd53bTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}  # For DLLs on Windows
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )
  
install(EXPORT Yarr_Rd53bTargets
  NAMESPACE Yarr::
  DESTINATION cmake
)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING 
        PATTERN "*.h"
        PATTERN "*.hpp"
)
