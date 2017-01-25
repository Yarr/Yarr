#ifndef __FEI4_EMU_H__
#define __FEI4_EMU_H__

#define HEXF(x,y) std::hex << "0x" << std::hex << std::setw(x) << std::setfill('0') << static_cast<int>(y) << std::dec

#include "Fei4.h"
#include "EmuShm.h"
#include "json.hpp"

#include <cstdint>

// this is temporary!!!
typedef struct {
	float Vthin_mean;
	float Vthin_sigma;
	float Vthin_gauss;
	float TDACVbp_mean;
	float TDACVbp_sigma;
	float TDACVbp_gauss;
	float noise_sigma_mean;
	float noise_sigma_sigma;
	float noise_sigma_gauss;
} PixelModeling;

class Fei4Emu {
	public:
		Fei4Emu();
		Fei4Emu(std::string output_model_cfg);
		Fei4Emu(std::string output_model_cfg, std::string input_model_cfg);
		~Fei4Emu();

		void initializePixelModels();

		// input pixel model configuration from a json file
		void initializePixelModelsFromFile(std::string jsonFile);

		// output pixel model configuration to a json file
		void writePixelModelsToFile();

		// the main loop which recieves commands from yarr
		void executeLoop();

		// functions for handling the recieved commands
		void handleTrigger();
		void handleWrFrontEnd(uint32_t chipid, uint32_t bitstream[21]);
		void handleWrRegister(uint32_t chipid, uint32_t address, uint32_t value);
		void handleRunMode(uint32_t chipid, int command);
		void handleGlobalPulse(uint32_t chipid);

		// functions for dealing with sending data to yarr
		void addDataHeader(bool hasErrorFlags);
		void addDataRecord(uint16_t col, uint16_t row, uint8_t tot1, uint8_t tot2);
		void addHit(uint16_t col, uint16_t row, uint8_t tot1, uint8_t tot2);
		uint8_t getToTCode(uint8_t dec_tot);
		void pushOutput(uint32_t value);

		// functions for modeling pixel responses
		float calculateThreshold(int col, int row);
		uint32_t calculateToT(float charge);

		EmuShm *m_txShm;
		EmuShm *m_rxShm;
		Fei4 *m_fe;

		uint32_t m_modeBits;
		uint32_t m_shiftRegisterBuffer[21][40];

		// these are not quite used - they are just set to 0
		uint8_t m_feId;
		uint32_t m_l1IdCnt;
		uint32_t m_bcIdCnt;

		// this is temporary!!!
		PixelModeling m_pixelModels[80][336];

		// this is the file path to output the pixel model configuration
		std::string m_output_model_cfg;
};

#endif //__FEI4_EMU_H__
