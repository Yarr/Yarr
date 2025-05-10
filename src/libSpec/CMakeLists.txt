add_library(Spec SHARED
        AD7995.cpp
        BenchmarkTools.cpp
        Exception.cpp
        KernelMemory.cpp
        LMK61E2.cpp
        PeripheralI2C.cpp
        Si534x.cpp
        SimpleI2C.cpp
        SpecCom.cpp
        SpecController.cpp
        SpecDevice.cpp
        SpecRxCore.cpp
        SpecTxCore.cpp
        UserMemory.cpp)

post_build_debug_library(Spec)
target_include_directories(Spec PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(Spec PRIVATE Util_interface Yarr_interface)

LIST(APPEND apps
	specReadWriteReg
        config-GBT-FMC
        specBenchmarkDma
        specBenchmarkSingle
        specComTest
        switchLPM
        eyeDiagram
        dataMergingCheck
        specErrorCheckDma
        specErrorCheckSingle
        specSoftReset)

foreach (target IN LISTS apps)
    add_executable(${target} app/${target}.cpp)
    target_link_libraries(${target} Yarr)
    post_build_debug_executable(${target})
endforeach ()

install(TARGETS Spec ${apps}
    EXPORT Yarr_SpecTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}  # For DLLs on Windows
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )

install(EXPORT Yarr_SpecTargets
  NAMESPACE Yarr::
  DESTINATION ${CMAKE_CONFIG_INSTALL_PATH}
)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING
        PATTERN "*.h"
)
