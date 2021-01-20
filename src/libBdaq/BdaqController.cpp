#include "AllHwControllers.h"

#include "BdaqController.h"

bool bdaq_registered =
  StdDict::registerHwController("bdaq",
                                []() { return std::unique_ptr<HwController>(new BdaqController); });
