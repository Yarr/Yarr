#include "BdaqCmdRd53.h"

void BdaqCmdRd53::checkVersion () {
	BdaqRegister::checkVersion(requireVersion, "auroraRx");
}

void BdaqCmdRd53::init() {
	memSize = getMemSize();
}

uint16_t BdaqCmdRd53::getMemSize() {
	return (*this)["MEM_BYTES"];
}

uint16_t BdaqCmdRd53::getCmdSize() {
	return (*this)["SIZE"];
}

void BdaqCmdRd53::start() {
	(*this)["START"] = 0xff;
}

void BdaqCmdRd53::setSize(uint16_t value) {
	//CMD buffer size
	(*this)["SIZE"] = value;
}

uint16_t BdaqCmdRd53::getSize() { //same as getCmdSize()
	//CMD buffer size
	return (*this)["SIZE"];
}

void BdaqCmdRd53::setRepetitions(uint16_t value) {
	//CMD repetitions
	(*this)["REPETITIONS"] = value;
}

uint16_t BdaqCmdRd53::getRepetitions() {
	//CMD repetitions
	return (*this)["REPETITIONS"];
}

void BdaqCmdRd53::setExtTrigger(bool value) { 
	//External trigger input enable
	(*this)["EXT_TRIGGER_EN"] = value;
}

bool BdaqCmdRd53::getExtTrigger() { 
	//External trigger input enable
	return (*this)["EXT_TRIGGER_EN"];
}

void BdaqCmdRd53::setExtStart(bool value) { 
	//External start input enable
	(*this)["EXT_START_EN"] = value;
}

bool BdaqCmdRd53::getExtStart() { 
	//External start input enable
	return (*this)["EXT_START_EN"];
}

void BdaqCmdRd53::setOutputEn(bool value) {
	//CMD output driver. False=high impedance
	(*this)["OUTPUT_EN"] = value;
}

void BdaqCmdRd53::setBypassMode(bool value) {
	//CDS bypass mode (KC705+FMC_LPC). Enables output drivers and sends cmd and serializer clock to the chip.
	//Probably not useful for YARR-BDAQ integration, since the idea is to support the BDAQ hardware onlye (no KC705).
	(*this)["BYPASS_MODE"] = value;
}

bool BdaqCmdRd53::getBypassMode() {
	//CDS bypass mode (KC705+FMC_LPC). Enables output drivers and sends cmd and serializer clock to the chip.
	//Probably not useful for YARR-BDAQ integration, since the idea is to support the BDAQ hardware onlye (no KC705).
	return (*this)["BYPASS_MODE"];
}

bool BdaqCmdRd53::isDone() {
	return (*this)["READY"];
}

void BdaqCmdRd53::setAzVetoCycles(uint16_t value) {
	//Veto clock cycles in 1/160 Mhz during AZ
	(*this)["AZ_VETO_CYCLES"] = value;
}

uint16_t BdaqCmdRd53::getAzVetoCycles() {
	//Veto clock cycles in 1/160 Mhz during AZ
	return (*this)["AZ_VETO_CYCLES"];
}

void BdaqCmdRd53::setData (std::vector<uint8_t>& data, uint8_t addr) {
	if (memSize < data.size()) {
		std::string error = "BdaqCmdRd53::setData(): Size of data (" + 
		std::to_string(data.size()) + " bytes) is bigger than memory (" + 
		std::to_string(memSize) + " bytes).";
		throw std::runtime_error(error);
	}
	intf.write(base + memOffset + addr, data);
}

void BdaqCmdRd53::getData(std::vector<uint8_t>& data, uint8_t size, uint8_t addr) {
	if (memSize < size) {
		std::string error = "Bdaq_i2c::getData(): Size of data (" + 
		std::to_string(data.size()) + " bytes) is bigger than memory (" + 
		std::to_string(memSize) + " bytes).";
		throw std::runtime_error(error);
	}
	if (size == 0)
		intf.read(base + memOffset + addr, data, memSize);
	else 
		intf.read(base + memOffset + addr, data, size);
}
