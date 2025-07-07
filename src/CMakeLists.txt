YARR_ADD_SPDLOG()

add_subdirectory(libUtil)
add_subdirectory(libYarr)

# controllers
foreach(controller IN LISTS CONTROLLERS)
   message(STATUS "Building controller ${controller}" )
   add_subdirectory(lib${controller})
endforeach()

# front ends
foreach(frontend IN LISTS FRONT_ENDS)
   message(STATUS "Building frontend ${frontend}" )
   add_subdirectory(lib${frontend})
endforeach()

add_subdirectory(tools)

if(YARR_ENABLE_PYTHON)
   add_subdirectory(python)
endif()
