cmake_minimum_required (VERSION 3.14)

project (YARR LANGUAGES CXX VERSION 1.0)

# compiler settings 
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


# Get version information from GIT
execute_process(
        COMMAND git rev-parse HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE YARR_GIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
        COMMAND git rev-parse --abbrev-ref HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE YARR_GIT_BRANCH
        OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
        COMMAND git describe --tag
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE YARR_GIT_TAG
        OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
        COMMAND git log -1 --format=%ad --date=iso
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE YARR_GIT_DATE
        OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
        COMMAND git log -1 --format=%s
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE YARR_GIT_SUBJECT
        OUTPUT_STRIP_TRAILING_WHITESPACE
)
string(TIMESTAMP BUILD_TIME "%Y-%m-%d %H:%M:%S UTC" UTC)
configure_file(
        ${PROJECT_SOURCE_DIR}/src/libYarr/include/yarr_version.h.in
        ${PROJECT_BINARY_DIR}/src/libYarr/include/yarr_version.h
)


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


function(post_build_debug_library name)
  add_custom_command(TARGET ${name}
    POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} --only-keep-debug $<TARGET_FILE:${name}> ${CMAKE_CURRENT_BINARY_DIR}/lib${name}.so.debug
    COMMAND ${CMAKE_STRIP} --strip-debug --strip-unneeded $<TARGET_FILE:${name}>
    COMMAND ${CMAKE_OBJCOPY} --add-gnu-debuglink=lib${name}.so.debug $<TARGET_FILE:${name}>
  )

  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/lib${name}.so.debug
	  DESTINATION ${CMAKE_INSTALL_LIBDIR}
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
	  DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endfunction(post_build_debug_executable)

include(cmake/CMakeLists.txt.external)
add_subdirectory(src)

    
include(CMakePackageConfigHelpers)
configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/YarrConfig.cmake.in
                              ${CMAKE_CURRENT_BINARY_DIR}/YarrConfig.cmake
                              INSTALL_DESTINATION cmake
                              )

write_basic_package_version_file(YarrConfigVersion.cmake
                                 VERSION ${PROJECT_VERSION}
                                 COMPATIBILITY AnyNewerVersion)

# Install the export set for use with the install-tree
install(EXPORT YarrTargets
  NAMESPACE Yarr::
  DESTINATION cmake
)

install(FILES
  "${PROJECT_BINARY_DIR}/YarrConfig.cmake"
  "${PROJECT_BINARY_DIR}/YarrConfigVersion.cmake"
  DESTINATION cmake
)

message(STATUS "YARR will be installed to: ${CMAKE_INSTALL_PREFIX}")
install(CODE "message(STATUS \"Installing YARR to: ${CMAKE_INSTALL_PREFIX}\")")

function(detect_in_tree_install OUT_VAR)
    if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.13")
        # Use reliable real path resolution if available
        file(REAL_PATH "${CMAKE_SOURCE_DIR}" ABS_SOURCE_DIR)
        file(REAL_PATH "${CMAKE_BINARY_DIR}" ABS_BINARY_DIR)
        file(REAL_PATH "${CMAKE_INSTALL_PREFIX}" ABS_INSTALL_PREFIX)
    else() # FIXME Remove workaround after CC7 drop
        get_filename_component(ABS_SOURCE_DIR "${CMAKE_SOURCE_DIR}" ABSOLUTE)
        get_filename_component(ABS_BINARY_DIR "${CMAKE_BINARY_DIR}" ABSOLUTE)
        get_filename_component(ABS_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}" ABSOLUTE)
    endif()

    # Check if install prefix is within source or binary dir
    string(FIND "${ABS_INSTALL_PREFIX}" "${ABS_SOURCE_DIR}" SOURCE_POS)
    string(FIND "${ABS_INSTALL_PREFIX}" "${ABS_BINARY_DIR}" BINARY_POS)

    if(SOURCE_POS EQUAL 0 OR BINARY_POS EQUAL 0)
        set(${OUT_VAR} TRUE PARENT_SCOPE)
    else()
        set(${OUT_VAR} FALSE PARENT_SCOPE)
    endif()
endfunction()
detect_in_tree_install(IS_IN_TREE_INSTALL)

if(IS_IN_TREE_INSTALL)
    message(WARNING "Skipping configs/ install because in-tree install detected")
else()
    install(DIRECTORY configs/
        DESTINATION configs
        USE_SOURCE_PERMISSIONS
    )
    message("Installing configs")
endif()

function(collect_and_group_all_targets result)
  set(allTargets "")

  # --- Collect directory-local targets recursively ---
  function(_collect_targets_recursive dir out_var)
    get_property(local_targets DIRECTORY "${dir}" PROPERTY BUILDSYSTEM_TARGETS)
    list(APPEND all_local_targets ${local_targets})

    get_property(subdirs DIRECTORY "${dir}" PROPERTY SUBDIRECTORIES)
    foreach(subdir IN LISTS subdirs)
      _collect_targets_recursive("${subdir}" sub_targets)
      list(APPEND all_local_targets ${sub_targets})
    endforeach()

    set(${out_var} "${all_local_targets}" PARENT_SCOPE)
  endfunction()

  # Get recursive local targets
  _collect_targets_recursive("${CMAKE_CURRENT_BINARY_DIR}" localTargets)

  # Get global targets (like IMPORTED GLOBAL targets)
  get_property(globalTargets GLOBAL PROPERTY TARGETS)

  # Merge and remove duplicates
  list(APPEND allTargets ${localTargets} ${globalTargets})
  list(REMOVE_DUPLICATES allTargets)

  set(${result} "${allTargets}" PARENT_SCOPE)
endfunction()
# summary of targets
collect_and_group_all_targets(allTargets)

# Prepare lists
set(libraries "")
set(executables "")
set(interface_libraries "")
set(other_targets "")

foreach(tgt ${allTargets})
  get_target_property(type ${tgt} TYPE)
  if(NOT type)
    set(type "UNKNOWN")
  endif()

  if(type STREQUAL "STATIC_LIBRARY" OR type STREQUAL "SHARED_LIBRARY" OR type STREQUAL "MODULE_LIBRARY")
    list(APPEND libraries ${tgt})
  elseif(type STREQUAL "EXECUTABLE")
    list(APPEND executables ${tgt})
  elseif(type STREQUAL "INTERFACE_LIBRARY")
    list(APPEND interface_libraries ${tgt})
  else()
    list(APPEND other_targets "${tgt} (${type})")
  endif()
endforeach()


message(STATUS "==== Project Targets Summary ====")

if(libraries)
  message(STATUS "Libraries:")
  foreach(lib ${libraries})
    message(STATUS "  - ${lib}")
  endforeach()
endif()

if(interface_libraries)
  message(STATUS "Interface Libraries:")
  foreach(lib ${interface_libraries})
    message(STATUS "  - ${lib}")
  endforeach()
endif()

if(executables)
  message(STATUS "Executables:")
  foreach(exe ${executables})
    message(STATUS "  - ${exe}")
  endforeach()
endif()

if(other_targets)
  message(STATUS "Other Targets:")
  foreach(other ${other_targets})
    message(STATUS "  - ${other}")
  endforeach()
endif()

message(STATUS "==================================")

