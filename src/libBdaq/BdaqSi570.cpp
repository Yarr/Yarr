#include "BdaqSi570.h"

void BdaqSi570::init(int8_t _slaveAddr, double freq) {
	slaveAddr = _slaveAddr;
	frequencyChange(freq);
}

void BdaqSi570::reset() {
	std::vector<uint8_t> data{135};
	intf.write(slaveAddr, data); //Maybe create a write SINGLE method?
	std::vector<uint8_t> RECALL(1);
	intf.read(slaveAddr, RECALL, 1); //Maybe creat a read SINGLE method?
	uint8_t temp = RECALL[0] | 0x01;
	data = {135, temp};
	intf.write(slaveAddr, data);
}

void BdaqSi570::frequencyChange(double freq) {
	const float f0 = 156.25f;
	reset();
	freqRegs fr = readRegister();
	double fXtal = (f0 * fr.HS_DIV * fr.N1) / (fr.RFREQ / std::pow(2, 28));
	double newFdco = freq * fr.HS_DIV * fr.N1;
	if (newFdco < 4850.0 || newFdco > 5670.0) //DCO freq. must be within 4.85 to 5.67 GHz.
		throw std::runtime_error("Si570 DCO frequency is out of the operating range.");
	double new_RFREQ_freq = newFdco / fXtal;
	uint64_t new_RFREQ = new_RFREQ_freq * std::pow(2, 28);
	fr.RFREQ = new_RFREQ;
	modifyRegister(fr);
	std::cout << "Changed Si570 frequency to " << newFdco / (fr.HS_DIV * fr.N1) << " MHz" << std::endl;
}

void BdaqSi570::modifyRegister(freqRegs fr) {
	fr.HS_DIV = fr.HS_DIV - 4;
	fr.N1 = fr.N1 - 1;
	std::vector<uint8_t> data{137};
	intf.write(slaveAddr, data);
	std::vector<uint8_t> dcoFreeze;
	intf.read(slaveAddr, dcoFreeze, 1);
	data = {135};
	intf.write(slaveAddr, data);
	std::vector<uint8_t> newFreqFlag;
	intf.read(slaveAddr, newFreqFlag, 1);
	//Freeze the DCO
	uint8_t temp = dcoFreeze[0] | 0x10; //0b10000
	data = {137, temp};
	intf.write(slaveAddr, data);
	//Write the new frequency configuration -- Make it nicer.
	data.resize(7);
	data[0] = 7;
	data[1] = ((fr.HS_DIV << 5) & 0xE0) | ((fr.N1    >>  2) & 0x1F); 
	data[2] = ((fr.N1     << 6) & 0xC0) | ((fr.RFREQ >> 32) & 0x3F); 
	data[3] = (fr.RFREQ >> 24) & 0xFF; //bit 31 to 24
	data[4] = (fr.RFREQ >> 16) & 0xFF; //bit 23 to 16
	data[5] = (fr.RFREQ >>  8) & 0xFF; //bit 16 to  8
	data[6] =  fr.RFREQ        & 0xFF; //bit  7 to  0
	intf.write(slaveAddr, data);
	//Unfreeze the DCO
	temp = dcoFreeze[0] & 0x0F; //0b01111
	data = {137, temp};
	intf.write(slaveAddr, data);
	//Assert the NewFreq bit
	temp = newFreqFlag[0] | 0x40; //0b1000000
	data = {135, temp};
	intf.write(slaveAddr, data);
}

freqRegs BdaqSi570::readRegister() {
	freqRegs fr;

	std::vector<uint8_t> data{7};
	intf.write(slaveAddr, data);

	std::vector<uint8_t> regVal;
	intf.read(slaveAddr, regVal, 6);

	fr.HS_DIV = (regVal[0] & 0xE0) >> 5;
	fr.HS_DIV += 4;

	fr.N1 = ((regVal[0] & 0x1F) << 2) | ((regVal[1] & 0xC0) >> 6);
	fr.N1 += 1;

	fr.RFREQ = (((uint64_t)regVal[1] & 0x3F) << 32) | 
				(uint32_t)regVal[2] << 24 | 
				(uint32_t)regVal[3] << 16 | 
				(uint16_t)regVal[4] <<  8 | 
				regVal[5];

	return fr;  
}
