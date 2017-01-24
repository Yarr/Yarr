#ifndef __FEI4_EMU_H__
#define __FEI4_EMU_H__

#define HEXF(x,y) std::hex << "0x" << std::hex << std::setw(x) << std::setfill('0') << static_cast<int>(y) << std::dec

#include "Fei4.h"
#include "EmuShm.h"

#include <cstdint>

class Fei4Emu {
	public:
		Fei4Emu();
		~Fei4Emu();

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

		EmuShm *m_txShm;
		EmuShm *m_rxShm;
		Fei4 *m_fe;

		uint32_t m_modeBits;
		uint32_t m_shiftRegisterBuffer[21][40];

		uint8_t m_feId;
		uint32_t m_l1IdCnt;
		uint32_t m_bcIdCnt;

		// functions for modeling pixel responses
		float calculateThreshold(uint32_t Vthin_Fine, uint32_t Vthin_Coarse, uint32_t TDACVbp, uint32_t TDAC);
		uint32_t calculateToT(float charge);
};

#endif //__FEI4_EMU_H__
