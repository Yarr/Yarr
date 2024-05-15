add_library(Itkpixv2Emu SHARED
        Itkpixv2Emu.cpp
        Itkpixv2EmuController.cpp
#        Itkpixv2EmuTxCore.cpp
)

post_build_debug_library(Itkpixv2Emu)
target_include_directories(Itkpixv2Emu PRIVATE ../libYarr/include)
target_include_directories(Itkpixv2Emu PRIVATE ../libUtil/include)
target_include_directories(Itkpixv2Emu PRIVATE ../libEmu/include)
target_include_directories(Itkpixv2Emu PUBLIC ./include)
target_link_libraries(Itkpixv2Emu Itkpixv2 Emu)
