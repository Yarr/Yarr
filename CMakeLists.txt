cmake_minimum_required (VERSION 3.6)

project (YARR LANGUAGES CXX)
set (YARR_VERSION_MAJOR 1)
set (YARR_VERSION_MINOR 0)
set (CMAKE_CXX_STANDARD 17)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 7.3)
        message(FATAL_ERROR "GCC version must be at least 7.3!")
    endif()
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 7.0)
        message(FATAL_ERROR "Clang version must be at least 7.0!")
    endif()
else()
    message(WARNING "You are using an unsupported compiler! Compilation has only been tested with Clang and GCC.")
endif()


# Build dirs
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(CMAKE_BUILD_TYPE MATCHES Asan)
    add_compile_options(-g -fsanitize=address -fsanitize=undefined)
    add_link_options(-fsanitize=address -fsanitize=undefined)
elseif(CMAKE_BUILD_TYPE MATCHES Debug)
    add_compile_options(-g)
elseif(CMAKE_BUILD_TYPE MATCHES Coverage)
#  Be sure not to use optimization for proper codecoverage results
    add_compile_options(--coverage)
    add_link_options(--coverage)
else()
    add_compile_options(-O2)
endif()
add_compile_options(-Wfatal-errors)
add_compile_options(-Wno-deprecated)
add_compile_options(-Wnon-virtual-dtor)

option(BUILD_TESTS "Build the YARR test suite" OFF)
set(YARR_FRONT_ENDS_TO_BUILD "Fei4;Rd53a;Star;Fe65p2;Rd53b"
    CACHE STRING "Semicolon-separated list of controllers to build, or \"all\"")
set(YARR_CONTROLLERS_TO_BUILD  "Spec;Emu"
    CACHE STRING "Semicolon-separated list of controllers to build, or \"all\".")
set(YARR_ACTIVE_LOGGER_LEVEL "DEBUG"
    CACHE STRING "This sets SPDLOG_ACTIVE_LEVEL, below which logger macros are disabled at build time. One of TRACE, DEBUG, INFO, WARN, ERROR, CRITICAL, OFF")

# always build all frontends
set(FRONT_ENDS "Fei4;Rd53a;Star;Fe65p2;Rd53b")

# build always these controllers
set(CONTROLLERS "Spec;Emu")

if(YARR_CONTROLLERS_TO_BUILD STREQUAL "all")
   set(CONTROLLERS "Spec;Emu;Bdaq;Rogue;NetioHW;Boc;ItsdaqFW;KU040")
else()
  foreach(ctrl IN LISTS YARR_CONTROLLERS_TO_BUILD )
    if(NOT ${ctrl} IN_LIST CONTROLLERS)
       LIST(APPEND CONTROLLERS ${ctrl})
    endif()
  endforeach()
endif()

if(BUILD_TESTS)
    message(STATUS "Building the YARR test suite" )
    enable_testing()
endif()
set(TARGET_INSTALL_AREA ${PROJECT_SOURCE_DIR})
set(SHARED_INSTALL_AREA ${PROJECT_SOURCE_DIR}/installed/)
include(cmake/CMakeLists.txt.external)
add_subdirectory(src)

install(DIRECTORY ${PROJECT_BINARY_DIR}/bin DESTINATION ${TARGET_INSTALL_AREA}  FILES_MATCHING PATTERN "*" PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
    WORLD_EXECUTE WORLD_READ GROUP_EXECUTE GROUP_READ  )
install(DIRECTORY ${PROJECT_BINARY_DIR}/lib DESTINATION ${TARGET_INSTALL_AREA}  FILES_MATCHING PATTERN "*" PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
    WORLD_EXECUTE WORLD_READ GROUP_EXECUTE GROUP_READ  )
