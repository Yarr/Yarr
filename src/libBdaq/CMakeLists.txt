find_package(Boost REQUIRED system)
add_library(Bdaq SHARED
        BdaqAuroraRx.cpp
        BdaqController.cpp
        Bdaq.cpp
        BdaqDriver.cpp
        BdaqGPIO.cpp
        Bdaq_i2c.cpp
        BdaqRBCP.cpp
        BdaqRegister.cpp
        BdaqRegisterProxy.cpp
        BdaqRxCore.cpp
        BdaqSi570.cpp
        BdaqSiTcpFifo.cpp
        BdaqTCP.cpp
        BdaqTxCore.cpp)

add_dependencies(Bdaq variant)
target_include_directories(Bdaq PRIVATE ../libYarr/include)
target_include_directories(Bdaq PRIVATE ../libUtil/include)
target_include_directories(Bdaq PUBLIC ./include)

target_link_libraries(Bdaq Boost::system)
