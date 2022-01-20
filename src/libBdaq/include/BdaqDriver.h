#ifndef BDAQDRIVER_H
#define BDAQDRIVER_H

#include <iostream>
#include <string>
#include <exception>

#include "BdaqRegister.h"
#include "BdaqRBCP.h"

class BdaqDriver : public BdaqRegister<BdaqRBCP> {
	public:
		const uint requireVersion = 2;

		BdaqDriver(BdaqRBCP &_rbcp) : BdaqRegister(_rbcp) {
			registers = {
				//Name            ,Addr, Size, Offset, Mode
				{"RESET"          , { 0,  8, 0, Register::WRITE}},
				{"VERSION"        , { 0,  8, 0, Register::READ}},
				{"START"          , { 1,  8, 0, Register::WRITE}},
				
				{"READY"          , { 2,  1, 0, Register::READ}},
				{"SYNCING"        , { 2,  1, 1, Register::READ}},
				{"EXT_START_EN"   , { 2,  1, 2, Register::READ_WRITE}},
				{"EXT_TRIGGER_EN" , { 2,  1, 3, Register::READ_WRITE}},
				{"OUTPUT_EN"      , { 2,  1, 4, Register::READ_WRITE}},
				{"BYPASS_MODE"    , { 2,  1, 5, Register::READ_WRITE}},
				{"CHIP_TYPE"      , { 2,  2, 6, Register::READ_WRITE}},

				{"SIZE"           , { 3, 16, 0, Register::READ_WRITE}},
				{"REPETITIONS"    , { 5, 16, 0, Register::READ_WRITE}},
				{"MEM_BYTES"      , { 7, 16, 0, Register::READ}},
				{"AZ_VETO_CYCLES" , { 9, 16, 0, Register::READ_WRITE}},

				{"AUTO_SYNC"      , {11,  1, 2, Register::WRITE}}
			};
		}

		void checkVersion ();
		void init();
		uint16_t getMemSize();
		uint16_t getCmdSize();
		void start();
		void setSize(uint16_t value);
		uint16_t getSize(); //same as getCmdSize()
		void setRepetitions(uint16_t value);
		uint16_t getRepetitions();
		void setExtTrigger(bool value); 
		bool getExtTrigger(); 
		void setExtStart(bool value); 
		bool getExtStart(); 
		void setOutputEn(bool value);
		void setBypassMode(bool value);
		bool getBypassMode();
		void setAutoSync(bool value);
		bool getAutoSync();
		bool isDone();
		void setAzVetoCycles(uint16_t value);
		uint16_t getAzVetoCycles();
		void setChipType(uint8_t value);
		void setData (std::vector<uint8_t>& data, uint8_t addr=0);
		void getData(std::vector<uint8_t>& data, uint8_t size, uint8_t addr=0) const;

	protected:
		const uint8_t memOffset = 16;
		uint16_t memSize = 0;
};

#endif
