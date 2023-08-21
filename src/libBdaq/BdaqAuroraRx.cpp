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

void BdaqAuroraRx::setUserkFilterMode(userkFilterMode mode) {
	switch(mode) {
		case block : (*this)["USER_K_FILTER_MODE"] = 0; break;
		case filter: (*this)["USER_K_FILTER_MODE"] = 1; break;
		case pass  : (*this)["USER_K_FILTER_MODE"] = 2; break;
		default:
			logger->critical("USERK_K filter mode: {} not allowed. Parameters: 'block', 'filter', 'pass'");
	}
}

uint8_t BdaqAuroraRx::getUserkFilterMode() {
	return (*this)["USER_K_FILTER_MODE"];
}

uint32_t BdaqAuroraRx::getFrameCount() {
	return (*this)["FRAME_COUNTER"];
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

uint BdaqAuroraRx::getRxConfig() {
	return (*this)["RX_LANES"];
}

