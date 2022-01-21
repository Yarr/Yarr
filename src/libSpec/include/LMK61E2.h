#ifndef LMK61E2_H__
#define LMK61E2_H__

#include "SimpleI2C.h"

// LMK registers
#define LMK61E2_VNDRID_BY1			0
#define LMK61E2_VNDRID_BY0			1
#define LMK61E2_PRODID				2
#define LMK61E2_REVID				3
#define LMK61E2_SLAVEADR			8
#define LMK61E2_EEREV				9
#define LMK61E2_DEV_CTL				10
#define LMK61E2_XO_CAPCTRL_BY1		16
#define LMK61E2_XO_CAPCTRL_BY0		17
#define LMK61E2_DIFFCTL				21
#define LMK61E2_OUTDIV_BY1			22
#define LMK61E2_OUTDIV_BY0			23
#define LMK61E2_PLL_NDIV_BY1		25
#define LMK61E2_PLL_NDIV_BY0		26
#define LMK61E2_PLL_FRACNUM_BY2		27
#define LMK61E2_PLL_FRACNUM_BY1		28
#define LMK61E2_PLL_FRACNUM_BY0		29
#define LMK61E2_PLL_FRACDEN_BY2		30
#define LMK61E2_PLL_FRACDEN_BY1		31
#define LMK61E2_PLL_FRACDEN_BY0		32
#define LMK61E2_PLL_MASHCTRL		33
#define LMK61E2_PLL_CTRL0			34
#define LMK61E2_PLL_CTRL1			35
#define LMK61E2_PLL_LF_R2			36
#define LMK61E2_PLL_LF_C1			37
#define LMK61E2_PLL_LF_R3			38
#define LMK61E2_PLL_LF_C3			39
#define LMK61E2_PLL_CALCTRL			42
#define LMK61E2_NVMSCRC				47
#define LMK61E2_NVMCNT				48
#define LMK61E2_NVMCTL				49
#define LMK61E2_NVMCRC				50
#define LMK61E2_MEMADR				51
#define LMK61E2_NVMDAT				52
#define LMK61E2_RAMDAT				53
#define LMK61E2_NVMUNLK				56
#define LMK61E2_INT_LIVE			66
#define LMK61E2_SWRST				72

// output configuration
#define LMK61E2_OUT_TRI				0
#define LMK61E2_OUT_LVPECL			1
#define LMK61E2_OUT_LVDS			2
#define LMK61E2_OUT_HCSL			3


class LMK61E2 {
public:
	struct LMK61E2ClockConfig {
		bool refDoubling;
		uint16_t pllInteger;
		uint32_t pllNumerator;
		uint32_t pllDenominator;
		uint16_t outputDivider;
	};

	LMK61E2(SimpleI2C &i2c, uint8_t dev = 0x58);
	uint8_t readRegister(uint8_t addr);
	void writeRegister(uint8_t addr, uint8_t value);

	uint16_t getVendor();
	uint8_t getProduct();
	uint8_t getRevision();

	LMK61E2ClockConfig getConfiguration();
	void setConfiguration(LMK61E2ClockConfig config);
	double setOutputFrequency(double outputFrequency, bool useRefDoubling = true);
	double getOutputFrequency();

	void setOutputConfiguration(uint8_t config);
	uint8_t getOutputConfiguration();

	void resetPll();

private:
	SimpleI2C m_i2c;
	uint8_t m_dev;
};

#endif // LMK61E2_H__
