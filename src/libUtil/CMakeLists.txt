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
        LoggingConfig.cpp
        SerialCom.cpp
        catch_amalgamated.cpp
        ItkpixEncoder.cpp
        HitMapGenerator.cpp)

post_build_debug_library(Util)
target_include_directories(Util PRIVATE ../libYarr/include)
target_include_directories(Util PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
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
