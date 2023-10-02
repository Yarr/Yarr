#ifndef BDAQAURORARX_H
#define BDAQAURORARX_H

#include <iostream>
#include <string>
#include <exception>

#include "BdaqRegister.h"
#include "BdaqRBCP.h"

class BdaqAuroraRx : public BdaqRegister<BdaqRBCP> {
	public:
		const uint requireVersion = 4;

		BdaqAuroraRx(BdaqRBCP& _rbcp) : BdaqRegister(_rbcp) {
			registers = {
				{"RESET"               , { 0,  8, 0, Register::WRITE}},
				{"VERSION"             , { 0,  8, 0, Register::READ}},

				{"EN"                  , { 2,  1, 0, Register::READ_WRITE}},
				{"RX_READY"            , { 2,  1, 1, Register::READ}},
				{"RX_LANE_UP"          , { 2,  1, 2, Register::READ}},
				{"PLL_LOCKED"          , { 2,  1, 3, Register::READ}},
				{"RX_HARD_ERROR"       , { 2,  1, 4, Register::READ}},
				{"RX_SOFT_ERROR"       , { 2,  1, 5, Register::READ}},
				{"USER_K_FILTER_MODE"  , { 2,  2, 6, Register::READ_WRITE}},

				{"LOST_COUNT"          , { 3,  8, 0, Register::READ}},

				{"USER_K_FILTER_MASK_1", { 4,  8, 0, Register::READ_WRITE}},
				{"USER_K_FILTER_MASK_2", { 5,  8, 0, Register::READ_WRITE}},
				{"USER_K_FILTER_MASK_3", { 6,  8, 0, Register::READ_WRITE}},

				{"RESET_COUNTERS"      , { 7,  1, 0, Register::READ_WRITE}},
				{"RESET_LOGIC"         , { 7,  1, 1, Register::READ_WRITE}},
				{"GTX_TX_MODE"         , { 7,  1, 3, Register::READ_WRITE}},

				{"FRAME_COUNTER"       , { 8, 32, 0, Register::READ_WRITE}},
				{"SOFT_ERROR_COUNTER"  , {12,  8, 0, Register::READ_WRITE}},
				{"HARD_ERROR_COUNTER"  , {13,  8, 0, Register::READ_WRITE}},

				{"RX_LANES"            , {14,  4, 4, Register::READ_WRITE}}
			};
		}

		enum userkFilterMode {block, filter, pass};
	
		void checkVersion ();
		void resetCounters();
		void resetLogic();
		void setEn(bool value);
		bool getEn();
		bool getRxReady();
		bool getPllLocked();
		uint8_t getLostCount();
		void setMgtRef(std::string value);
		std::string getMgtRef();
		int8_t getSoftErrorCounter ();
		int8_t getHardErrorCounter ();
		void setUserKfilterMask (uint8_t mask, uint8_t value);
		//uint8_t getUserKfilterMask();
		void setUserkFilterMode(userkFilterMode mode);	
		uint8_t getUserkFilterMode();
		uint32_t getFrameCount();
		void setGtxTxMode(std::string value);
		uint getRxConfig();
                
	protected:
};

#endif
