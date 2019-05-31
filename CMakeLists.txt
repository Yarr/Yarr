cmake_minimum_required (VERSION 3.6)
include(ExternalProject)
if(CMAKE_TOOLCHAIN_FILE) 
set(TOOLCHAIN "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}")
else()
set(TOOLCHAIN "")
endif()

set(TARGET_INSTALL_AREA ${PROJECT_SOURCE_DIR}/installed/${TARGET_ARCH})
set(SHARED_INSTALL_AREA ${PROJECT_SOURCE_DIR}/installed/)
add_compile_options(-std=c++17)

if ("${TARGET_ARCH}" STREQUAL "arm")
  set(TBB_ARCH "arch=arm")
  else()
    if ("${TARGET_ARCH}" STREQUAL "arm64")
      set(TBB_ARCH "arch=arm64")
  else()
     set(TBB_ARCH "")
endif()
endif()
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
set(TBB_COMP compiler=clang)
else()
set(TBB_COMP CXX=${CMAKE_CXX_COMPILER})
endif()
ExternalProject_Add (
tbb_2019
GIT_REPOSITORY https://github.com/intel/tbb.git
GIT_TAG tbb_2019
UPDATE_COMMAND ""
INSTALL_COMMAND ${PROJECT_SOURCE_DIR}/scripts/install_tbb.sh ${CMAKE_CURRENT_BINARY_DIR}  ${CMAKE_CURRENT_BINARY_DIR}  
BUILD_COMMAND cd ${CMAKE_CURRENT_BINARY_DIR}/external/src/tbb_2019 &&  make -j4 ${TBB_COMP} ${TBB_ARCH} extra_inc=big_iron.inc
CONFIGURE_COMMAND ""
PREFIX "${CMAKE_CURRENT_BINARY_DIR}/external"
)

ExternalProject_Add (
      netio4
      GIT_REPOSITORY https://:@gitlab.cern.ch:8443/atlas-tdaq-felix/netio.git
      GIT_TAG felix-4.0.x
      UPDATE_COMMAND ""
      PATCH_COMMAND cp ${PROJECT_SOURCE_DIR}/cmake/CMakeLists.txt.netio ${CMAKE_CURRENT_BINARY_DIR}/external/src/netio4/CMakeLists.txt
      INSTALL_COMMAND ""
      PREFIX "${CMAKE_CURRENT_BINARY_DIR}/external"
      CMAKE_ARGS "${TOOLCHAIN}"
      DEPENDS tbb_2019
      )
ExternalProject_Add (
      felixbase4
      GIT_REPOSITORY https://:@gitlab.cern.ch:8443/atlas-tdaq-felix/felixbase.git
      GIT_TAG felix-4.0.x
      UPDATE_COMMAND ""
      PATCH_COMMAND ""
      INSTALL_COMMAND ""
      CONFIGURE_COMMAND ""
      BUILD_COMMAND ""
      PREFIX "${CMAKE_CURRENT_BINARY_DIR}/external"
      DEPENDS netio4
      )
ExternalProject_Add (
      rogue
      GIT_REPOSITORY https://github.com/slaclab/rogue.git
      GIT_TAG atlas-fixes
      UPDATE_COMMAND ""
      INSTALL_COMMAND make install
      PREFIX "${CMAKE_CURRENT_BINARY_DIR}/external"
      CMAKE_ARGS "-DNO_PYTHON=1" "-DNO_ZMQ=1" "-DNO_BZIP=1" "-DSTATIC_LIB=1" "${TOOLCHAIN}"
      )

include_directories(${CMAKE_CURRENT_BINARY_DIR}/external/src/felixbase4)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/external/src/netio4)
link_directories(${CMAKE_CURRENT_BINARY_DIR}/external/src/netio4-build)
add_subdirectory(src)

