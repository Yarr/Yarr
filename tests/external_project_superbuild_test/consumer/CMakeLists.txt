cmake_minimum_required(VERSION 3.14)
project(YarrConsumerTest)

# Standard install rule
include(GNUInstallDirs)

# Find the installed Yarr package
find_package(Yarr REQUIRED COMPONENTS Yarr Spec)

add_executable(YarrConsumerTest main.cpp)

# Link against the installed targets
target_link_libraries(YarrConsumerTest PRIVATE Yarr::Yarr Yarr::Spec)

# Ensure RPATH is set correctly for running from install tree
set_target_properties(YarrConsumerTest PROPERTIES
  BUILD_RPATH_USE_ORIGIN TRUE
  INSTALL_RPATH_USE_LINK_PATH TRUE
  INSTALL_RPATH "$ORIGIN/../${CMAKE_INSTALL_LIBDIR}"
)

install(TARGETS YarrConsumerTest
  EXPORT YarrConsumerTestTargets
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

