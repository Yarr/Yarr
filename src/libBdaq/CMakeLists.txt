find_package(Boost REQUIRED system)
add_library(Bdaq SHARED
        BdaqAuroraRx.cpp
        BdaqController.cpp
        Bdaq.cpp
        BdaqDriver.cpp
        BdaqGPIO.cpp
        Bdaq_i2c.cpp
        BdaqRBCP.cpp
        BdaqRegister.cpp
        BdaqRegisterProxy.cpp
        BdaqRxCore.cpp
        BdaqSi570.cpp
        BdaqSiTcpFifo.cpp
        BdaqTCP.cpp
        BdaqTxCore.cpp)

post_build_debug_library(Bdaq)
target_include_directories(Bdaq PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(Bdaq PUBLIC Yarr_interface Util_interface)
target_link_libraries(Bdaq PUBLIC Boost::system)

install(TARGETS Bdaq
    EXPORT Yarr_BdaqTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}  # For DLLs on Windows
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )
  
install(EXPORT Yarr_BdaqTargets
  NAMESPACE Yarr::
  DESTINATION ${CMAKE_CONFIG_INSTALL_PATH}
)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING 
        PATTERN "*.h"
)
