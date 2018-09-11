cmake_minimum_required (VERSION 2.8)
project (YARR)
set (YARR_VERSION_MAJOR 1)
set (YARR_VERSION_MINOR 0)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.8)
        message(FATAL_ERROR "GCC version must be at least 4.8!")
    endif()
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 3.2)
        message(FATAL_ERROR "Clang version must be at least 3.2!")
    endif()
else()
    message(WARNING "You are using an unsupported compiler! Compilation has only been tested with Clang and GCC.")
endif()

if(CMAKE_BUILD_TYPE MATCHES Debug)
add_compile_options(-g)
else()
add_compile_options(-O2)
endif()
add_compile_options(-Wfatal-errors)
add_compile_options(-Wno-deprecated)
add_compile_options(-std=c++11)

# Add -fPIC for (e.g. linking to shared libraries)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(ENABLE_NETIO "Enable NetIO, needs NETIO_DIR setting separately" OFF)

#add libraries
list(APPEND libs Yarr Spec Util Emu Fe65p2 Fei4 Rce Boc Rd53a KU040)
if(ENABLE_NETIO)
    list(APPEND libs NetioHW)
endif()

foreach(lib ${libs})
include_directories(lib${lib}/include)
file(GLOB SrcFiles lib${lib}/*.cpp)
list(APPEND LibSrcFiles ${SrcFiles})
endforeach()
add_library(Yarr STATIC ${LibSrcFiles})

set(NETIO_DIR "/path/to/felix/software" CACHE
    PATH "Path to FELIX software distribution for NetIO library")

if(ENABLE_NETIO)
  include_directories("${NETIO_DIR}/include")
endif()

#add executables
file(GLOB tools tools/*.cpp)
if(NOT ENABLE_NETIO)
  file(GLOB tools_netio tools/testNetioHw.cpp tools/testScan.cpp tools/testBitStream.cpp)
  list(REMOVE_ITEM tools ${tools_netio})
  # CMake 3. something has filter command
  ## list(FILTER tools EXCLUDE REGEX tools/testNetioHw.cpp tools/testBitStream.cpp tools/testScan.cpp)
  # message(${tools})
endif()

foreach(target ${tools})
get_filename_component(execname ${target} NAME_WE)
get_filename_component(srcfile ${target} NAME)
add_executable(${execname} tools/${srcfile})
add_dependencies(${execname} Yarr)
set_target_properties(${execname} PROPERTIES LINKER_LANGUAGE CXX)
# Whole-archive means not to drop symbols from Yarr.
# Mainly this is about making sure the auto-registration works.
if(APPLE)
    set(WRAPPED_YARR -Wl,-all_load Yarr)
    target_link_libraries(${execname} ${WRAPPED_YARR} pthread)
elseif(UNIX)
    set(WRAPPED_YARR -Wl,--whole-archive Yarr -Wl,--no-whole-archive)
    target_link_libraries(${execname} ${WRAPPED_YARR} pthread rt)

    if(ENABLE_NETIO)
        target_link_libraries(${execname} "${NETIO_DIR}/lib/libnetio.so")
    endif()
endif()
# Not sure why rt is not linked by default here
endforeach()
