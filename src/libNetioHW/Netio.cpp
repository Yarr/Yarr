#include "NetioRxCore.h"
#include "NetioTxCore.h"

#include "AllHwControllers.h"
#include "HwController.h"

#include "logging.h"

namespace {
  auto nlog = logging::make_log("Netio::Controller");
}

class NetioController
  : public HwController, public NetioTxCore, public NetioRxCore
{
public:
  NetioController()
  {}

  void loadConfig(json &j) override;
};

void NetioController::loadConfig(json &j) {
  try {
    NetioTxCore::fromFileJson(j);
  } catch(std::runtime_error &je) {
    nlog->error("Failed reading NetioTxCore config");
    throw je;
  }

  try {
    NetioRxCore::fromFileJson(j);
  } catch(std::runtime_error &je) {
    nlog->error("Failed reading NetioRxCore config");
    throw je;
  }
}

bool netio_registered =
  StdDict::registerHwController("Netio",
      []() { return std::unique_ptr<HwController>(new NetioController); });
