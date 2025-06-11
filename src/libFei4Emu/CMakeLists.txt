add_library(Fei4Emu SHARED
        Fei4Emu.cpp
        Fei4EmuController.cpp
        Fei4EmuTxCore.cpp
)

post_build_debug_library(Fei4Emu)
target_include_directories(Fei4Emu PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(Fei4Emu Fei4 Emu Yarr_interface Util_interface)

install(TARGETS Fei4Emu
    EXPORT Yarr_Fei4EmuTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}  # For DLLs on Windows
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )

install(EXPORT Yarr_Fei4EmuTargets
  NAMESPACE Yarr::
  DESTINATION ${CMAKE_CONFIG_INSTALL_PATH}/Yarr
)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING
        PATTERN "*.h"
)
