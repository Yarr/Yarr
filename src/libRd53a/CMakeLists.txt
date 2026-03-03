add_library(Rd53a SHARED
        AllRd53aActions.cpp
        Rd53a2TriggerLoop.cpp
        Rd53aCfg.cpp
        Rd53aCmd.cpp
        Rd53aCoreColLoop.cpp
        Rd53a.cpp
        Rd53aDataProcessor.cpp
        Rd53aGlobalCfg.cpp
        Rd53aGlobalFeedback.cpp
        Rd53aMaskLoop.cpp
        Rd53aParameterLoop.cpp
        Rd53aPixelCfg.cpp
        Rd53aPixelFeedback.cpp
        Rd53aReadRegLoop.cpp
        Rd53aTriggerLoop.cpp
        Rd53aTwoParameterLoop.cpp)

post_build_debug_library(Rd53a)
target_include_directories(Rd53a PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(Rd53a PUBLIC Scan Yarr_interface Util_interface)

# Apps (Spec only)
# rd53aReadRegister
# rd53aReadRingosc

install(TARGETS Rd53a
    EXPORT Yarr_Rd53aTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}  # For DLLs on Windows
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )

install(EXPORT Yarr_Rd53aTargets
  NAMESPACE Yarr::
  DESTINATION ${CMAKE_CONFIG_INSTALL_PATH}/Yarr
)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING
        PATTERN "*.h"
)

