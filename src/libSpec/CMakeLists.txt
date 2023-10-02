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
target_include_directories(Spec PRIVATE ../libYarr/include)
target_include_directories(Spec PRIVATE ../libUtil/include)
target_include_directories(Spec PUBLIC ./include)

LIST(APPEND apps
        config-GBT-FMC
        specBenchmarkDma
        specBenchmarkSingle
        specComTest
	switchLPM
        eyeDiagram
        specErrorCheckDma
        specErrorCheckSingle
        specS6programFpga
        specSoftReset)

foreach (target IN LISTS apps)
    add_executable(${target} app/${target}.cpp)
    target_link_libraries(${target} Yarr)
    post_build_debug_executable(${target})
endforeach ()
