#include <iostream>
#include <cmath>
#include "SimpleI2C.h"
#include "LMK61E2.h"

LMK61E2::LMK61E2(SimpleI2C &i2c, uint8_t dev) : m_i2c(i2c)
{
	// set device
	m_dev = dev;

	// check
	if((getVendor() != 0x100B) || (getProduct() != 0x33))
	{
		std::cerr << "I2C slave 0x" << std::hex << (int)dev << std::dec << " does not look to be a LMK61E2!" << std::endl;
	}
}

uint8_t LMK61E2::readRegister(uint8_t addr)
{
	uint8_t ret;

	// set register address
	m_i2c.write(m_dev, &addr, 1);

	// read register
	m_i2c.read(m_dev, &ret, 1);

	return ret;
}

void LMK61E2::writeRegister(uint8_t addr, uint8_t value)
{
	uint8_t buf[2];

	// prepare write
	buf[0] = addr;
	buf[1] = value;

	// write
	m_i2c.write(m_dev, buf, 2);
}

uint16_t LMK61E2::getVendor()
{
	uint16_t ret;

	ret = (uint16_t) readRegister(LMK61E2_VNDRID_BY1) << 8;
	ret |= (uint16_t) readRegister(LMK61E2_VNDRID_BY0);

	return ret;
}

uint8_t LMK61E2::getProduct(void)
{
	return readRegister(LMK61E2_PRODID);
}

uint8_t LMK61E2::getRevision(void)
{
	return readRegister(LMK61E2_REVID);
}

LMK61E2::LMK61E2ClockConfig LMK61E2::getConfiguration()
{
	LMK61E2::LMK61E2ClockConfig config;

	// get reference doubling
	config.refDoubling = readRegister(LMK61E2_PLL_CTRL0) & 0x20;

	// get integer part
	config.pllInteger = (uint16_t) (readRegister(LMK61E2_PLL_NDIV_BY1) & 0x0F) << 8;
	config.pllInteger |= readRegister(LMK61E2_PLL_NDIV_BY0);

	// get numerator
	config.pllNumerator = (uint32_t) (readRegister(LMK61E2_PLL_FRACNUM_BY2) & 0x3F) << 16;
	config.pllNumerator = (uint32_t) readRegister(LMK61E2_PLL_FRACNUM_BY1) << 8;
	config.pllNumerator |= readRegister(LMK61E2_PLL_FRACNUM_BY0);	

	// get denominator
	config.pllDenominator = (uint32_t) (readRegister(LMK61E2_PLL_FRACDEN_BY2) & 0x3F) << 16;
	config.pllDenominator = (uint32_t) readRegister(LMK61E2_PLL_FRACDEN_BY1) << 8;
	config.pllDenominator |= readRegister(LMK61E2_PLL_FRACDEN_BY0);	

	// get output divider
	config.outputDivider = (uint16_t) (readRegister(LMK61E2_OUTDIV_BY1) & 0x01) << 8;
	config.outputDivider |= readRegister(LMK61E2_OUTDIV_BY0);

	return config;
}

void LMK61E2::setConfiguration(LMK61E2::LMK61E2ClockConfig config)
{
	uint8_t tmp;

	// check configuration
	if(config.pllInteger == 0)				config.pllInteger = 1;
	if(config.pllNumerator > 4194303)		config.pllNumerator = 4194303;
	if(config.pllDenominator > 4194303)		config.pllDenominator = 4194303;
	if(config.pllDenominator == 0)			config.pllDenominator = 1;
	if(config.outputDivider < 5)			config.outputDivider = 5;
	if(config.outputDivider > 511)			config.outputDivider = 511;

	// update ref doubling
	tmp = readRegister(LMK61E2_PLL_CTRL0);
	if(config.refDoubling)
		tmp |= 0x20;
	else
		tmp &= ~0x20;
	writeRegister(LMK61E2_PLL_CTRL0, tmp);

	// update integer part
	writeRegister(LMK61E2_PLL_NDIV_BY1, config.pllInteger >> 8);
	writeRegister(LMK61E2_PLL_NDIV_BY0, config.pllInteger & 0xFF);

	// update numerator
	writeRegister(LMK61E2_PLL_FRACNUM_BY2, config.pllNumerator >> 16);
	writeRegister(LMK61E2_PLL_FRACNUM_BY1, config.pllNumerator >> 8);
	writeRegister(LMK61E2_PLL_FRACNUM_BY0, config.pllNumerator);

	// update denominator
	writeRegister(LMK61E2_PLL_FRACDEN_BY2, config.pllDenominator >> 16);
	writeRegister(LMK61E2_PLL_FRACDEN_BY1, config.pllDenominator >> 8);
	writeRegister(LMK61E2_PLL_FRACDEN_BY0, config.pllDenominator);

	// update output divider
	writeRegister(LMK61E2_OUTDIV_BY1, (config.outputDivider >> 8) & 0x01);
	writeRegister(LMK61E2_OUTDIV_BY0, config.outputDivider & 0xFF);

	// reset PLL to update settings
	resetPll();
}

