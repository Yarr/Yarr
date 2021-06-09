#include "AllStdActions.h"

#include "AllStarActions.h"

namespace AllStarActionsRegistry {
  using StdDict::registerLoopAction;

  bool trigger_loop_registered = registerLoopAction("StarTriggerLoop",
                       []() { return std::unique_ptr<LoopActionBase>(new StarTriggerLoop); });

  bool register_loop_registered = registerLoopAction("StarRegDump",
                       []() { return std::unique_ptr<LoopActionBase>(new StarRegDump); });

  bool mask_loop_registered = registerLoopAction("StarMaskLoop",
                       []() { return std::unique_ptr<LoopActionBase>(new StarMaskLoop); });
}


