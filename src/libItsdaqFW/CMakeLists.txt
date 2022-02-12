add_library(ItsdaqFW SHARED
        Checksum.cpp
        ItsdaqFW.cpp
        ItsdaqHandler.cpp
        ItsdaqRxCore.cpp
        ItsdaqTxCore.cpp
        UdpSocket.cpp
        )
add_dependencies(ItsdaqFW variant)
target_include_directories(ItsdaqFW PRIVATE ../libYarr/include)
target_include_directories(ItsdaqFW PRIVATE ../libUtil/include)
target_include_directories(ItsdaqFW PUBLIC ./include)