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
        )

post_build_debug_library(Util)
target_include_directories(Util PRIVATE ../libYarr/include)
target_include_directories(Util PUBLIC ./include)

if(BUILD_TESTS)
    add_subdirectory(tests)
endif()
