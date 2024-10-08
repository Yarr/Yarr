#ifndef FELIXCONTROLLER_H
#define FELIXCONTROLLER_H

#include "HwController.h"
#include "FelixRxCore.h"
#include "FelixTxCore.h"

#include "felix/felix_client_thread.hpp"

#include "storage.hpp"

class FelixController
  : public HwController, public FelixTxCore, public FelixRxCore
{
public:
  FelixController() = default;

  void loadConfig(json const &j) override;
  const json getStatus() override;

  // E-link control
  bool getICEnable(uint16_t linkId, bool toflx);
  bool getECEnable(uint16_t linkId, bool toflx);
  bool getELinkEnable(unsigned chn, bool toflx);
  bool getELinkEnable(uint64_t fid);
  bool getELinkEnablesAll(const std::vector<unsigned>& chns, bool toflx);
  bool getELinkEnablesAll(const std::vector<uint64_t>& fids);

  bool setICEnable(uint16_t linkId, bool toflx, bool enable=true);
  bool setECEnable(uint16_t linkId, bool toflx, bool enable=true);
  bool setELinkEnable(unsigned chn, bool toflx, bool enable=true);
  bool setELinkEnable(uint64_t fid, bool enable=true);

  bool setELinkEnables(const std::vector<unsigned> chns, bool toflx, const std::vector<bool>& enables);
  bool setELinkEnables(const std::vector<unsigned> chns, bool toflx);
  bool setELinkEnables(const std::vector<uint64_t> fids, const std::vector<bool>& enables);
  bool setELinkEnables(const std::vector<uint64_t> fids);

private:

  std::shared_ptr<FelixClientThread> client;

  // Felix client callbacks
  void on_init() {}

  void on_connect(uint64_t fid) {
    FelixRxCore::on_connect(fid);
  }

  void on_disconnect(uint64_t fid) {
    FelixRxCore::on_disconnect(fid);
  }

  void on_data(uint64_t fid, const uint8_t* data, size_t size, uint8_t status) {
    FelixRxCore::on_data(fid, data, size, status);
  }

  // E-Link control utilities
  void updateEnableMap(std::map<std::string, uint8_t>& maskMap, uint16_t linkId, uint8_t egroup, uint8_t epath, bool toflx, bool val=1);

  bool checkELinkEnableRegs(const std::map<std::string, uint8_t>& maskMap);

  bool setELinkEnableImpl(bool enable, uint16_t linkId, uint8_t egroup, uint8_t epath, bool toflx);

  bool setELinkEnableRegs(const std::map<std::string, uint8_t>& maskMap, const std::map<std::string, uint8_t>& valMap);
};

#endif
