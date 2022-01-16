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
  NetioController() = default;

  void loadConfig(json const &j) override;
};

void NetioController::loadConfig(const json &j) {
  try {
      NetioTxCore::loadConfig(j);
  } catch(std::runtime_error &je) {
    nlog->error("Failed reading NetioTxCore config");
    throw je;
  }

  try {
      NetioRxCore::loadConfig(j);
  } catch(std::runtime_error &je) {
    nlog->error("Failed reading NetioRxCore config");
    throw je;
  }
}

bool netio_registered =
  StdDict::registerHwController("Netio",
      []() { return std::unique_ptr<HwController>(new NetioController); });
