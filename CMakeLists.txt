cmake_minimum_required (VERSION 3.14)

project (YARR LANGUAGES CXX VERSION 1.5.4)

include(cmake/SupportFunctionsLibrary.cmake)

# compiler settings ---------------------------------------------------------------------------------------

# set C++ version as 17 - CMake does not support to set a minimum version globally (only per target) but
# if a higher version is requested by setting CMAKE_CXX_STANDARD it will be used instead
set(CMAKE_CXX_STANDARD 17 CACHE STRING "C++ standard to use")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# FIXME Is this not already covered by requesting C++17? Otherwise the specific CMake compiler capability check should be used.
# minimum required gcc version
if(NOT MIN_GCC_VERSION)
  set(MIN_GCC_VERSION 9)
endif()

# check for minimum required gcc and clang versions
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

# needs always the debug version in order to produce the stripped debug info - until something native gets introduced in CMake
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

add_compile_options(
  $<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang>:-Wfatal-errors>
  $<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang>:-Wno-deprecated>
  $<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang>:-Wnon-virtual-dtor>
)

# compiler settings ---------------------------------------------------------------------------------------

# set global options  ---------------------------------------------------------------------------------------
# Debug buffer and buffersize for Rd53b/Itkpixv2
if(DEFINED USE_ITKPIX_DEBUG_BUFFER)
    add_compile_definitions(USE_ITKPIX_DEBUG_BUFFER=${USE_ITKPIX_DEBUG_BUFFER})
    message(STATUS "Set USE_ITKPIX_DEBUG_BUFFER=${USE_ITKPIX_DEBUG_BUFFER}")
endif()

if(DEFINED ITKPIX_DEBUG_BUFFERSIZE)
    add_compile_definitions(ITKPIX_DEBUG_BUFFERSIZE=${ITKPIX_DEBUG_BUFFERSIZE})
    message(STATUS "Set ITKPIX_DEBUG_BUFFERSIZE=${ITKPIX_DEBUG_BUFFERSIZE}")
endif()


# Export compile commands JSON
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# set global options  ---------------------------------------------------------------------------------------

# internal global state tracking variables  ---------------------------------------------------------------------------------------

# tracking whether spdlog was fetched and built or whether it gets provided somehow externally
# check in order to avoid overwriting in case of re-configure
if(NOT DEFINED YARR_HAS_BUILTIN_SPDLOG)
    set(YARR_HAS_BUILTIN_SPDLOG FALSE CACHE INTERNAL "Whether Yarr provides its own spdlog")
endif()

# internal global state tracking variables  ---------------------------------------------------------------------------------------

# get version information from GIT ---------------------------------------------------------------------------------------

get_git_version_info()
string(TIMESTAMP BUILD_TIME "%Y-%m-%d %H:%M:%S UTC" UTC)
configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/src/libYarr/include/yarr_version.h.in
        ${CMAKE_CURRENT_BINARY_DIR}/src/libYarr/include/yarr_version.h
)
# get version information from GIT ---------------------------------------------------------------------------------------

# setting install paths and RPATH  ---------------------------------------------------------------------------------------
# Standard install dir layout
include(GNUInstallDirs)

# Set RPATH
set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

set(CMAKE_INSTALL_RPATH "$ORIGIN/../${CMAKE_INSTALL_LIBDIR}")

# Clear existing RPATHs to be safe

# Loop over all targets
get_property(all_targets GLOBAL PROPERTY TARGETS)

foreach(target IN LISTS all_targets)
    if (NOT target MATCHES "^Yarr::")
        continue()
    endif()
    
    # Skip imported targets (you can't get usage requirements from those reliably)
    get_target_property(is_imported ${target} IMPORTED)
    if (is_imported)
        continue()
    endif()

    # Get all link directories from INTERFACE_LINK_DIRECTORIES
    get_target_property(link_dirs ${target} INTERFACE_LINK_DIRECTORIES)
    if (NOT link_dirs)
        continue()
    endif()

    foreach(dir IN LISTS link_dirs)
        file(REAL_PATH "${dir}" resolved_dir)

        # Add to RPATH only if it's not part of the install prefix
        if(NOT resolved_dir MATCHES "^${CMAKE_INSTALL_PREFIX}")
            list(APPEND CMAKE_INSTALL_RPATH "${resolved_dir}")
        endif()
    endforeach()
endforeach()

# Show the result for debugging
message(STATUS "Final install RPATH: ${CMAKE_INSTALL_RPATH}")

# Remove duplicates
list(REMOVE_DUPLICATES CMAKE_INSTALL_RPATH)

# deal with in-source installs
option(YARR_FORCE_OWN_INSTALL_PREFIX "Force Yarr to use its own install prefix even as subdirectory" OFF)

