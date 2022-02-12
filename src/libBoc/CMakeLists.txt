add_Library(Boc SHARED
        BocCom.cpp
        BocController.cpp
        BocRxCore.cpp
        BocTxCore.cpp
        )

add_dependencies(Boc variant)
target_include_directories(Boc PRIVATE ../libYarr/include)
target_include_directories(Boc PRIVATE ../libUtil/include)
target_include_directories(Boc PUBLIC ./include)
