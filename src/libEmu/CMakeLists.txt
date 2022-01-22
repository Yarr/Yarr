add_library(Emu SHARED
        EmuCom.cpp
        EmuController.cpp
        EmuRxCore.cpp
        EmuShm.cpp
        EmuTxCore.cpp
        Fei4Emu.cpp
        PixelModel.cpp
        Rd53aDiffPixelModel.cpp
        Rd53aEmu.cpp
        Rd53aLinPixelModel.cpp
        Rd53aSyncPixelModel.cpp
        RingBuffer.cpp
        StarChipsetEmu.cpp
        StarEmu.cpp
        StripModel.cpp)

add_dependencies(Emu variant)
target_include_directories(Emu PRIVATE ../libYarr/include)
target_include_directories(Emu PRIVATE ../libUtil/include)
target_include_directories(Emu PUBLIC ./include)
target_link_libraries(Emu Scan Rd53a Fei4 Star)