if(NOT YARR_FORCE_OWN_INSTALL_PREFIX AND NOT CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    # Being added via add_subdirectory: inherit parent's prefix
    set(CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}" CACHE PATH "Install path" FORCE)
    message("Installation will be done at ${CMAKE_INSTALL_PREFIX} as Yarr has been included by add_subdirectory into another project")
elseif(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    # Only set the install prefix if it hasn’t already been set by a parent project and if it is the default path
    set_property(CACHE CMAKE_INSTALL_PREFIX PROPERTY VALUE "${PROJECT_SOURCE_DIR}")
    message("Installation will be done in-tree as no location other than the default location has been chosen.")
endif()

detect_in_tree_install(IS_IN_TREE_INSTALL)
if(IS_IN_TREE_INSTALL)
    set(CMAKE_CONFIG_INSTALL_PATH lib/cmake)
else()
    set(CMAKE_CONFIG_INSTALL_PATH cmake)
endif()

message(STATUS "YARR will be installed to: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "YARR libraries: ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
message(STATUS "YARR executables: ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}")
message(STATUS "YARR headers: ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}")
message(STATUS "YARR cmake configs: ${CMAKE_INSTALL_PREFIX}/${CMAKE_CONFIG_INSTALL_PATH}")

install(CODE "message(STATUS \"Installing YARR to: ${CMAKE_INSTALL_PREFIX}\")")
install(CODE "message(STATUS \"Installing YARR libraries to: ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}\")")
install(CODE "message(STATUS \"Installing YARR executables to: ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}\")")
install(CODE "message(STATUS \"Installing YARR headers to: ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}\")")
install(CODE "message(STATUS \"Installing YARR cmake configs to: ${CMAKE_INSTALL_PREFIX}/${CMAKE_CONFIG_INSTALL_PATH}\")")


# setting install paths and RPATH ---------------------------------------------------------------------------------------

# setting build options ---------------------------------------------------------------------------------------

option(BUILD_TESTS "Build the YARR test suite" OFF)

set(YARR_FRONT_ENDS_TO_BUILD "Fei4;Rd53a;Star;Rd53b;Itkpixv2"
    CACHE STRING "Semicolon-separated list of controllers to build, or \"all\"")
set(YARR_CONTROLLERS_TO_BUILD  "Spec;Emu"
    CACHE STRING "Semicolon-separated list of controllers to build, or \"all\".")
set(YARR_EMULATORS_TO_BUILD "StarEmu;Fei4Emu;Rd53aEmu;Itkpixv2Emu"
    CACHE STRING "Semicolon-separated list of front-end specific emulators to build (if Emu in controller list)")

set(YARR_ACTIVE_LOGGER_LEVEL "DEBUG"
    CACHE STRING "This sets SPDLOG_ACTIVE_LEVEL, below which logger macros are disabled at build time. One of TRACE, DEBUG, INFO, WARN, ERROR, CRITICAL, OFF")

# the variables FRONT_ENDS and CONTROLLERS will be used to steer the actual
# building of libraries downstream
set(FRONT_ENDS "")
set(CONTROLLERS "")

# only build those controllers specified in YARR_CONTROLLERS_TO_BUILD
if(YARR_CONTROLLERS_TO_BUILD STREQUAL "all")
   set(CONTROLLERS "Spec;Emu;StarEmu;Fei4Emu;Rd53aEmu;Itkpixv2Emu;Bdaq;NetioHW;FelixClient;ItsdaqFW")
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

option(YARR_ENABLE_PYTHON "Build python bindings" OFF)

# setting build options ---------------------------------------------------------------------------------------

# finally include all source, create package config files and install ---------------------------------------------------------------
include(cmake/CMakeLists.txt.external)
add_subdirectory(src)

include(CMakePackageConfigHelpers)
configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/YarrConfig.cmake.in
                              ${CMAKE_CURRENT_BINARY_DIR}/YarrConfig.cmake
                              INSTALL_DESTINATION ${CMAKE_CONFIG_INSTALL_PATH}
                              )

write_basic_package_version_file(YarrConfigVersion.cmake
                                 VERSION ${PROJECT_VERSION}
                                 COMPATIBILITY AnyNewerVersion)

# Install the export set for use with the install-tree
install(EXPORT YarrTargets
  NAMESPACE Yarr::
  DESTINATION ${CMAKE_CONFIG_INSTALL_PATH}
)

install(FILES
  "${PROJECT_BINARY_DIR}/YarrConfig.cmake"
  "${PROJECT_BINARY_DIR}/YarrConfigVersion.cmake"
  DESTINATION ${CMAKE_CONFIG_INSTALL_PATH}
)

if(IS_IN_TREE_INSTALL)
    message(WARNING "Skipping configs/ install because in-tree install detected")
else()
    install(DIRECTORY configs/
        DESTINATION configs
        USE_SOURCE_PERMISSIONS
    )
    message("Installing configs")
endif()

# finally include all source, create package config files and install ---------------------------------------------------------------

# list all targets for info purposes ---------------------------------------------------------------------------------------

option(YARR_DEBUG_PRINT_TARGETS "Print all targets and their properties" OFF)

if(YARR_DEBUG_PRINT_TARGETS)
    print_all_targets()
endif()

# list all targets for info purposes ---------------------------------------------------------------------------------------
