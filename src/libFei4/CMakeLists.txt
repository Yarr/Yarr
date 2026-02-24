add_library(Fei4 SHARED
        AllFei4Actions.cpp
        Fei4Cfg.cpp
        Fei4Cmd.cpp
        Fei4.cpp
        Fei4DataProcessor.cpp
        Fei4DcLoop.cpp
        Fei4GlobalCfg.cpp
        Fei4GlobalFeedback.cpp
        Fei4MaskLoop.cpp
        Fei4ParameterLoop.cpp
        Fei4PixelCfg.cpp
        Fei4TriggerLoop.cpp)

post_build_debug_library(Fei4)
target_include_directories(Fei4 PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(Fei4 Scan Yarr_interface Util_interface)

LIST(APPEND apps fei4ReadRegister)
foreach (target IN LISTS apps)
    add_executable(${target} app/${target}.cpp)
    target_link_libraries(${target} Yarr)
    post_build_debug_executable(${target})
endforeach ()

if (BUILD_TESTS)
    add_subdirectory(tests)
endif ()

install(TARGETS Fei4 ${apps}
    EXPORT Yarr_Fei4Targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}  # For DLLs on Windows
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )

install(EXPORT Yarr_Fei4Targets
  NAMESPACE Yarr::
  DESTINATION ${CMAKE_CONFIG_INSTALL_PATH}/Yarr
)
install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING
        PATTERN "*.h"
)
