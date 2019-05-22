cmake_minimum_required (VERSION 3.6)
include(ExternalProject)
if(CMAKE_TOOLCHAIN_FILE) 
set(TOOLCHAIN "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}")
else()
set(TOOLCHAIN "")
endif()
ExternalProject_Add (
      rogue
      GIT_REPOSITORY https://github.com/slaclab/rogue.git
      GIT_TAG atlas-fixes
      UPDATE_COMMAND ""
      INSTALL_COMMAND make install
      PREFIX "${CMAKE_CURRENT_BINARY_DIR}/external"
      CMAKE_ARGS "-DNO_PYTHON=1" "-DNO_ZMQ=1" "-DNO_BZIP=1" "-DSTATIC_LIB=1" "${TOOLCHAIN}"
      )
add_subdirectory(src)
