#include "AllStdActions.h"
#include "AllRd53aActions.h"

namespace AllRd53aActionsRegistry {
    using StdDict::registerLoopAction;

    bool trigger_loop_registered = registerLoopAction("Rd53aTriggerLoop",
            []() { return std::unique_ptr<LoopActionBase>(new Rd53aTriggerLoop);});
    bool two_trigger_loop_registered = registerLoopAction("Rd53a2TriggerLoop",
            []() { return std::unique_ptr<LoopActionBase>(new Rd53a2TriggerLoop);});
    bool mask_loop_registered = registerLoopAction("Rd53aMaskLoop",
            []() { return std::unique_ptr<LoopActionBase>(new Rd53aMaskLoop);});
    bool corecol_loop_registered = registerLoopAction("Rd53aCoreColLoop",
            []() { return std::unique_ptr<LoopActionBase>(new Rd53aCoreColLoop);});
    bool parameter_loop_registered = registerLoopAction("Rd53aParameterLoop",
            []() { return std::unique_ptr<LoopActionBase>(new Rd53aParameterLoop);});
    bool two_parameter_loop_registered = registerLoopAction("Rd53aTwoParameterLoop",
            []() { return std::unique_ptr<LoopActionBase>(new Rd53aTwoParameterLoop);});
    bool globalfb_loop_registered = registerLoopAction("Rd53aGlobalFeedback",
            []() { return std::unique_ptr<LoopActionBase>(new Rd53aGlobalFeedback);});
    bool pixelfb_loop_registered = registerLoopAction("Rd53aPixelFeedback",
            []() { return std::unique_ptr<LoopActionBase>(new Rd53aPixelFeedback);});
    bool readreg_loop_registered = registerLoopAction("Rd53aReadRegLoop",
            []() { return std::unique_ptr<LoopActionBase>(new Rd53aReadRegLoop);});


}
