#include "AllStdActions.h"

#include "AllStarActions.h"

namespace AllStarActionsRegistry {
  using StdDict::registerLoopAction;

  bool trigger_loop_registered = registerLoopAction("StarTriggerLoop",
                       []() { return std::unique_ptr<LoopActionBase>(new StarTriggerLoop); });

  bool mask_loop_registered = registerLoopAction("StarMaskLoop",
                       []() { return std::unique_ptr<LoopActionBase>(new StarMaskLoop); });

  bool counter_loop_registered = registerLoopAction("StarCounterLoop",
                       []() { return std::unique_ptr<LoopActionBase>(new StarCounterLoop); });

}


