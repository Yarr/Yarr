add_library(Rd53aEmu SHARED
        Rd53aEmu.cpp
        Rd53aEmuController.cpp
        Rd53aEmuTxCore.cpp
)

post_build_debug_library(Rd53aEmu)
target_include_directories(Rd53aEmu PRIVATE ../libYarr/include)
target_include_directories(Rd53aEmu PRIVATE ../libUtil/include)
target_include_directories(Rd53aEmu PRIVATE ../libEmu/include)
target_include_directories(Rd53aEmu PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(Rd53aEmu Rd53a Emu)

install(TARGETS Rd53aEmu
    EXPORT Yarr_Rd53aEmuTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}  # For DLLs on Windows
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )

install(EXPORT Yarr_Rd53aEmuTargets
  NAMESPACE Yarr::
  DESTINATION cmake
)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING 
        PATTERN "*.h"
        PATTERN "*.hpp"
)
