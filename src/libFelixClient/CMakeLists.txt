YARR_ADD_FELIX_CLIENT()
add_library(FelixClient SHARED
        FelixController.cpp
        FelixDirectMode.cpp
        FelixTxCore.cpp
        FelixRxCore.cpp
        FelixTools.cpp
        FelixRxThread.cpp
        OptoUtils.cpp
        )

target_compile_options(
    FelixClient
  PRIVATE
    $<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang>:-Wall>
)

if(YARR_FELIX_BACKEND STREQUAL "FELIX_PROXY")
  message(STATUS "Using felix-proxy for access to FelixClientThread")
  target_compile_definitions(FelixClient PUBLIC YARR_CONFIG_FELIX_PROXY=1)

  set(YARR_FELIX_LIB tdaq::felix_proxy tdaq::felix-interface)
else()
  set(YARR_FELIX_LIB felix-client-thread)
endif()

#add_dependencies(FelixClient felixclientthread)
target_include_directories(FelixClient PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(FelixClient PUBLIC Yarr_interface Util_interface
  ${YARR_FELIX_LIB}
  )

LIST(APPEND apps readWriteLpGBTRegister testFelixClient felixRegister elinkConfig)
foreach (target IN LISTS apps)
    add_executable(${target} app/${target}.cpp)
    target_link_libraries(${target} Yarr FelixClient)
    post_build_debug_executable(${target})
endforeach ()

if (BUILD_TESTS)
    add_subdirectory(tests)
endif()

install(TARGETS FelixClient ${apps}
    EXPORT Yarr_FelixClientTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}  # For DLLs on Windows
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )

install(EXPORT Yarr_FelixClientTargets
  NAMESPACE Yarr::
  DESTINATION ${CMAKE_CONFIG_INSTALL_PATH}/Yarr
)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING
        PATTERN "*.h"
)
