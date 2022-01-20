#include "RogueController.h"

#include "AllHwControllers.h"

bool rogue_registered =
  StdDict::registerHwController("rogue",
                                []() { return std::unique_ptr<HwController>(new RogueController); });

RogueController::~RogueController() {
  std::shared_ptr<RogueCom> com=RogueCom::getInstance();
  com->printStats();
}

void RogueController::loadConfig(const json &j) {
  std::shared_ptr<RogueCom> com=RogueCom::getInstance();
  std::string conn=j["connection"];
  unsigned  enableMask=j["enableMask"];

  if(j.find("type") != j.end()) {
    uint32_t type=j["type"];
    com->setType(type);
  }else{
    com->setType(0);//if no type, then, set to 0 for FEB board
  }
 
  com->connect(conn);
  com->enableLane(enableMask);
  //reset FW didn't help
  //com->resetFirmware();


  if(j.find("enableDebug") != j.end()) {
    bool enableDebug=j["enableDebug"];
    com->enableDebugStream(enableDebug);
  }else{
	com->enableDebugStream(true);//enable by default
  }
 
  //0: speed 1.28Gbps; 1: 0.64Gbps; 2: 0.32Gbps; 3: 0.16Gbps;
  if(j.find("selectRate") != j.end()) {
    uint32_t rate=j["selectRate"];  
    com->selectRate(rate);
  }
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

  if(j.find("pllFile") != j.end()) {
	  std::string pllFile=j["pllFile"];  
	  bool forcePllConfig=true;
	  if(j.find("forcePllConfig") != j.end()) { forcePllConfig=j["forcePllConfig"]; }
	  com->configPLL(pllFile, forcePllConfig);
  }

  if(j.find("doDataTransmissionCheck") != j.end()) {
	  bool doDataTransmissionCheck=true;
	  doDataTransmissionCheck=j["doDataTransmissionCheck"]; 
	  com->doDataTransmissionCheck(doDataTransmissionCheck);
  }

  bool dlyCMD=true;
  bool invCMD=false;
  if(j.find("dlyCMD") != j.end()) {
	  dlyCMD=j["dlyCMD"];
  }
  if(j.find("invCMD") != j.end()) {
	  invCMD=j["invCMD"];
  }
  uint32_t temp=2*int(dlyCMD)+int(invCMD);
  com->inv_delay_CMD(temp);


  if(j.find("linkDlyCtrlMode") != j.end()) {
	  int linkDlyCtrlMode=-1;
	  linkDlyCtrlMode=j["linkDlyCtrlMode"]; 

	  if(linkDlyCtrlMode<0){
		  //so, just ignore the setting of DlyCtrl
	  } else if(linkDlyCtrlMode==0){
		  //auto mode
		  com->enManualDlyCtrl(0);

		  //uint32_t slideDlyCfg=0;
		  //uint32_t slideDlyDir=0; 
		  //if(j.find("slideDlyDir") != j.end()) {
		  //    slideDlyDir=j["slideDlyDir"];
		  //    slideDlyDir=slideDlyDir&0x1; }
		  //if(j.find("slideDlyCfg") != j.end()) {
		  //    slideDlyCfg=j["slideDlyCfg"];
		  //    slideDlyCfg=slideDlyCfg&0x3f; }
		  //com->autoDelayData(slideDlyCfg, slideDlyDir);
		  if(j.find("lockingCntCfg") != j.end()) {
			  int lockingCntCfg=j["lockingCntCfg"];
			  com->setAutoLockingCntCfg(lockingCntCfg);
		  }
	  }else{
		  //manual mode
		  com->enManualDlyCtrl(1);
	  }
  }

  if(j.find("tuningDly") != j.end()) {
	  bool tuningDly=false;
	  tuningDly=j["tuningDly"]; 
	  com->enTuningDly(tuningDly);
  }

  if(j.find("eyeScanCfg") != j.end()) {
	  int eyeScanCfg=0;
	  eyeScanCfg=j["eyeScanCfg"]; 
	  com->setEyeScan(eyeScanCfg);
  }


}
