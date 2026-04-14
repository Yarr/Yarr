# Set -DYARR_PROJECT_DIR=/path/to/yarr-src

cmake_minimum_required(VERSION 3.14)

project(YarrExternalProjectSuperbuildTest)

set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "Install path for the superbuild")
set(YARR_PROJECT_DIR "" CACHE PATH "Path to local Yarr source directory")
set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Choose the type of build")

include(GNUInstallDirs)
include(ExternalProject)

# Directory definitions
set(YARR_INSTALL_DIR ${CMAKE_INSTALL_PREFIX})
set(CONSUMER_INSTALL_DIR ${CMAKE_INSTALL_PREFIX})

# 1. Build & Install Yarr
ExternalProject_Add(Yarr
  SOURCE_DIR "${YARR_PROJECT_DIR}"  
  CMAKE_ARGS
    -DCMAKE_INSTALL_PREFIX=${YARR_INSTALL_DIR}
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
  INSTALL_COMMAND ${CMAKE_COMMAND} --install . --prefix ${YARR_INSTALL_DIR}
)

# 2. Build & Install Consumer, depends on Yarr
ExternalProject_Add(YarrConsumerTest
  DEPENDS Yarr
  SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/consumer"
  CMAKE_ARGS
    -DYarr_DIR=${YARR_INSTALL_DIR}/cmake/Yarr
    -DCMAKE_INSTALL_PREFIX=${CONSUMER_INSTALL_DIR}
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    -DCMAKE_INSTALL_RPATH=\$ORIGIN/../${CMAKE_INSTALL_LIBDIR}
  INSTALL_COMMAND ${CMAKE_COMMAND} --install . --prefix ${CONSUMER_INSTALL_DIR}
)



