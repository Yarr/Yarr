add_library(Rd53a SHARED
        AllRd53aActions.cpp
        Rd53a2TriggerLoop.cpp
        Rd53aCfg.cpp
        Rd53aCmd.cpp
        Rd53aCoreColLoop.cpp
        Rd53a.cpp
        Rd53aDataProcessor.cpp
        Rd53aGlobalCfg.cpp
        Rd53aGlobalFeedback.cpp
        Rd53aMaskLoop.cpp
        Rd53aParameterLoop.cpp
        Rd53aPixelCfg.cpp
        Rd53aPixelFeedback.cpp
        Rd53aReadRegLoop.cpp
        Rd53aTriggerLoop.cpp
        Rd53aTwoParameterLoop.cpp)

post_build_debug_library(Rd53a)
target_include_directories(Rd53a PRIVATE ../libYarr/include)
target_include_directories(Rd53a PRIVATE ../libUtil/include)
target_include_directories(Rd53a PUBLIC ./include)
target_link_libraries(Rd53a Scan)

