if(CMAKE_BUILD_TYPE MATCHES Asan)
    add_compile_options(-g -fsanitize=address -fsanitize=undefined)
    add_link_options(-fsanitize=address -fsanitize=undefined)
elseif(CMAKE_BUILD_TYPE MATCHES Debug)
    add_compile_options(-g)
else()
    add_compile_options(-O2)
endif()
add_compile_options(-Wfatal-errors)
add_compile_options(-Wno-deprecated)
add_compile_options(-std=c++17)

link_libraries(-lstdc++fs)

# Add -fPIC for (e.g. linking to shared libraries)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(YARR_FRONT_ENDS_TO_BUILD "Fei4;Rd53a;Star;Fe65p2"
    CACHE STRING "Semicolon-separated list of front-ends to build, or \"all\".")

set(YARR_CONTROLLERS_TO_BUILD "Spec;Emu"
    CACHE STRING "Semicolon-separated list of controllers to build, or \"all\".")

# Default to allowing debug macros (spdlog default is info)
set(YARR_ACTIVE_LOGGER_LEVEL DEBUG
    CACHE BOOL "This sets SPDLOG_ACTIVE_LEVEL, below which logger macros are disabled at build time. One of TRACE, DEBUG, INFO, WARN, ERROR, CRITICAL, OFF")

set(YARR_ALL_FRONT_ENDS
    Fe65p2 Fei4 Rd53a Star)

set(YARR_ALL_CONTROLLERS
    Spec Emu Rce Boc KU040 Rogue NetioHW)

if( YARR_FRONT_ENDS_TO_BUILD STREQUAL "all" )
  set( YARR_FRONT_ENDS_TO_BUILD ${YARR_ALL_FRONT_ENDS} )
endif()

if( YARR_CONTROLLERS_TO_BUILD STREQUAL "all" )
  set( YARR_CONTROLLERS_TO_BUILD ${YARR_ALL_CONTROLLERS} )
endif()

message("Controllers to build:")
foreach (_controller ${YARR_CONTROLLERS_TO_BUILD})
    message("- ${_controller}")
endforeach()
message("FrontEnds to build:")
foreach (_frontends ${YARR_FRONT_ENDS_TO_BUILD})
    message("- ${_frontends}")
endforeach()

#add libraries
list(APPEND libs Yarr Util)
list(APPEND libs ${YARR_CONTROLLERS_TO_BUILD} )
list(APPEND libs ${YARR_FRONT_ENDS_TO_BUILD} )

link_directories(${PROJECT_BINARY_DIR}/lib)

foreach(lib ${libs})
    add_definitions( -DNO_PYTHON )
    include_directories(lib${lib}/include)
    include_directories(BEFORE SYSTEM ${PROJECT_BINARY_DIR}/include)
    file(GLOB SrcFiles
         CONFIGURE_DEPENDS
         lib${lib}/*.cpp)
    list(APPEND LibSrcFiles ${SrcFiles})
endforeach()

# Level below which logger macros are not enabled
add_definitions(-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_${YARR_ACTIVE_LOGGER_LEVEL})

add_library(Yarr STATIC ${LibSrcFiles})

YARR_ADD_VARIANT()
add_dependencies(Yarr variant)
YARR_ADD_TBB()
add_dependencies(Yarr tbb_2019)

# Plotting Tools
find_package(ROOT COMPONENTS Hist Graf)
if (${ROOT_FOUND})
    YARR_ADD_PLOTTING()
    add_dependencies(Yarr plotting_tools)
    INSTALL(DIRECTORY ${CMAKE_BINARY_DIR}/src/external/src/plotting_tools/bin DESTINATION ${TARGET_INSTALL_AREA} FILES_MATCHING PATTERN "*" PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ WORLD_EXECUTE WORLD_READ GROUP_EXECUTE GROUP_READ)
else (${ROOT_FOUND})
    MESSAGE(STATUS "Could not locate ROOT. Will not compile plotting tools!")
endif (${ROOT_FOUND})

if ("Rogue" IN_LIST YARR_CONTROLLERS_TO_BUILD )
  YARR_ADD_ROGUE()
  add_dependencies(Yarr tbb_2019 rogue)
  set(YARR_EXEC_DEPS ${YARR_EXEC_DEPS} rogue tbb_2019)
  set(YARR_LIB_DEPS ${YARR_LIB_DEPS} rogue-core)
  set(TBB_DEPS tbb)
endif()

if ("NetioHW" IN_LIST YARR_CONTROLLERS_TO_BUILD )
  YARR_ADD_NETIO()
  YARR_ADD_FELIX()
  add_dependencies(Yarr felixbase4 netio4)

  set(YARR_EXEC_DEPS ${YARR_EXEC_DEPS} tbb_2019 felixbase4 netio4)
  set(YARR_LIB_DEPS ${YARR_LIB_DEPS} netio)
  set(TBB_DEPS tbb)

  include_directories(${CMAKE_CURRENT_BINARY_DIR}/external/src/felixbase4)
  include_directories(${CMAKE_CURRENT_BINARY_DIR}/external/src/netio4)
endif()

#add executables
file(GLOB tools
     CONFIGURE_DEPENDS
     tools/*.cpp)

if (NOT "NetioHW" IN_LIST YARR_CONTROLLERS_TO_BUILD )
  list(FILTER tools EXCLUDE REGEX tools/testNetioHw.cpp|tools/testBitStream.cpp|tools/testScan.cpp)
endif()

message("Executeables to build:")
foreach (_tool ${tools})
    message("- ${_tool}")
endforeach()

# Command to link Yarr
# Because of the auto-registration, we need to keep symbols that
# might otherwise be dropped (there's probably a better way to do this!)
# Mainly this is about making sure the auto-registration works.
#if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
if(APPLE)
  set(YARR_LINK_LIB_COMMAND
      -Wl,-all_load Yarr -Wl, ${YARR_LIB_DEPS} pthread)
else()
  # Whole-archive means not to drop symbols from Yarr.
  # We're not sure why tbb needs same treatment...
  set(YARR_LINK_LIB_COMMAND
      -Wl,--whole-archive Yarr ${TBB_DEPS} -Wl,--no-whole-archive ${YARR_LIB_DEPS} ${TBB_DEPS} pthread rt)
endif()
# (Not sure why rt is not linked by default here)

foreach(target ${tools})
    get_filename_component(execname ${target} NAME_WE)
    get_filename_component(srcfile ${target} NAME)
    add_executable(${execname} tools/${srcfile})
    add_dependencies(${execname} Yarr ${YARR_EXEC_DEPS})
    set_target_properties(${execname} PROPERTIES LINKER_LANGUAGE CXX)
    link_directories(BEFORE SYSTEM ${PROJECT_BINARY_DIR}/lib)
    target_link_libraries(${execname} ${YARR_LINK_LIB_COMMAND})
endforeach()

install(DIRECTORY ${PROJECT_BINARY_DIR}/bin DESTINATION ${TARGET_INSTALL_AREA}  FILES_MATCHING PATTERN "*" PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
    WORLD_EXECUTE WORLD_READ GROUP_EXECUTE GROUP_READ  )

# Tests
file(GLOB_RECURSE test_src
     CONFIGURE_DEPENDS
     tests/test_*.cpp)

# Make one executable
add_executable(test_main ${test_src} tests/main.cpp)
target_link_libraries(test_main ${YARR_LINK_LIB_COMMAND})
add_dependencies(test_main Yarr)

# This uses features of CTest, so requires enable_testing somewhere
add_test(catch2_tests ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test_main)
