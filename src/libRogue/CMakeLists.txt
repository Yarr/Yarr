YARR_ADD_ROGUE()
add_library(Rogue SHARED
        RogueCom.cpp
        RogueController.cpp
        RogueRxCore.cpp
        RogueTxCore.cpp
        )


add_dependencies(Rogue tbb_2020 rogue variant)
target_include_directories(Rogue PRIVATE ../libYarr/include)
target_include_directories(Rogue PRIVATE ../libUtil/include)
target_include_directories(Rogue PUBLIC ./include)
target_compile_definitions(Rogue PUBLIC NO_PYTHON=1)
target_link_libraries(Rogue PRIVATE rogue-core tbb)
