add_library(StarEmu SHARED
        StarChipsetEmu.cpp
        StarEmu.cpp
        StarEmuTxCore.cpp
)

post_build_debug_library(StarEmu)
target_include_directories(StarEmu PRIVATE ../libYarr/include)
target_include_directories(StarEmu PRIVATE ../libUtil/include)
target_include_directories(StarEmu PRIVATE ../libEmu/include)
target_include_directories(StarEmu PUBLIC ./include)
target_link_libraries(StarEmu Star Emu)

if (BUILD_TESTS)
    add_subdirectory(tests)
endif ()
