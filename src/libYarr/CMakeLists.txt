add_library(Yarr SHARED
        yarr.cpp
        )

target_include_directories(Yarr PRIVATE ../libUtil/include)
target_include_directories(Yarr PUBLIC ./include)
target_link_libraries(Yarr ${CONTROLLERS})
target_link_libraries(Yarr ${FRONT_ENDS})
target_link_options(Yarr PUBLIC -Wl,--no-as-needed)

post_build_debug_library(Yarr)

add_library(Scan SHARED
        AllAnalyses.cpp
        AllChips.cpp
        AllHistogrammers.cpp
        AllHwControllers.cpp
        AllProcessors.cpp
        AllStdActions.cpp
        AnalysisAlgorithm.cpp
        AnalysisAlgorithm.cpp
        Bookkeeper.cpp
        Clipboard.cpp
        DataProcessor.cpp
        EventData.cpp
        FeedbackBase.cpp
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
target_include_directories(Scan PRIVATE ../libUtil/include)
target_include_directories(Scan PUBLIC ./include)
target_link_libraries(Scan Util pthread)

post_build_debug_library(Scan)

if (BUILD_TESTS)
    add_subdirectory(tests)
endif ()
