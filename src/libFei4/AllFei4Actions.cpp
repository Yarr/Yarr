#include "AllStdActions.h"

#include "AllFei4Actions.h"

namespace AllFei4ActionsRegistry {
  using StdDict::registerLoopAction;

  bool dc_loop_registered =
    registerLoopAction("Fei4DcLoop",
                       []() { return std::unique_ptr<LoopActionBase>(new Fei4DcLoop); });
  bool mask_loop_registered =
    registerLoopAction("Fei4MaskLoop",
                       []() { return std::unique_ptr<LoopActionBase>(new Fei4MaskLoop); });
  bool trigger_loop_registered =
    registerLoopAction("Fei4TriggerLoop",
                       []() { return std::unique_ptr<LoopActionBase>(new Fei4TriggerLoop); });
}
