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
        Itkpixv2TriggerLoop.cpp)

post_build_debug_library(Itkpixv2)
target_include_directories(Itkpixv2 PRIVATE ../libYarr/include)
target_include_directories(Itkpixv2 PRIVATE ../libUtil/include)
target_include_directories(Itkpixv2 PUBLIC ./include)
target_link_libraries(Itkpixv2 Scan)

LIST(APPEND apps
        ITkPixV2CheckPulse)

foreach (target IN LISTS apps)
    add_executable(${target} app/${target}.cpp)
    target_link_libraries(${target} Yarr)
    post_build_debug_executable(${target})
endforeach ()

if (BUILD_TESTS)
    #    add_subdirectory(tests)
endif ()
