#include "AllStdActions.h"

#include "AllStarActions.h"

namespace AllStarActionsRegistry {
  using StdDict::registerLoopAction;

  bool trigger_loop_registered = registerLoopAction("StarTriggerLoop",
                       []() { return std::unique_ptr<LoopActionBase>(new StarTriggerLoop); });

}


