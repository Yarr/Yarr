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
  bool parameter_loop_registered =
    registerLoopAction("Fei4ParameterLoop",
                       []() { return std::unique_ptr<LoopActionBase>(new Fei4ParameterLoop); });
  bool trigger_loop_registered =
    registerLoopAction("Fei4TriggerLoop",
                       []() { return std::unique_ptr<LoopActionBase>(new Fei4TriggerLoop); });
  bool global_feedback_registered =
    registerLoopAction("Fei4GlobalFeedback",
                       []() { return std::unique_ptr<LoopActionBase>(new Fei4GlobalFeedback); });
  bool pixel_feedback_registered =
    registerLoopAction("Fei4PixelFeedback",
                       []() { return std::unique_ptr<LoopActionBase>(new Fei4PixelFeedback); });

}

// These classes have logger registration in header files, here we instantiate
// them so the loggers are registered at static init (ie before they're likely
// to need to be configured)
void logger_static_init_fei4() {
  (void)Fei4PixelFeedback::logger();
  (void)Fei4GlobalFeedback::logger();
  (void)Fei4ParameterLoop::logger();
}

bool fei4_header_loggers_registered = [](){ logger_static_init_fei4(); return true; }();
