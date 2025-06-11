add_library(Itkpixv2Emu SHARED
        Itkpixv2Emu.cpp
        Itkpixv2EmuController.cpp
        Itkpixv2EmuTxCore.cpp
        Itkpixv2EmuCommandInterpreter.cpp
        Itkpixv2EmuCommandExe.cpp
)

post_build_debug_library(Itkpixv2Emu)
target_include_directories(Itkpixv2Emu PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(Itkpixv2Emu Itkpixv2 Emu Yarr_interface Util_interface)

install(TARGETS Itkpixv2Emu
    EXPORT Yarr_Itkpixv2EmuTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}  # For DLLs on Windows
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )
  
install(EXPORT Yarr_Itkpixv2EmuTargets
  NAMESPACE Yarr::
  DESTINATION ${CMAKE_CONFIG_INSTALL_PATH}/Yarr
)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING
        PATTERN "*.h"
)
