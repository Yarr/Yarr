#ifndef BDAQREGISTER_H
#define BDAQREGISTER_H

// Register Layer

#include <map>
#include <string>

#include "BdaqRegisterProxy.h"

struct Register {
	int addr;
	int size;
	int offset;

	enum {
		READ = 1 << 0,
		WRITE = 1 << 1,
		READ_WRITE = READ | WRITE
	} type;
};

struct FERegister {
	int addr;
	int size;
	int offset;

	enum {
		READ = 1 << 0,
		WRITE = 1 << 1,
		MSB_LAST = 1 << 2,
		READ_WRITE = READ | WRITE,
		READ_WRITE_MSB = READ_WRITE | MSB_LAST
	} type;
};

template<typename T>
T selectBits(T val, int offset, int length) {
	return (val >> offset) & ((1ull << length) - 1);
}

template<typename T>
T clearBits(T val, int offset, int length) {
	return val & ~(((1ull << length) - 1) << offset);
}

//Made this class generic so it could take "any" interface.
//Up to now, only the "BdaqRBCP" interface is used.
template<typename T>
class BdaqRegister {
	public:
		BdaqRegister(T& _intf) : intf(_intf) {}
		BdaqRegisterProxy operator[](const std::string &name);

		void setBase(uint _base) {base = _base;}

		void printRegisters(void) const;
		void reset(void);

	protected:
		void checkVersion(unsigned int v, std::string id);
		uint64_t readRegister(const Register& reg) const;
		void writeRegister(const Register& reg, uint64_t val);

		std::map<std::string, Register> registers;

		uint base = 0;
		T& intf;
};

#endif
