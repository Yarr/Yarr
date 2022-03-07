#ifndef FELIXRXCORE_H
#define FELIXRXCORE_H

#include "RxCore.h"

#include "felix/felix_client_thread.hpp"
#include "storage.hpp"

class FelixRxCore : virtual public RxCore {

public:

  FelixRxCore();
  ~FelixRxCore() override;

  void setRxEnable(uint32_t val) override;
  void setRxEnable(std::vector<uint32_t> channels) override;
  void disableRx() override;
  void maskRxEnable(uint32_t val, uint32_t mask) override;

  void flushBuffer() override;
  RawData* readData() override;

  uint32_t getDataRate() override;
  uint32_t getCurCount() override;
  bool isBridgeEmpty() override;

protected:

  void writeConfig(json &j);
  void loadConfig(const json &j);
  void setClient(FelixClientThread* client); // set Felix client

private:

  // Owned by FelixController
  FelixClientThread* fclient {nullptr};
};

#endif
