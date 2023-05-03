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

  bool readFelixRegister(const std::string&, uint64_t&);
  bool writeFelixRegister(const std::string&, const std::string&);

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

  FelixClientThread::Reply accessFelixRegister(FelixClientThread::Cmd, const std::vector<std::string>&);
  bool checkReply(const FelixClientThread::Reply&);
};

#endif
