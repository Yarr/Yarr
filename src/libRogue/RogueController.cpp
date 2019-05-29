#include "RogueController.h"

#include "AllHwControllers.h"

bool rogue_registered =
  StdDict::registerHwController("rogue",
                                []() { return std::unique_ptr<HwController>(new RogueController); });

RogueController::~RogueController() {
  std::shared_ptr<RogueCom> com=RogueCom::getInstance();
  com->printStats();
}

void RogueController::loadConfig(json &j) {
  std::shared_ptr<RogueCom> com=RogueCom::getInstance();
  std::string conn=j["connection"];
  unsigned  enableMask=j["enableMask"];
  uint32_t port=j["port"];  

  com->connect(conn,port);
  com->enableLane(enableMask);
  com->enableDebugStream(false);

  if(j.find("batchTimer") != j.end()) {
    uint32_t batchTimer=j["batchTimer"];  
    com->setBatchTimer(batchTimer);
  }
  if(j.find("batchSize") != j.end()) {
    uint32_t batchSize=j["batchSize"];
    com->setBatchSize(batchSize);
  }
  bool firmwareTrigger=false;
  if(j.find("firmwareTrigger") != j.end()) {
    firmwareTrigger=j["firmwareTrigger"];
  }   
  com->setFirmwareTrigger(firmwareTrigger);
  
}
