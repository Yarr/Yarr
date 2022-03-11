add_library(KU040 SHARED
        IPbus.cpp
        KU040Controller.cpp
        KU040RxCore.cpp
        KU040TxCore.cpp
        )
add_dependencies(KU040 variant)
post_build_debug_library(KU040)
target_include_directories(KU040 PRIVATE ../libYarr/include)
target_include_directories(KU040 PRIVATE ../libUtil/include)
target_include_directories(KU040 PUBLIC ./include)
