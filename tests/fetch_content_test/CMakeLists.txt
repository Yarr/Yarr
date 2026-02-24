# Set -DYARR_PROJECT_DIR=<path> to a local Yarr source directory for the CI test
cmake_minimum_required(VERSION 3.14)
project(YarrFetchContentTest)

include(FetchContent)
include(GNUInstallDirs)

# Pass in the local Yarr source dir
set(YARR_PROJECT_DIR "" CACHE PATH "Path to Yarr source (used with FetchContent)")

if(NOT IS_DIRECTORY "${YARR_PROJECT_DIR}")
    message(FATAL_ERROR "You must provide -DYARR_SOURCE_DIR=<path> to a local Yarr source directory")
endif()

# FetchContent: make Yarr part of the current build
FetchContent_Declare(
  Yarr
  SOURCE_DIR ${YARR_PROJECT_DIR}
)

FetchContent_MakeAvailable(Yarr)

# Your test binary
add_executable(YarrFetchContentTest main.cpp)
target_link_libraries(YarrFetchContentTest PRIVATE Yarr Spec)

set_target_properties(YarrFetchContentTest PROPERTIES
    BUILD_RPATH "\$ORIGIN/../${CMAKE_INSTALL_LIBDIR}"
    INSTALL_RPATH "\$ORIGIN/../${CMAKE_INSTALL_LIBDIR}"
)

# Optional: install test binary and yarr
install(TARGETS YarrFetchContentTest
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

