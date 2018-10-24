#include "NetioRxCore.h"
#include "NetioTxCore.h"

#include "AllHwControllers.h"
#include "HwController.h"

class NetioController
  : public HwController, public NetioTxCore, public NetioRxCore
{
  NetioHandler m_nioh;
public:
  NetioController()
    : m_nioh("posix", "localhost", 12340, 12345, 50000000, true),
      NetioRxCore(m_nioh)
  {
  }

  void loadConfig(json &j) override;
};

void NetioController::loadConfig(json &j) {
  NetioTxCore::fromFileJson(j);
  NetioRxCore::fromFileJson(j);
}

bool netio_registered =
  StdDict::registerHwController("Netio",
      []() { return std::unique_ptr<HwController>(new NetioController); });
