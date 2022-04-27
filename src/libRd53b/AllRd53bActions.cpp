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
    bool par_loop_registered = registerLoopAction("Rd53bParameterLoop",
            []() { return std::unique_ptr<LoopActionBase>(new Rd53bParameterLoop);});
    bool gfb_loop_registered = registerLoopAction("Rd53bGlobalFeedback",
            []() { return std::unique_ptr<LoopActionBase>(new Rd53bGlobalFeedback);});
    bool pixfb_loop_registered = registerLoopAction("Rd53bPixelFeedback",
            []() { return std::unique_ptr<LoopActionBase>(new Rd53bPixelFeedback);});
    bool readreg_loop_registered = registerLoopAction("Rd53bReadRegLoop",
            []() { return std::unique_ptr<LoopActionBase>(new Rd53bReadRegLoop);});			
}
