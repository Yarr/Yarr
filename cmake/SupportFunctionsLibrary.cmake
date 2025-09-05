function(get_git_version_info)
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
endfunction()

function(detect_in_tree_install OUT_VAR)
    if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.19")
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

function(detect_in_tdaq_install OUT_VAR)
    # Checking for configuration of "CMTCONFIG"
    if(YARR_INSTALL_BIN_CONFIG)
        set(${OUT_VAR} TRUE PARENT_SCOPE)
    else()
        set(${OUT_VAR} FALSE PARENT_SCOPE)
    endif()
endfunction()

# debug symbol stripping ---------------------------------------------------------------------------------------
function(post_build_debug_library name)
  set(our_debug_name lib${name}.so.debug)

  add_custom_command(TARGET ${name}
    POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} --only-keep-debug $<TARGET_FILE:${name}> ${CMAKE_CURRENT_BINARY_DIR}/${our_debug_name}
    COMMAND ${CMAKE_STRIP} --strip-debug --strip-unneeded $<TARGET_FILE:${name}>
    COMMAND ${CMAKE_OBJCOPY} --add-gnu-debuglink=${our_debug_name} $<TARGET_FILE:${name}>
  )

  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${our_debug_name}
	  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
endfunction(post_build_debug_library)

function(post_build_debug_executable name)
  set(our_debug_name ${name}.debug)

  add_custom_command(TARGET ${name}
    POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} --only-keep-debug $<TARGET_FILE:${name}> ${CMAKE_CURRENT_BINARY_DIR}/${our_debug_name}
    COMMAND ${CMAKE_STRIP} --strip-debug --strip-unneeded $<TARGET_FILE:${name}>
    COMMAND ${CMAKE_OBJCOPY} --add-gnu-debuglink=${our_debug_name} $<TARGET_FILE:${name}>
  )

  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${our_debug_name}
	  DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endfunction(post_build_debug_executable)
# debug symbol stripping ---------------------------------------------------------------------------------------

function(collect_and_group_all_targets result)
  set(allTargets "")

  # --- Collect directory-local targets recursively ---
  function(_collect_targets_recursive dir out_var)
    if(IS_DIRECTORY "${dir}")
      get_property(local_targets DIRECTORY "${dir}" PROPERTY BUILDSYSTEM_TARGETS)
      list(APPEND all_local_targets ${local_targets})
    endif()

    get_property(subdirs DIRECTORY "${dir}" PROPERTY SUBDIRECTORIES)
    foreach(subdir IN LISTS subdirs)
      _collect_targets_recursive("${subdir}" sub_targets)
      list(APPEND all_local_targets ${sub_targets})
    endforeach()

    set(${out_var} "${all_local_targets}" PARENT_SCOPE)
  endfunction()

  # Get recursive local targets
  if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.19")
     _collect_targets_recursive("${CMAKE_CURRENT_BINARY_DIR}" localTargets)
  endif()

  # Get global targets (like IMPORTED GLOBAL targets)
  get_property(globalTargets GLOBAL PROPERTY TARGETS)

  # Merge and remove duplicates
  list(APPEND allTargets ${localTargets} ${globalTargets})
  list(REMOVE_DUPLICATES allTargets)

  set(${result} "${allTargets}" PARENT_SCOPE)
endfunction()

function(print_all_targets)

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
endfunction()
