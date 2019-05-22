cmake_minimum_required (VERSION 3.6)
include(ExternalProject)
ExternalProject_Add (
      rogue
      GIT_REPOSITORY https://github.com/slaclab/rogue.git
      GIT_TAG atlas-fixes
      UPDATE_COMMAND ""
      INSTALL_COMMAND make install
      PREFIX "${CMAKE_CURRENT_BINARY_DIR}/external"
      CMAKE_ARGS "-DNO_PYTHON=1" "-DNO_ZMQ=1" "-DNO_BZIP=1" "-DSTATIC_LIB=1"
      )
add_subdirectory(src)