double LMK61E2::setOutputFrequency(double outputFrequencyMHz, bool useRefDoubling)
{
	// store bet settings
	int outputDivider_best = 511;
	int pllInteger_best = 1;
	int pllNumerator_best = 4194303;
	int pllDenominator_best = 4194303;

	// for now disable reference doubling
	int outputDivider_min = ceil(4600.0 / outputFrequencyMHz);
	int outputDivider_max = floor(5600.0 / outputFrequencyMHz);

	// debug
	std::cout << "outputDivider: min=" << outputDivider_min << ", max=" << outputDivider_max << std::endl;

	// loop through possible output dividers
	for(int outputDivider = outputDivider_min; outputDivider <= outputDivider_max; outputDivider++)
	{
		double multiplier;

		if(useRefDoubling)
		{
			multiplier = (outputFrequencyMHz * outputDivider) / 100.0;
		}
		else
		{
			multiplier = (outputFrequencyMHz * outputDivider) / 50.0;
		}

		int integer = floor(multiplier);
		double frac = multiplier - (double)integer;

		// get numerator/denominator
		int denominator = 1;
		int numerator = 0;
		for(denominator = 1; denominator < 4194303; denominator++)
		{
			double dnumerator = frac * (double)denominator;
			numerator = frac * denominator;
			if(dnumerator - (double)numerator < 1e-2) break;
		}

		// compare to best settings
		if(numerator <= pllNumerator_best)
		{
			outputDivider_best = outputDivider;
			pllInteger_best = integer;
			pllNumerator_best = numerator;
			pllDenominator_best = denominator;
		}
	}

	// more debugging
	std::cout << "best config: outputDivider=" << outputDivider_best << ", integer=" << pllInteger_best << ", num=" << pllNumerator_best << ", den=" << pllDenominator_best << std::endl;

	// update configuration
	LMK61E2::LMK61E2ClockConfig config;
	config.refDoubling = useRefDoubling;
	config.pllInteger = pllInteger_best;
	config.pllNumerator = pllNumerator_best;
	config.pllDenominator = pllDenominator_best;
	config.outputDivider = outputDivider_best;
	setConfiguration(config);

	// return the actual output frequency
	return getOutputFrequency();
}

double LMK61E2::getOutputFrequency()
{
	// get the current configuration
	LMK61E2::LMK61E2ClockConfig config = getConfiguration();

	// check for proper denominator
	if(config.pllDenominator == 0)
		config.pllDenominator = 1;

	// calculate VCO frequency
	double freq = 50.0 * (config.refDoubling ? 2.0 : 1.0) * ((double)config.pllInteger + (double)config.pllNumerator/(double)config.pllDenominator) / config.outputDivider;

	return freq;
}

void LMK61E2::setOutputConfiguration(uint8_t config)
{
	writeRegister(LMK61E2_DIFFCTL, config & 0x03);
}

uint8_t LMK61E2::getOutputConfiguration()
{
	return readRegister(LMK61E2_DIFFCTL) & 0x3;
}

void LMK61E2::resetPll()
{
	writeRegister(LMK61E2_SWRST, 0x2);
}