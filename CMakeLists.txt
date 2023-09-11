cmake_minimum_required (VERSION 3.14)

project (YARR LANGUAGES CXX)
set (YARR_VERSION_MAJOR 1)
set (YARR_VERSION_MINOR 0)
set (CMAKE_CXX_STANDARD 17)

# minimum required gcc version
if(NOT MIN_GCC_VERSION)
  set(MIN_GCC_VERSION 9)
endif()

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS ${MIN_GCC_VERSION})
        message(FATAL_ERROR "GCC version must be at least ${MIN_GCC_VERSION}!")
    endif()
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 11.0)
        message(FATAL_ERROR "Clang version must be at least 11.0!")
    endif()
else()
    message(WARNING "You are using an unsupported compiler! Compilation has only been tested with Clang and GCC.")
endif()

add_custom_target(git_hash ALL
  COMMAND scripts/git_hash.sh
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)



# Build dirs
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Set RPATH to find shlib relative to executable
SET(CMAKE_BUILD_RPATH_USE_ORIGIN ON)
SET(CMAKE_BUILD_RPATH ../lib)

add_compile_options(-g)

if(CMAKE_BUILD_TYPE MATCHES Asan)
    add_compile_options(-fsanitize=address -fsanitize=undefined)
    add_link_options(-fsanitize=address -fsanitize=undefined)
elseif(CMAKE_BUILD_TYPE MATCHES Debug)
    # Don't optimize
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
option(DISABLE_PLOTTING_TOOLS "Disable build of plotting tools" OFF)
set(YARR_FRONT_ENDS_TO_BUILD "Fei4;Rd53a;Star;Rd53b;Itkpixv2"
    CACHE STRING "Semicolon-separated list of controllers to build, or \"all\"")
set(YARR_CONTROLLERS_TO_BUILD  "Spec;Emu"
    CACHE STRING "Semicolon-separated list of controllers to build, or \"all\".")
set(YARR_EMULATORS_TO_BUILD "StarEmu;Fei4Emu;Rd53aEmu"
    CACHE STRING "Semicolon-separated list of front-end specific emulators to build (if Emu in controller list)")

set(YARR_ACTIVE_LOGGER_LEVEL "DEBUG"
    CACHE STRING "This sets SPDLOG_ACTIVE_LEVEL, below which logger macros are disabled at build time. One of TRACE, DEBUG, INFO, WARN, ERROR, CRITICAL, OFF")

# the variables FRONT_ENDS and CONTROLLERS will be used to steer the actual
# building of libraries downstream
set(FRONT_ENDS "")
set(CONTROLLERS "")

# only build those controllers specified in YARR_CONTROLLERS_TO_BUILD
if(YARR_CONTROLLERS_TO_BUILD STREQUAL "all")
   set(CONTROLLERS "Spec;Emu;StarEmu;Fei4Emu;Rd53aEmu;Bdaq;Rogue;NetioHW;FelixClient;ItsdaqFW")
else()
  foreach(ctrl IN LISTS YARR_CONTROLLERS_TO_BUILD )
    if(NOT ${ctrl} IN_LIST CONTROLLERS)
       LIST(APPEND CONTROLLERS ${ctrl})
    endif()
    if((${ctrl} MATCHES Emu) AND (NOT Emu IN_LIST CONTROLLERS))
       LIST(APPEND CONTROLLERS Emu)
    endif()
  endforeach()
endif()

# only build those front-ends specified in YARR_FRONT_ENDS_TO_BUILD
if(YARR_FRONT_ENDS_TO_BUILD STREQUAL "all")
    set(FRONT_ENDS "Fei4;Rd53a;Star;Rd53b;Itkpixv2")
else()
    foreach(fe IN LISTS YARR_FRONT_ENDS_TO_BUILD)
        if(NOT ${fe} IN_LIST FRONT_ENDS)
            LIST(APPEND FRONT_ENDS ${fe})
        endif()
    endforeach()
endif()

if(Emu IN_LIST CONTROLLERS)
    foreach(fe_emu IN LISTS YARR_EMULATORS_TO_BUILD)
        if(NOT ${fe_emu} IN_LIST CONTROLLERS)
            LIST(APPEND CONTROLLERS ${fe_emu})
        endif()
    endforeach()
endif()

if(BUILD_TESTS)
    message(STATUS "Building the YARR test suite" )
    enable_testing()
endif()
set(TARGET_INSTALL_AREA ${PROJECT_SOURCE_DIR})
set(SHARED_INSTALL_AREA ${PROJECT_SOURCE_DIR}/installed/)

function(post_build_debug_library name)
  add_custom_command(TARGET ${name}
    POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} --only-keep-debug $<TARGET_FILE:${name}> ${CMAKE_CURRENT_BINARY_DIR}/lib${name}.so.debug
    COMMAND ${CMAKE_STRIP} --strip-debug --strip-unneeded $<TARGET_FILE:${name}>
    COMMAND ${CMAKE_OBJCOPY} --add-gnu-debuglink=lib${name}.so.debug $<TARGET_FILE:${name}>
  )

  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/lib${name}.so.debug
    DESTINATION ${TARGET_INSTALL_AREA}/lib
    )
endfunction(post_build_debug_library)

function(post_build_debug_executable name)
  add_custom_command(TARGET ${name}
    POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} --only-keep-debug $<TARGET_FILE:${name}> ${CMAKE_CURRENT_BINARY_DIR}/${name}.debug
    COMMAND ${CMAKE_STRIP} --strip-debug --strip-unneeded $<TARGET_FILE:${name}>
    COMMAND ${CMAKE_OBJCOPY} --add-gnu-debuglink=${name}.debug $<TARGET_FILE:${name}>
  )

  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${name}.debug
    DESTINATION ${TARGET_INSTALL_AREA}/bin
    )
endfunction(post_build_debug_executable)

include(cmake/CMakeLists.txt.external)
add_subdirectory(src)

install(DIRECTORY ${PROJECT_BINARY_DIR}/bin DESTINATION ${TARGET_INSTALL_AREA}  FILES_MATCHING PATTERN "*" PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
    WORLD_EXECUTE WORLD_READ GROUP_EXECUTE GROUP_READ  )
install(DIRECTORY ${PROJECT_BINARY_DIR}/lib DESTINATION ${TARGET_INSTALL_AREA}  FILES_MATCHING PATTERN "*" PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
    WORLD_EXECUTE WORLD_READ GROUP_EXECUTE GROUP_READ  )
