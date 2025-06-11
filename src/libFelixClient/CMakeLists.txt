YARR_ADD_FELIX_CLIENT()
add_library(FelixClient SHARED
        FelixController.cpp
        FelixTxCore.cpp
        FelixRxCore.cpp
        FelixTools.cpp
        OptoUtils.cpp
        )

#add_dependencies(FelixClient felixclientthread)
target_include_directories(FelixClient PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(FelixClient PUBLIC Yarr_interface Util_interface felix-client-thread)

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
