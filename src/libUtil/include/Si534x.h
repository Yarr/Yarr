#ifndef SI534X_H__
#define SI534X_H__

#include "SimpleI2C.h"

class Si534x {
public:
	struct Si534xConfig {
		uint16_t address;
		uint8_t value;
	};

	Si534x(SimpleI2C &i2c, uint8_t dev = 0x74);
	uint8_t readRegister(uint16_t addr);
	void writeRegister(uint16_t addr, uint8_t value);
	uint16_t getBasePart();
	char getDeviceRev();
	char getDeviceSpeed();
	void writeConfiguration(Si534xConfig &config);

private:
	SimpleI2C m_i2c;
	uint8_t m_dev;
	uint8_t m_page;
};

#endif // SI534X_H__