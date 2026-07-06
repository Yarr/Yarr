add_library(Util SHARED
        BitFile.cpp
        DBHandler.cpp
        Gauss.cpp
        GraphErrors.cpp
        Histo1d.cpp
        Histo2d.cpp
        Histo3d.cpp
        lmcurve.cpp
        lmmin.cpp
        scurvegauss.cpp
        LoadJson.cpp
        LoggingConfig.cpp
        SerialCom.cpp
        ItkpixEncoder.cpp
        HitMapGenerator.cpp)

post_build_debug_library(Util)
target_include_directories(Util PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(Util PRIVATE Yarr_interface)
target_link_libraries(Util PUBLIC yarrspdlog)

target_compile_options(
    Util
  PRIVATE
    $<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang>:-Wall>
)

if(BUILD_TESTS)
    add_subdirectory(tests)
endif()

install(TARGETS Util
    EXPORT YarrTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}  # For DLLs on Windows
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )
  

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING 
        PATTERN "*.h"
        PATTERN "*.hpp"
)

# add interface only target to resolve cyclical dependency in a cleaner way (until CMAKE 3.27 which offers a different mechanism)
add_library(Util_interface INTERFACE)

target_include_directories(Util_interface INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(Util_interface INTERFACE yarrspdlog)

install(TARGETS Util_interface
    EXPORT YarrTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}  # For DLLs on Windows
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )
