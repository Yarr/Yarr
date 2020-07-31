#include "BdaqDriver.h"
#include "logging.h"

namespace {
  auto logger = logging::make_log("BdaqDriver");
}


void BdaqDriver::checkVersion () {
	BdaqRegister::checkVersion(requireVersion, "auroraRx");
}

void BdaqDriver::init() {
	memSize = getMemSize();
}

uint16_t BdaqDriver::getMemSize() {
	return (*this)["MEM_BYTES"];
}

uint16_t BdaqDriver::getCmdSize() {
	return (*this)["SIZE"];
}

void BdaqDriver::start() {
	(*this)["START"] = 0xff;
}

void BdaqDriver::setSize(uint16_t value) {
	//CMD buffer size
	(*this)["SIZE"] = value;
}

uint16_t BdaqDriver::getSize() { //same as getCmdSize()
	//CMD buffer size
	return (*this)["SIZE"];
}

void BdaqDriver::setRepetitions(uint16_t value) {
	//CMD repetitions
	(*this)["REPETITIONS"] = value;
}

uint16_t BdaqDriver::getRepetitions() {
	//CMD repetitions
	return (*this)["REPETITIONS"];
}

void BdaqDriver::setExtTrigger(bool value) { 
	//External trigger input enable
	(*this)["EXT_TRIGGER_EN"] = value;
}

bool BdaqDriver::getExtTrigger() { 
	//External trigger input enable
	return (*this)["EXT_TRIGGER_EN"];
}

void BdaqDriver::setExtStart(bool value) { 
	//External start input enable
	(*this)["EXT_START_EN"] = value;
}

bool BdaqDriver::getExtStart() { 
	//External start input enable
	return (*this)["EXT_START_EN"];
}

void BdaqDriver::setOutputEn(bool value) {
	//CMD output driver. False=high impedance
	(*this)["OUTPUT_EN"] = value;
}

void BdaqDriver::setBypassMode(bool value) {
	//CDR bypass mode (KC705+FMC_LPC). Enables output drivers and sends cmd and serializer clock to the chip.
	//Probably not useful for YARR-BDAQ integration, since the idea is to support the BDAQ hardware onlye (no KC705).
	(*this)["BYPASS_MODE"] = value;
}

bool BdaqDriver::getBypassMode() {
	//CDR bypass mode (KC705+FMC_LPC). Enables output drivers and sends cmd and serializer clock to the chip.
	//Probably not useful for YARR-BDAQ integration, since the idea is to support the BDAQ hardware onlye (no KC705).
	return (*this)["BYPASS_MODE"];
}

void BdaqDriver::setAutoSync(bool value) {
	(*this)["AUTO_SYNC"] = value;
}

bool BdaqDriver::getAutoSync() {
	return (*this)["AUTO_SYNC"];
}

bool BdaqDriver::isDone() {
	return (*this)["READY"];
}

void BdaqDriver::setAzVetoCycles(uint16_t value) {
	//Veto clock cycles in 1/160 Mhz during AZ
	(*this)["AZ_VETO_CYCLES"] = value;
}

uint16_t BdaqDriver::getAzVetoCycles() {
	//Veto clock cycles in 1/160 Mhz during AZ
	return (*this)["AZ_VETO_CYCLES"];
}

void BdaqDriver::setChipType(uint8_t value) {
	//Defines chip type for DAQ 0 = RD53A, 1 = ITKPixV1
	(*this)["CHIP_TYPE"] = value;
}

void BdaqDriver::setData (std::vector<uint8_t>& data, uint8_t addr) {
	if (memSize < data.size()) {
		std::string error = "BdaqDriver::setData(): Size of data (" + 
		std::to_string(data.size()) + " bytes) is bigger than memory (" + 
		std::to_string(memSize) + " bytes).";
		logger->critical(error);
		exit(-1);
	}
	intf.write(base + memOffset + addr, data);
}

void BdaqDriver::getData(std::vector<uint8_t>& data, uint8_t size, uint8_t addr) {
	if (memSize < size) {
		std::string error = "Bdaq_i2c::getData(): Size of data (" + 
		std::to_string(data.size()) + " bytes) is bigger than memory (" + 
		std::to_string(memSize) + " bytes).";
		logger->critical(error);
		exit(-1);
	}
	if (size == 0)
		intf.read(base + memOffset + addr, data, memSize);
	else 
		intf.read(base + memOffset + addr, data, size);
}
