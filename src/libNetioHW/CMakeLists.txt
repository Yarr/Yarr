if (LSB_RELEASE_ID_SHORT MATCHES "AlmaLinux 9")
    # Get netio and felixbase from prebuilt library
    YARR_ADD_NETIO_FELIXBASE_PREBUILT()
    add_library(NetioHW SHARED
        Netio.cpp
        NetioHandler.cpp
        NetioRxCore.cpp
        NetioTxCore.cpp
        )
    target_include_directories(NetioHW PUBLIC "${FELIX_INCLUDE}")
    target_link_libraries(NetioHW PUBLIC "${NETIO_LIB}" "${FELIXBASE_LIB}")
else()
    # Get netio and felixbase from source
    YARR_ADD_NETIO()
    YARR_ADD_FELIX()
    add_library(NetioHW SHARED
        Netio.cpp
        NetioHandler.cpp
        NetioRxCore.cpp
        NetioTxCore.cpp
        )
    add_dependencies(NetioHW felixbase4 netio4 tbb_2020)
    target_link_libraries(NetioHW PRIVATE netio tbb)
endif()

post_build_debug_library(NetioHW)
target_include_directories(NetioHW PRIVATE ../libYarr/include)
target_include_directories(NetioHW PRIVATE ../libUtil/include)
target_include_directories(NetioHW PUBLIC ./include)

LIST(APPEND apps testNetioHw netio_bridge)
foreach (target IN LISTS apps)
    add_executable(${target} app/${target}.cpp)
    target_link_libraries(${target} Yarr)
    post_build_debug_executable(${target})
endforeach ()
