YARR_ADD_EXTERNAL_DEPS()

# Level below which logger macros are not enabled
add_definitions(-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_${YARR_ACTIVE_LOGGER_LEVEL})

add_subdirectory(libUtil)
add_subdirectory(libYarr)
# controllers
foreach(controller IN LISTS CONTROLLERS)
   if(controller MATCHES "NetioHW" AND NOT LSB_RELEASE_ID_SHORT MATCHES "AlmaLinux 9")
      YARR_ADD_TBB()
   endif()
   message(STATUS "Building controller ${controller}" )
   add_subdirectory(lib${controller})
endforeach()

# front ends
foreach(frontend IN LISTS FRONT_ENDS)
   message(STATUS "Building frontend ${frontend}" )
   add_subdirectory(lib${frontend})
endforeach()

add_subdirectory(tools)

if(ENABLE_PYTHON)
   add_subdirectory(python)
endif()
