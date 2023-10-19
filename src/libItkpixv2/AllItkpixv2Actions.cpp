#include "AllStdActions.h"
#include "AllItkpixv2Actions.h"

namespace AllItkpixv2ActionsRegistry {
    using StdDict::registerLoopAction;

    bool trigger_loop_registered = registerLoopAction("Itkpixv2TriggerLoop",
            []() { return std::unique_ptr<LoopActionBase>(new Itkpixv2TriggerLoop);});
    bool mask_loop_registered = registerLoopAction("Itkpixv2MaskLoop",
            []() { return std::unique_ptr<LoopActionBase>(new Itkpixv2MaskLoop);});
    bool par_mask_loop_registered = registerLoopAction("Itkpixv2ParMaskLoop",
            []() { return std::unique_ptr<LoopActionBase>(new Itkpixv2ParMaskLoop);});
    bool corecol_loop_registered = registerLoopAction("Itkpixv2CoreColLoop",
            []() { return std::unique_ptr<LoopActionBase>(new Itkpixv2CoreColLoop);});
    bool par_loop_registered = registerLoopAction("Itkpixv2ParameterLoop",
            []() { return std::unique_ptr<LoopActionBase>(new Itkpixv2ParameterLoop);});
    bool gfb_loop_registered = registerLoopAction("Itkpixv2GlobalFeedback",
            []() { return std::unique_ptr<LoopActionBase>(new Itkpixv2GlobalFeedback);});
    bool pixfb_loop_registered = registerLoopAction("Itkpixv2PixelFeedback",
            []() { return std::unique_ptr<LoopActionBase>(new Itkpixv2PixelFeedback);});
    bool readreg_loop_registered = registerLoopAction("Itkpixv2ReadRegLoop",
            []() { return std::unique_ptr<LoopActionBase>(new Itkpixv2ReadRegLoop);});			
}
