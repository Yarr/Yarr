YARR_ADD_EXTERNAL_DEPS()


add_subdirectory(libUtil)
add_subdirectory(libYarr)
# controllers
foreach(controller IN LISTS CONTROLLERS)
   if(controller MATCHES "NetioHW")
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
