add_library(Itkpixv2 SHARED
        AllItkpixv2Actions.cpp
        Itkpixv2Cfg.cpp
        Itkpixv2Cmd.cpp
        Itkpixv2CoreColLoop.cpp
        Itkpixv2.cpp
        Itkpixv2DataProcessor.cpp
        Itkpixv2GlobalCfg.cpp
        Itkpixv2GlobalFeedback.cpp
        Itkpixv2MaskLoop.cpp
        Itkpixv2ParMaskLoop.cpp
        Itkpixv2ParameterLoop.cpp
        Itkpixv2PixelCfg.cpp
        Itkpixv2PixelFeedback.cpp
        Itkpixv2ReadRegLoop.cpp
        Itkpixv2TriggerLoop.cpp
        Itkpixv2Encoder.cpp)

post_build_debug_library(Itkpixv2)
target_include_directories(Itkpixv2 PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(Itkpixv2 Scan Yarr_interface Util_interface)

LIST(APPEND apps
        Itkpixv2CheckPulse
        Itkpixv2ReadRingosc
        Itkpixv2ColumnTest
        Itkpixv2EncodeRawData
        )

if(TARGET Spec)
  LIST(APPEND apps
    Itkpixv2ConnectivityScan
  )
endif()

foreach (target IN LISTS apps)
    add_executable(${target} app/${target}.cpp)
    target_link_libraries(${target} Yarr)
    post_build_debug_executable(${target})
endforeach ()

if (BUILD_TESTS)
    add_subdirectory(tests)
endif ()

install(TARGETS Itkpixv2 ${apps}
    EXPORT Yarr_Itkpixv2Targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}  # For DLLs on Windows
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )

install(EXPORT Yarr_Itkpixv2Targets
  NAMESPACE Yarr::
  DESTINATION ${CMAKE_CONFIG_INSTALL_PATH}/Yarr
)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING
        PATTERN "*.h"
)
