#include "SimpleNetioRxCore.h"
#include "SimpleNetioTxCore.h"

#include "AllHwControllers.h"
#include "HwController.h"

class SimpleNetioController
  : public HwController, public SimpleNetioTxCore, public SimpleNetioRxCore
{
public:
  void loadConfig(json &j) override;
};

void SimpleNetioController::loadConfig(json &j) {
  SimpleNetioTxCore::fromFileJson(j);
  SimpleNetioRxCore::fromFileJson(j);
}

bool simple_netio_registered =
  StdDict::registerHwController("SimpleNetio",
      []() { return std::unique_ptr<HwController>(new SimpleNetioController); });
