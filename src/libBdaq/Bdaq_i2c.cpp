#include "Bdaq_i2c.h"

void Bdaq_i2c::checkVersion() {
	BdaqRegister::checkVersion(requireVersion, "i2c");
}

void Bdaq_i2c::init() {
	memSize = getMemSize();
}

void Bdaq_i2c::reset() {
	(*this)["RX_RESET"] = 0;
}

uint16_t Bdaq_i2c::getMemSize() {
	return (*this)["MEM_BYTES"];
}

void Bdaq_i2c::start() {
	(*this)["START"] = 0;
}

void Bdaq_i2c::setAddr (uint8_t addr) {
	(*this)["ADDR"] = addr;
}

uint8_t Bdaq_i2c::getAddr() {
	return (*this)["ADDR"];
}

void Bdaq_i2c::setSize (uint16_t size) {
	(*this)["SIZE"] = size;
}

uint16_t Bdaq_i2c::getSize() {
	return (*this)["SIZE"];
}

bool Bdaq_i2c::isDone() {
	return isReady(); 
}

bool Bdaq_i2c::isReady() {
	if ((*this)["NO_ACK"])
		throw std::runtime_error("i2c: Transfer not acknowledged.");
	return (*this)["READY"];
}

void Bdaq_i2c::setData(std::vector<uint8_t>& data, uint8_t addr) {
	if (memSize < data.size()) {
		std::string error = "Bdaq_i2c::setData(): Size of data (" + 
		std::to_string(data.size()) + " bytes) is bigger than memory (" + 
		std::to_string(memSize) + " bytes).";
		throw std::runtime_error(error);
	}
	intf.write(base + memOffset + addr, data);
}

void Bdaq_i2c::getData(std::vector<uint8_t>& data, uint8_t size, uint8_t addr) {
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

void Bdaq_i2c::write(uint8_t addr, std::vector<uint8_t>& data) {
	setAddr(addr & 0xFE);
	setData(data);
	setSize(data.size());
	start();
	while (!isReady());
}

void Bdaq_i2c::read(uint8_t addr, std::vector<uint8_t>& data, uint8_t size) {
	setAddr(addr | 0x01);
	setSize(size);
	start();
	while (!isReady());
	getData(data, size);
}
