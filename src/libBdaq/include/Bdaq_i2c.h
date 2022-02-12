#ifndef BDAQ_I2C_H
#define BDAQ_I2C_H

#include <stdexcept>
#include <string>

#include "BdaqRegister.h"
#include "BdaqRBCP.h"

class Bdaq_i2c : public BdaqRegister<BdaqRBCP> {
	public:
		const uint requireVersion = 1;

		Bdaq_i2c(BdaqRBCP& _rbcp) : BdaqRegister(_rbcp) {
			registers = {
				{"RESET"    , {0,  8, 0, Register::WRITE}},
				{"VERSION"  , {0,  8, 0, Register::READ}},
				{"START"    , {1,  8, 0, Register::WRITE}}, 
				{"READY"    , {1,  1, 0, Register::READ}},
				{"NO_ACK"   , {1,  1, 1, Register::READ}},
				{"SIZE"     , {3, 16, 0, Register::READ_WRITE}}, 
				{"ADDR"     , {2,  8, 0, Register::READ_WRITE}},
				{"MEM_BYTES", {6, 16, 0, Register::READ_WRITE}}
			};
		}

		void checkVersion();
		void init();
		void reset();
		uint16_t getMemSize();
		void start();
		void setAddr (uint8_t addr);
		uint8_t getAddr();
		void setSize (uint16_t size);
		uint16_t getSize();
		bool isDone();
		bool isReady();
		void setData(std::vector<uint8_t>& data, uint8_t addr=0);
		void getData(std::vector<uint8_t>& data, uint8_t size, uint8_t addr=0) const;
		void write(uint8_t addr, std::vector<uint8_t>& data);
		void read(uint8_t addr, std::vector<uint8_t>& data, uint8_t size);

	protected:
		const uint8_t memOffset = 8;
		uint16_t memSize = 0;

};

#endif
