#include <iostream>
#include "SimpleI2C.h"
#include "Si534x.h"

Si534x::Si534x(SimpleI2C &i2c, uint8_t dev) : m_i2c(i2c)
{
	// set device
	m_dev = dev;

	// make sure we are on page 0
	writeRegister(0x1, 0);
	m_page = 0;
}

uint8_t Si534x::readRegister(uint16_t addr)
{
	uint8_t page = addr >> 8;
	uint8_t reg = addr & 0xFF;
	uint8_t ret;

	// we have to set correct page
	if((reg != 1) && (m_page != page))
	{
		writeRegister(0x1, page);
		m_page = page;
	}

	// now set correct address
	m_i2c.write(m_dev, &reg, 1);

	// read register value
	m_i2c.read(m_dev, &ret, 1);

	return ret;
}

void Si534x::writeRegister(uint16_t addr, uint8_t value)
{
	uint8_t buf[2];

	uint8_t page = addr >> 8;
	uint8_t reg = addr & 0xFF;

	// we have to set correct page
	if((reg != 1) && (m_page != page))
	{
		writeRegister(0x1, page);
		m_page = page;
	}

	// prepare buffer
	buf[0] = reg;
	buf[1] = value;

	// write register
	m_i2c.write(m_dev, buf, 2);
}

uint16_t Si534x::getBasePart()
{
	uint16_t ret;

	ret = (uint16_t) readRegister(2);
	ret |= (uint16_t) readRegister(3) << 8;

	return ret;
}

char Si534x::getDeviceRev()
{
	return 'A' + readRegister(0x5);
}

char Si534x::getDeviceSpeed()
{
	return 'A' + readRegister(0x4);
}