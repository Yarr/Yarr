add_library(StarEmu SHARED
        StarChipsetEmu.cpp
        StarEmu.cpp
        StarEmuTxCore.cpp
)

post_build_debug_library(StarEmu)
target_include_directories(StarEmu PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(StarEmu Star Emu Yarr_interface Util_interface)

if (BUILD_TESTS)
    add_subdirectory(tests)
endif ()

install(TARGETS StarEmu
    EXPORT Yarr_StarEmuTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}  # For DLLs on Windows
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )

install(EXPORT Yarr_StarEmuTargets
  NAMESPACE Yarr::
  DESTINATION cmake
)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING
        PATTERN "*.h"
)
