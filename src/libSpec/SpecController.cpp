#include "AllHwControllers.h"

#include "SpecController.h"

bool spec_registered =
  StdDict::registerHwController("spec",
                                []() { return std::unique_ptr<HwController>(new SpecController); });
