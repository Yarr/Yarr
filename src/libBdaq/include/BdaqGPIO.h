#ifndef BDAQ_GPIO_H
#define BDAQ_GPIO_H

#include <stdexcept>
#include <string>

#include "BdaqRegister.h"
#include "BdaqRBCP.h"

class BdaqGPIO : public BdaqRegister<BdaqRBCP> {
	public:
		const uint requireVersion = 0;

		BdaqGPIO(BdaqRBCP &_rbcp) : BdaqRegister(_rbcp) {
			int ioBytes = ((size - 1) / 8) + 1;
			registers = {
				{"RESET"     , {                    0,     8, 0, Register::WRITE}},
				{"VERSION"   , {                    0,     8, 0, Register::READ}},
				{"INPUT"     , {                    1,  size, 0, Register::READ}}, 
				{"OUTPUT"    , {    2 + (ioBytes - 1),  size, 0, Register::READ_WRITE}},
				{"OUTPUT_EN" , {3 + 2 * (ioBytes - 1),  size, 0, Register::READ_WRITE}}
			};
		}

		void checkVersion();
		void init(uint _size, uint outputEn);
		void setOutputEn (uint outputEn);
		uint getOutputEn();
		void setData(uint data);
		uint getData();

	protected:
		int size = 0;

};

#endif
