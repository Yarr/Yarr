#include "RceController.h"

#include "AllHwControllers.h"

bool rce_registered =
  StdDict::registerHwController("rce",
                                []() { return std::unique_ptr<HwController>(new RceController); });

void RceController::loadConfig(json &j) {

}
