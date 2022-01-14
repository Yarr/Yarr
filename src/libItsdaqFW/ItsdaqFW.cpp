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
    // NB initialisation is only done in loadConfig
  }

  void loadConfig(json &j) override;

  const json getStatus() override;
};

void ItsdaqFWController::loadConfig(json &j) {
  logger->debug("Load config from json");

  // Default if all switches off
  std::string remote = "192.168.222.16";
  if(j.contains("remote")) {
    remote = j["remote"];
  }

  int rPort = 60000;
  if(j.contains("localPort")) {
    rPort = j["remotePort"];
  }

  int lPort;

  if(!j.contains("localPort")) {
    // Pick automatically
    lPort = 0;
  } else {
    lPort = j["localPort"];
  }

  uint32_t remoteIp;

  // Parse: 192.168.222.22 to 0x16dea8c0
  for(int i=0; i<4; i++) {
    auto f = remote.find(".");
    if(i<3 && f == std::string::npos) {
      throw std::runtime_error("Can't parse IP address from ItsdaqFW config");
    }
    auto segment = remote.substr(0, f);
    if(segment.empty() || segment.size() > 3) {
      throw std::runtime_error("Can't parse segment of IP address from ItsdaqFW config");
    }
    remoteIp >>= 8;
    remoteIp |= (std::atoi(segment.c_str()) << 24);

    if(i < 3) remote = remote.substr(f+1);
  }

  h.reconfigure(remoteIp, lPort, rPort);

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
