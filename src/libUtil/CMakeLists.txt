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
        LoggingConfig.cpp
        SerialCom.cpp
        )

add_dependencies(Util variant)
target_include_directories(Util PRIVATE ../libYarr/include)
target_include_directories(Util PUBLIC ./include)

if(BUILD_TEST)
    add_subdirectory(tests)
endif()