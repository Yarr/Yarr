add_library(Fei4Emu SHARED
        Fei4Emu.cpp
        Fei4EmuController.cpp
        Fei4EmuTxCore.cpp
)

post_build_debug_library(Fei4Emu)
target_include_directories(Fei4Emu PRIVATE ../libYarr/include)
target_include_directories(Fei4Emu PRIVATE ../libUtil/include)
target_include_directories(Fei4Emu PRIVATE ../libEmu/include)
target_include_directories(Fei4Emu PUBLIC ./include)
target_link_libraries(Fei4Emu Fei4 Emu)
