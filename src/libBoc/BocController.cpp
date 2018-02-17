#include "BocController.h"

#include "AllHwControllers.h"

bool boc_registered =
  StdDict::registerHwController("boc",
                                []() { return std::unique_ptr<HwController>(new BocController); });

void BocController::loadConfig(json &j) {
	m_com = new BocCom(static_cast<const std::string & >(j["bocHost"]));
	BocTxCore::setCom(m_com);
	BocRxCore::setCom(m_com);
	BocRxCore::setEmu(static_cast<uint32_t>(j["emuMask"]), static_cast<uint8_t>(j["emuHitCnt"]));
}

BocController::~BocController()
{
	if(m_com) delete m_com;
}
