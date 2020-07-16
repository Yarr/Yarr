#include "BdaqGPIO.h"

void BdaqGPIO::checkVersion() {
	BdaqRegister::checkVersion(requireVersion, "gpio");
}

void BdaqGPIO::init(uint _size, uint outputEn) {
	size = _size;
	(*this)["OUTPUT_EN"] = outputEn;
}

void BdaqGPIO::setOutputEn (uint outputEn) {
	(*this)["OUTPUT_EN"] = outputEn;
}

uint BdaqGPIO::getOutputEn() {
	return (*this)["OUTPUT_EN"];
}

void BdaqGPIO::setData(uint data) {
	(*this)["OUTPUT"] = data;
}

uint BdaqGPIO::getData() {
	return (*this)["INPUT"];
}
