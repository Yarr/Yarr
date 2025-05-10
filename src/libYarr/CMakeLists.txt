add_library(Yarr SHARED
        yarr.cpp
        )

target_include_directories(Yarr PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    # for the yarr_version.h which is created in the binary tree
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/src/libYarr/include> 
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(Yarr ${CONTROLLERS})
target_link_libraries(Yarr ${FRONT_ENDS})
target_link_libraries(Yarr Util_interface)
target_link_options(Yarr PUBLIC -Wl,--no-as-needed)

post_build_debug_library(Yarr)

install(TARGETS Yarr
    EXPORT YarrTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}  # For DLLs on Windows
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING
        PATTERN "*.h"
)

# add interface only target to resolve cyclical dependency in a cleaner way (until CMAKE 3.27 which offers a different mechanism)
add_library(Yarr_interface INTERFACE)

target_include_directories(Yarr_interface INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(Yarr_interface INTERFACE Util_interface)

install(TARGETS Yarr_interface
    EXPORT YarrTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}  # For DLLs on Windows
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

add_library(Scan SHARED
        AllAnalyses.cpp
        AllChips.cpp
        AllConfigurations.cpp
        AllHistogrammers.cpp
        AllHwControllers.cpp
        AllProcessors.cpp
        AllStdActions.cpp
        AnalysisAlgorithm.cpp
        AnalysisConsole.cpp
        Bookkeeper.cpp
        Clipboard.cpp
        ClusterData.cpp
        DataProcessor.cpp
        EventData.cpp
        FeedbackBase.cpp
        FileConfiguration.cpp
        FrontEnd.cpp
        HistogramAlgorithm.cpp
        HistogramBase.cpp
	JsonData.cpp
        LoopActionBase.cpp
        LoopEngine.cpp
        ResultBase.cpp
        ScanBase.cpp
        ScanConsole.cpp
        ScanConsoleImpl.cpp
        ScanFactory.cpp
        ScanHelper.cpp
        StdAnalysis.cpp
        StdDataGatherer.cpp
        StdDataLoop.cpp
        StdHistogrammer.cpp
        StdParameterLoop.cpp
        StdRepeater.cpp
        YarrDiagram.cpp)
target_include_directories(Scan PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(Scan Util)

post_build_debug_library(Scan)

if (BUILD_TESTS)
    add_subdirectory(tests)
endif ()

install(TARGETS Scan
    EXPORT YarrTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING
        PATTERN "*.h"
)

