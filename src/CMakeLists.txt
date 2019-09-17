if(CMAKE_BUILD_TYPE MATCHES Debug)
    add_compile_options(-g)
else()
    add_compile_options(-O2)
endif()
add_compile_options(-Wfatal-errors)
add_compile_options(-Wno-deprecated)
add_compile_options(-std=c++17)

# Add -fPIC for (e.g. linking to shared libraries)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(YARR_FRONT_ENDS_TO_BUILD "Fei4;Rd53a;Star;Fe65p2"
    CACHE STRING "Semicolon-separated list of front-ends to build, or \"all\".")

set(YARR_CONTROLLERS_TO_BUILD "Spec;Emu"
    CACHE STRING "Semicolon-separated list of controllers to build, or \"all\".")

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
    file(GLOB SrcFiles lib${lib}/*.cpp)
    list(APPEND LibSrcFiles ${SrcFiles})
endforeach()

add_library(Yarr STATIC ${LibSrcFiles})

YARR_ADD_VARIANT()
add_dependencies(Yarr variant)
YARR_ADD_TBB()
add_dependencies(Yarr tbb_2019)

if ("Rogue" IN_LIST YARR_CONTROLLERS_TO_BUILD )
  YARR_ADD_ROGUE()
  add_dependencies(Yarr tbb_2019)
  set(YARR_EXEC_DEPS ${YARR_EXEC_DEPS} rogue tbb_2019)
  set(YARR_LIB_DEPS ${YARR_LIB_DEPS} tbb rogue-core)
endif()

if ("NetioHW" IN_LIST YARR_CONTROLLERS_TO_BUILD )
  YARR_ADD_NETIO()
  YARR_ADD_FELIX()
  add_dependencies(Yarr felixbase4 netio4)

  set(YARR_EXEC_DEPS ${YARR_EXEC_DEPS} tbb_2019 felixbase4 netio4)
  set(YARR_LIB_DEPS ${YARR_LIB_DEPS} tbb netio)

  include_directories(${CMAKE_CURRENT_BINARY_DIR}/external/src/felixbase4)
  include_directories(${CMAKE_CURRENT_BINARY_DIR}/external/src/netio4)
endif()

#add executables
file(GLOB tools tools/*.cpp)

if (NOT "NetioHW" IN_LIST YARR_CONTROLLERS_TO_BUILD )
  list(FILTER tools EXCLUDE REGEX tools/testNetioHw.cpp|tools/testBitStream.cpp|tools/testScan.cpp)
endif()

message("Executeables to build:")
foreach (_tool ${tools})
    message("- ${_tool}")
endforeach()


foreach(target ${tools})
    get_filename_component(execname ${target} NAME_WE)
    get_filename_component(srcfile ${target} NAME)
    add_executable(${execname} tools/${srcfile})
    add_dependencies(${execname} Yarr ${YARR_EXEC_DEPS})
    set_target_properties(${execname} PROPERTIES LINKER_LANGUAGE CXX)
    link_directories(BEFORE SYSTEM ${PROJECT_BINARY_DIR}/lib)
    # Whole-archive means not to drop symbols from Yarr.
    # Mainly this is about making sure the auto-registration works.
    # Not sure why tbb needs same treatment...
    if(APPLE)
        target_link_libraries(${execname}  -Wl,-all_load Yarr -Wl, ${YARR_LIB_DEPS} pthread)
    else()
        target_link_libraries(${execname}  -Wl,--whole-archive Yarr tbb -Wl,--no-whole-archive ${YARR_LIB_DEPS} pthread rt)
    endif()
    # Not sure why rt is not linked by default here
endforeach()

install(DIRECTORY ${PROJECT_BINARY_DIR}/bin DESTINATION ${TARGET_INSTALL_AREA}  FILES_MATCHING PATTERN "*" PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
    WORLD_EXECUTE WORLD_READ GROUP_EXECUTE GROUP_READ  )
