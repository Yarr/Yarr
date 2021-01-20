#include <stdexcept>
#include <iostream>

#include "BdaqRegister.h"
#include "BdaqRBCP.h"
#include "logging.h"

namespace {
  auto logger = logging::make_log("BdaqRegister");
}

template<typename T>
uint64_t BdaqRegister<T>::readRegister(const Register &reg) const {
	if(!(reg.type & Register::READ)) {
		logger->critical("Read access to write-only register");
		exit(-1);
	}

	int size = reg.size / 8;
	if(reg.size % 8) {
		size++;
	}

	std::vector<uint8_t> data;
	intf.read(base + reg.addr, data, size);

	int i = 0;
	uint64_t value = 0;
	for(auto j : data) {
		value |= j << (i * 8);
		i++;
	}

	return selectBits(value, reg.offset, reg.size);
}

template<typename T>
void BdaqRegister<T>::writeRegister(const Register &reg, uint64_t val) {
	if(!(reg.type & Register::WRITE)) {
		logger->critical("Write access to read-only register");
		exit(-1);
	}
	val = selectBits(val, 0, reg.size);
	uint64_t value = val;
	int size = reg.size / 8;
	if(reg.size % 8) {
		size++;
 		std::vector<uint8_t> data; 
		intf.read(base + reg.addr, data, size);
		int i = 0;
		value = 0;
		for(auto j : data) {
			value |= j << (i * 8);
			i++;
		}
		value = clearBits(value, reg.offset, reg.size);
		value |= val << reg.offset;
	}
	std::vector<uint8_t> data;
	for(int i = 0; i < size; i++) {
		data.push_back(value & 0xFF);
		value >>= 8;
	}
	intf.write(base + reg.addr, data);
}

template<typename T>
void BdaqRegister<T>::checkVersion(unsigned int v, std::string id) {
	uint mVersion = (*this)["VERSION"];
	if(mVersion != v) {
		std::string error = id + " version mismatch! Expected \"" + 
		std::to_string(v) + "\" and read \"" + std::to_string(mVersion) + 
		"\" from the hardware module.";
		logger->critical(error);
		exit(-1);
	}
}

template<typename T>
void BdaqRegister<T>::reset(void) {
	(*this)["RESET"] = 0;
}

template<typename T>
BdaqRegisterProxy BdaqRegister<T>::operator[](const std::string &name) {
	return BdaqRegisterProxy(std::bind(&BdaqRegister::readRegister, this, registers.at(name)), std::bind(&BdaqRegister::writeRegister, this, registers.at(name), std::placeholders::_1));
}

template<typename T>
void BdaqRegister<T>::printRegisters(void) const {
	for(auto const &i : registers) {
		std::cout << i.first << ": ";
		if(i.second.type & Register::READ) {
			std::cout << readRegister(i.second) ;
		}
		std::cout << std::endl;
	}
}

template class BdaqRegister<BdaqRBCP>;
