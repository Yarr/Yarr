YARR_ADD_FELIX_CLIENT()
add_library(FelixClient SHARED
        FelixController.cpp
        FelixTxCore.cpp
        FelixRxCore.cpp
        FelixTools.cpp
        )

add_dependencies(FelixClient felix_client_thread)
target_include_directories(FelixClient PRIVATE ../libYarr/include)
target_include_directories(FelixClient PRIVATE ../libUtil/include)
target_include_directories(FelixClient PUBLIC ./include)
target_link_libraries(FelixClient PRIVATE felix-client-thread)

LIST(APPEND apps testFelixClient felixRegister)
foreach (target IN LISTS apps)
    add_executable(${target} app/${target}.cpp)
    target_link_libraries(${target} Yarr)
endforeach ()
