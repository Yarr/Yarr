cmake_minimum_required(VERSION 3.14)
project(YarrAddSubdirectoryTest)

add_subdirectory(${CMAKE_SOURCE_DIR}/../../ /tmp/build)

set(CMAKE_BUILD_RPATH_USE_ORIGIN TRUE)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(CMAKE_INSTALL_RPATH "$ORIGIN/../lib")

add_executable(YarrAddSubdirectoryTest main.cpp)
target_link_libraries(YarrAddSubdirectoryTest Yarr Spec)

include(GNUInstallDirs)

install(TARGETS YarrAddSubdirectoryTest
    EXPORT YarrAddSubdirectoryTestTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )

