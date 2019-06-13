cmake_minimum_required (VERSION 3.6)
project (YARR)
set (YARR_VERSION_MAJOR 1)
set (YARR_VERSION_MINOR 0)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 7.3)
        message(FATAL_ERROR "GCC version must be at least 7.3!")
    endif()
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5.0)
        message(FATAL_ERROR "Clang version must be at least 5.0!")
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
add_compile_options(-std=c++17)

# Add -fPIC for (e.g. linking to shared libraries)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)


#add libraries
list(APPEND libs Yarr Spec Util Emu Fe65p2 Fei4 Rce Boc Rd53a KU040 Star)
# TODO add gate
list(APPEND libs Rogue )
# TODO add gate
list(APPEND libs NetioHW )

link_directories(${PROJECT_BINARY_DIR}/../external/src/rogue/lib)
link_directories(${PROJECT_BINARY_DIR}/../lib)

foreach(lib ${libs})
add_definitions( -DNO_PYTHON )
include_directories(lib${lib}/include)
include_directories(${PROJECT_BINARY_DIR}/../external/src/rogue/include)
include_directories(BEFORE SYSTEM ${PROJECT_BINARY_DIR}/../include)
file(GLOB SrcFiles lib${lib}/*.cpp)
list(APPEND LibSrcFiles ${SrcFiles})
endforeach()

add_library(Yarr STATIC ${LibSrcFiles})
add_dependencies(Yarr tbb_2019 felixbase4 netio4 rogue)


#add executables
file(GLOB tools tools/*.cpp)


foreach(target ${tools})
get_filename_component(execname ${target} NAME_WE)
get_filename_component(srcfile ${target} NAME)
add_executable(${execname} tools/${srcfile})
add_dependencies(${execname} Yarr rogue tbb_2019 felixbase4 netio4)
set_target_properties(${execname} PROPERTIES LINKER_LANGUAGE CXX)
link_directories(${PROJECT_BINARY_DIR}/../external/src/rogue/lib)
link_directories(BEFORE SYSTEM ${PROJECT_BINARY_DIR}/../lib)
# Whole-archive means not to drop symbols from Yarr.
# Mainly this is about making sure the auto-registration works.
target_link_libraries(${execname}  -Wl,--whole-archive Yarr tbb -Wl,--no-whole-archive rogue-core netio pthread rt)
# Not sure why rt is not linked by default here
endforeach()

install(DIRECTORY ${PROJECT_BINARY_DIR}/../bin DESTINATION ${TARGET_INSTALL_AREA}  FILES_MATCHING PATTERN "*" PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
                    WORLD_EXECUTE WORLD_READ GROUP_EXECUTE GROUP_READ  )
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/configs  DESTINATION ${SHARED_INSTALL_AREA})
