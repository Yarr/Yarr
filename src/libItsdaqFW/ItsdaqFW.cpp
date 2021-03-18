#include "ItsdaqRxCore.h"
#include "ItsdaqTxCore.h"

#include "AllHwControllers.h"
#include "HwController.h"

#include "Utils.h"

#include "logging.h"

namespace {
auto logger = logging::make_log("ItsdaqFW::Controller");
}

class ItsdaqFWController
  : public HwController, public ItsdaqTxCore, public ItsdaqRxCore
{
  ItsdaqHandler h;

public:
  ItsdaqFWController()
    : ItsdaqTxCore(h), ItsdaqRxCore(h)
  {
    // Do some initialisation that requires ItsdaqHandler to exist
    ItsdaqRxCore::init();
  }

  void loadConfig(json &j) override;

  const json getStatus() override;
};

void ItsdaqFWController::loadConfig(json &j) {
  logger->debug("Load config from json");
  ItsdaqTxCore::fromFileJson(j);
  ItsdaqRxCore::fromFileJson(j);
}

const json ItsdaqFWController::getStatus() {
  logger->debug("getStatus");
  json j_status;

  // Status from different clock domains
  auto &status = h.LatestStatus();
  auto &sys_status = h.LatestSysStatus();

  // Sometimes it changes, so record total length
  j_status["status_length"] = std::to_string(status.size());

  if(status.empty()) {
    // Bail early
    return j_status;
  }

  j_status["firmware_version"] = Utils::hexify(status[1]);

  if(sys_status.empty()) {
    // Bail early
    return j_status;
  }

  uint16_t raw_temp = sys_status[1];
  j_status["raw_temp"] = raw_temp;
  if(raw_temp != 0) {
    float t = (((raw_temp>>4) & 0xfff) - 2213) / 8.125;
    j_status["temp"] = t;
  }

  return j_status;
}

bool itsdaq_fw_registered =
  StdDict::registerHwController("Itsdaq",
      []() { return std::unique_ptr<HwController>(new ItsdaqFWController); });
