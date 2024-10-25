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
  // IC
  bool getICEnable(uint64_t fid);
  bool getICEnable(const std::vector<uint64_t>& fids, bool exclusive);

  bool setICEnable(uint64_t fid, bool enable=true);
  bool setICEnable(const std::vector<uint64_t>& fids, const std::vector<bool>& enables, bool exclusive);
  inline bool setICEnable(const std::vector<uint64_t>& fids, bool exclusive) {
    std::vector<bool> enables(fids.size(), true);
    return setICEnable(fids, enables, exclusive);
  }

  bool disableAllICs(bool toflx);

  // EC
  bool getECEnable(uint64_t fid);
  bool getECEnable(const std::vector<uint64_t>& fids, bool exclusive);

  bool setECEnable(uint64_t fid, bool enable=true);
  bool setECEnable(const std::vector<uint64_t>& fids, const std::vector<bool>& enables, bool exclusive);
  inline bool setECEnable(const std::vector<uint64_t>& fids, bool exclusive) {
    std::vector<bool> enables(fids.size(), true);
    return setECEnable(fids, enables, exclusive);
  }

  bool disableAllECs(bool toflx);

  // Data E-links
  bool getELinkEnable(uint64_t fid);
  bool getELinkEnable(const std::vector<uint64_t>& fids, bool exclusive);
  unsigned getELinkWidthNBits(uint64_t fid);
  unsigned getELinkWidthMbps(uint64_t fid);

  bool setELinkEnable(uint64_t fid, bool enable=true);
  bool setELinkEnable(const std::vector<uint64_t>& fids, const std::vector<bool>& enables, bool exclusive);
  inline bool setELinkEnable(const std::vector<uint64_t>& fids, bool exclusive) {
    std::vector<bool> enables(fids.size(), true);
    return setELinkEnable(fids, enables, exclusive);
  }

  bool disableAllELinks(bool toflx);

  bool setELinkWidthNBits(uint64_t fid, unsigned nbits);
  bool setELinkWidthNBits(const std::vector<uint64_t>& fids, unsigned nbits);
  bool setELinkWidthMbps(uint64_t fid, unsigned bandwidth);
  bool setELinkWidthMbps(const std::vector<uint64_t>& fids, unsigned bandwidth);

protected:

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
  void updateRegMap(std::map<std::string, unsigned>& regMap, const std::string& regName, unsigned value, bool overwrite);

  bool checkRegValue(const std::string& regName, unsigned value, unsigned mask=0);
  bool checkRegValuesAll(const std::map<std::string, unsigned>& regValueMap, const std::map<std::string, unsigned>& regMaskMap);
  bool checkRegValuesAll(const std::map<std::string, unsigned>& regValueMap, unsigned mask=0);

  bool setRegValue(const std::string& regName, unsigned value, unsigned mask=0);
  bool setRegValueAll(const std::map<std::string, unsigned>& regValueMap, const std::map<std::string, unsigned>& regMaskMap);
  bool setRegValueAll(const std::map<std::string, unsigned>& regValueMap, unsigned mask=0);

  void initAllICEnableRegMap(std::map<std::string, unsigned>& regMap, bool toflx, bool tohost);
  void initAllECEnableRegMap(std::map<std::string, unsigned>& regMap, bool toflx, bool tohost);
  void initAllELinkEnableRegMap(std::map<std::string, unsigned>& regMap, bool toflx, bool tohost);
};

#endif
