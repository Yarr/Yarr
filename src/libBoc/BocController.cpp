#include "BocController.h"

void BocController::loadConfig(json &j) {
	m_com = new BocCom(static_cast<const std::string & >(j["bocHost"]));
	BocTxCore::setCom(m_com);
	BocRxCore::setCom(m_com);
}

BocController::~BocController()
{
	if(m_com) delete m_com;
}