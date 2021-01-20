#include "BdaqAuroraRx.h"
#include "logging.h"

namespace {
  auto logger = logging::make_log("BdaqAuroraRx");
}

void BdaqAuroraRx::checkVersion () {
	BdaqRegister::checkVersion(requireVersion, "auroraRx");
}

void BdaqAuroraRx::resetCounters() {
	//Resets the soft/hard error counters
	(*this)["RESET_COUNTERS"] = true;
	(*this)["RESET_COUNTERS"] = false;
}

void BdaqAuroraRx::resetLogic() {
	//Resets ony the logic and the FIFOs
	(*this)["RESET_LOGIC"] = true;
	(*this)["RESET_LOGIC"] = false;
}

void BdaqAuroraRx::setEn(bool value) {
	(*this)["EN"] = value;
} 

bool BdaqAuroraRx::getEn() {
	return (*this)["EN"];
}

bool BdaqAuroraRx::getRxReady() {
	//Aurora link established
	return (*this)["RX_READY"];
}

bool BdaqAuroraRx::getPllLocked() {
	//Aurora PLL locked
	return (*this)["PLL_LOCKED"];
}

uint8_t BdaqAuroraRx::getLostCount() {
	//Lost data due to RX FIFO overflow
	return (*this)["LOST_COUNT"];
}

void BdaqAuroraRx::setMgtRef(std::string value) {
	//Controls the clock multiplexer chip, which is used for providing the Aurora reference clock
	if (value == "int") {
		logger->info("MGT: Switching to on-board (Si570) oscillator");
		(*this)["MGT_REF_SEL"] = 1;
	} else if (value == "ext") {
		logger->info("MGT: Switching to external (SMA) clock source");
		(*this)["MGT_REF_SEL"] = 0;
	}
}

std::string BdaqAuroraRx::getMgtRef() {
	bool value = (*this)["MGT_REF_SEL"];
	if (value == false)
		return "EXT (SMA)";
	else
		return "INT (Si570)";
}

int8_t BdaqAuroraRx::getSoftErrorCounter () {
	//Aurora soft errors
	return (*this)["SOFT_ERROR_COUNTER"];
}

int8_t BdaqAuroraRx::getHardErrorCounter () {
	//Aurora hard errors
	return (*this)["HARD_ERROR_COUNTER"];
}

void BdaqAuroraRx::setUserKfilterMask (uint8_t mask, uint8_t value) {
	if (mask == 1)
		(*this)["USER_K_FILTER_MASK_1"] = value;
	else if (mask == 2)
		(*this)["USER_K_FILTER_MASK_2"] = value;
	else if (mask == 3)
		(*this)["USER_K_FILTER_MASK_3"] = value;
	else {
		logger->critical("USER_K_FILTER_MASK: Parameters: mask_number[1, 2, 3], value[byte]");
		exit(-1);
	}
}

/*uint8_t BdaqAuroraRx::getUserKfilterMask() {
	return (*this)["USER_K_FILTER_MASK"];
}*/

void BdaqAuroraRx::setUserKfilterEn(bool value) {
	(*this)["USER_K_FILTER_EN"] = value;
}

bool BdaqAuroraRx::getUserKfilterEn() {
	return (*this)["USER_K_FILTER_EN"];
}

uint32_t BdaqAuroraRx::getFrameCount() {
	return (*this)["FRAME_COUNTER"];
}

void BdaqAuroraRx::setSi570IsConfigured(bool value) {
	//Emulates a "is_configured" register for the Si570 reference clock chip
	(*this)["SI570_IS_CONFIGURED"] = value;
}

bool BdaqAuroraRx::getSi570IsConfigured() {
	return (*this)["SI570_IS_CONFIGURED"];
}

void BdaqAuroraRx::setGtxTxMode(std::string value) {
	//Set operation mode for the GTX transmitter
	//-CMD:		Command data is sent via GTX 
	//-CLK640:	640 MHz clock output
	//Modes can be changed at any time after getPllLocked==1
	if (value == "CMD") {
		(*this)["GTX_TX_MODE"] = 0;
		//Debug: "GTX TX MODE: Command data"
	} else if (value == "CLK640") {
		(*this)["GTX_TX_MODE"] = 1;
		//Debug: "GTX TX MODE: 640 mHz clock"
	} else {
		logger->critical("GTX TX MODE: parameter out of range");
		exit(-1);
	}
}

rxConfig BdaqAuroraRx::getRxConfig() {
	rxConfig rxc;
	rxc.rxLanes    = (*this)["RX_LANES"];
	rxc.rxChannels = (*this)["RX_CHANNELS"];
	return rxc;
}
