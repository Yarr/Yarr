#include "NetioRxCore.h"
#include "NetioTxCore.h"

#include "AllHwControllers.h"
#include "HwController.h"

class NetioController
  : public HwController, public NetioTxCore, public NetioRxCore
{
public:
  NetioController()
  {}

  void loadConfig(json &j) override;
};

void NetioController::loadConfig(json &j) {
  NetioTxCore::fromFileJson(j);
  NetioRxCore::fromFileJson(j);
}

bool netio_registered =
  StdDict::registerHwController("Netio",
      []() { return std::unique_ptr<HwController>(new NetioController); });
