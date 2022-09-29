add_library(Rd53aEmu SHARED
        Rd53aEmu.cpp
        Rd53aEmuController.cpp
        Rd53aEmuTxCore.cpp
)

post_build_debug_library(Rd53aEmu)
target_include_directories(Rd53aEmu PRIVATE ../libYarr/include)
target_include_directories(Rd53aEmu PRIVATE ../libUtil/include)
target_include_directories(Rd53aEmu PRIVATE ../libEmu/include)
target_include_directories(Rd53aEmu PUBLIC ./include)
target_link_libraries(Rd53aEmu Rd53a Emu)
