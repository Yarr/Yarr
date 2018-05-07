#include "KU040Controller.h"

#include "AllHwControllers.h"

bool ku040_registered =
  StdDict::registerHwController("ku040",
                                []() { return std::unique_ptr<HwController>(new KU040Controller); });

void KU040Controller::loadConfig(json &j) {
	m_com = new IPbus(static_cast<const std::string & >(j["ku040Host"]));
	KU040TxCore::setCom(m_com);
	KU040RxCore::setCom(m_com);
	KU040RxCore::setEmu(static_cast<uint32_t>(j["emuMask"]), static_cast<uint8_t>(j["emuHitCnt"]));
	KU040RxCore::setSkipRecsWithErrors(static_cast<bool>(j["skipRecsWithErrors"]));
	KU040RxCore::setLinkSpeed(static_cast<uint32_t>(j["linkSpeed"]));
	KU040RxCore::setUDP(static_cast<bool>(j["useUDP"]));
}

KU040Controller::~KU040Controller()
{
	if(m_com) delete m_com;
}
