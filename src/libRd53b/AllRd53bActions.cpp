#include "AllStdActions.h"
#include "AllRd53bActions.h"

namespace AllRd53bActionsRegistry {
    using StdDict::registerLoopAction;

    bool trigger_loop_registered = registerLoopAction("Rd53bTriggerLoop",
            []() { return std::unique_ptr<LoopActionBase>(new Rd53bTriggerLoop);});
    bool mask_loop_registered = registerLoopAction("Rd53bMaskLoop",
            []() { return std::unique_ptr<LoopActionBase>(new Rd53bMaskLoop);});
    bool corecol_loop_registered = registerLoopAction("Rd53bCoreColLoop",
            []() { return std::unique_ptr<LoopActionBase>(new Rd53bCoreColLoop);});
}
