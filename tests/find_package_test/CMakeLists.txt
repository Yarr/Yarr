cmake_minimum_required(VERSION 3.14)
project(YarrFindPackageTest)

find_package(Yarr CONFIG REQUIRED COMPONENTS Yarr Spec Util)

set(CMAKE_BUILD_RPATH_USE_ORIGIN TRUE)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(CMAKE_INSTALL_RPATH "$ORIGIN/../lib")

add_executable(YarrFindPackageTest main.cpp)
target_link_libraries(YarrFindPackageTest PRIVATE Yarr::Yarr Yarr::Spec)

include(GNUInstallDirs)

install(TARGETS YarrFindPackageTest
    EXPORT YarrFindPackageTestTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )

