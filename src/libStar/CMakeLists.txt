add_library(Star SHARED
        AbcCfg.cpp
        AllStarActions.cpp
        HccCfg.cpp
        StarCfg.cpp
        StarChannelFeedback.cpp
        StarChips.cpp
        StarCmd.cpp
        StarCounterLoop.cpp
        StarDataProcessor.cpp
	StarJsonData.cpp
        StarMaskLoop.cpp
        StarRegDump.cpp
        StarTriggerLoop.cpp)

add_dependencies(Star variant)
target_include_directories(Star PRIVATE ../libYarr/include)
target_include_directories(Star PUBLIC ./include)
target_link_libraries(Star Scan)

LIST(APPEND apps test_star)
foreach (target IN LISTS apps)
    add_executable(${target} app/${target}.cpp)
    target_link_libraries(${target} Yarr)
endforeach ()


if (BUILD_TESTS)
    add_subdirectory(tests)
endif ()
