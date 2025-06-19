add_library(ItsdaqFW SHARED
        Checksum.cpp
        ItsdaqFW.cpp
        ItsdaqHandler.cpp
        ItsdaqRxCore.cpp
        ItsdaqTxCore.cpp
        UdpSocket.cpp
        )
post_build_debug_library(ItsdaqFW)
target_include_directories(ItsdaqFW PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(ItsdaqFW Yarr_interface Util_interface)

install(TARGETS ItsdaqFW
    EXPORT Yarr_ItsdaqFWTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}  # For DLLs on Windows
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )

install(EXPORT Yarr_ItsdaqFWTargets
  NAMESPACE Yarr::
  DESTINATION ${CMAKE_CONFIG_INSTALL_PATH}/Yarr
)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING
        PATTERN "*.h"
)
