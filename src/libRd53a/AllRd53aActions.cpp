#include "AllStdActions.h"
#include "AllRd53aActions.h"

namespace AllRd53aActionsRegistry {
    using StdDict::registerLoopAction;

    bool trigger_loop_registered = registerLoopAction("Rd53aTriggerLoop",
            []() { return std::unique_ptr<LoopActionBase>(new Rd53aTriggerLoop);});
    bool mask_loop_registered = registerLoopAction("Rd53aMaskLoop",
            []() { return std::unique_ptr<LoopActionBase>(new Rd53aMaskLoop);});
    bool corecol_loop_registered = registerLoopAction("Rd53aCoreColLoop",
            []() { return std::unique_ptr<LoopActionBase>(new Rd53aCoreColLoop);});
    bool parameter_loop_registered = registerLoopAction("Rd53aParameterLoop",
            []() { return std::unique_ptr<LoopActionBase>(new Rd53aParameterLoop);});
}
