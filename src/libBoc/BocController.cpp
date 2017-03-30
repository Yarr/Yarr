#include "BocController.h"

void BocController::loadConfig(json &j) {
	m_com = new BocCom(static_cast<const std::string & >(j["bocHost"]));
	BocTxCore::setCom(m_com);
	BocRxCore::setCom(m_com);
	BocRxCore::setEmu(static_cast<uint32_t>(j["emuMask"]));
}

BocController::~BocController()
{
	if(m_com) delete m_com;
}