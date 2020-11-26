#ifndef BDAQSI570_H
#define BDAQSI570_H

#include <bitset>
#include <vector>
#include <iostream>

#include "Bdaq_i2c.h"

struct freqRegs {
	uint8_t  HS_DIV;
	uint8_t  N1;
	uint64_t RFREQ;
};

class BdaqSi570 {
	public:
		BdaqSi570(Bdaq_i2c& _intf) : intf(_intf) {}

		void init(int8_t _slaveAddr, double freq);
		void reset();
		void frequencyChange(double freq);
		void modifyRegister(freqRegs fr);
		freqRegs readRegister();

	protected:
		int8_t slaveAddr = 0x00; //i2c slave address
		Bdaq_i2c& intf;
};

#endif
