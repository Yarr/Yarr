#include "ItsdaqRxCore.h"
#include "ItsdaqTxCore.h"

#include "AllHwControllers.h"
#include "HwController.h"

class ItsdaqFWController
  : public HwController, public ItsdaqTxCore, public ItsdaqRxCore
{
  ItsdaqHandler h;

public:
  ItsdaqFWController()
    : ItsdaqTxCore(h), ItsdaqRxCore(h)
  {}

  void loadConfig(json &j) override;
};

void ItsdaqFWController::loadConfig(json &j) {
  ItsdaqTxCore::fromFileJson(j);
  ItsdaqRxCore::fromFileJson(j);
}

bool itsdaq_fw_registered =
  StdDict::registerHwController("Itsdaq",
      []() { return std::unique_ptr<HwController>(new ItsdaqFWController); });
