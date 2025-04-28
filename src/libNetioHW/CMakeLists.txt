YARR_ADD_NETIO()
YARR_ADD_FELIX()
add_library(NetioHW SHARED
        Netio.cpp
        NetioHandler.cpp
        NetioRxCore.cpp
        NetioTxCore.cpp
        )

add_dependencies(NetioHW felixbase4 netio4)
post_build_debug_library(NetioHW)
target_include_directories(NetioHW PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(NetioHW PRIVATE netio tbb::tbb PUBLIC Yarr_interface Util_interface)

LIST(APPEND apps testNetioHw netio_bridge)
foreach (target IN LISTS apps)
    add_executable(${target} app/${target}.cpp)
    target_link_libraries(${target} Yarr NetioHW)
    post_build_debug_executable(${target})
endforeach ()

install(TARGETS NetioHW ${apps}
    EXPORT Yarr_NetioHWTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}  # For DLLs on Windows
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )

install(EXPORT Yarr_NetioHWTargets
  NAMESPACE Yarr::
  DESTINATION cmake
)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING
        PATTERN "*.h"
)
