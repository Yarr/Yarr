add_library(Emu SHARED
        PixelModel.cpp
        Rd53aDiffPixelModel.cpp
        Rd53aLinPixelModel.cpp
        Rd53aSyncPixelModel.cpp
        StripModel.cpp)

post_build_debug_library(Emu)
target_include_directories(Emu PRIVATE ../libYarr/include)
target_include_directories(Emu PRIVATE ../libUtil/include)
target_include_directories(Emu PUBLIC ./include)
target_link_libraries(Emu Scan)

if (BUILD_TESTS)
    add_subdirectory(tests)
endif ()
