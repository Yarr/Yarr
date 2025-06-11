add_library(Emu SHARED
        PixelModel.cpp
        Rd53aDiffPixelModel.cpp
        Rd53aLinPixelModel.cpp
        Rd53aSyncPixelModel.cpp
        StripModel.cpp)

post_build_debug_library(Emu)
target_include_directories(Emu PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(Emu Scan Yarr_interface Util_interface)

if (BUILD_TESTS)
    add_subdirectory(tests)
endif ()

install(TARGETS Emu
    EXPORT Yarr_EmuTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}  # For DLLs on Windows
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )
  
install(EXPORT Yarr_EmuTargets
  NAMESPACE Yarr::
  DESTINATION ${CMAKE_CONFIG_INSTALL_PATH}/Yarr
)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING 
        PATTERN "*.h"
)
