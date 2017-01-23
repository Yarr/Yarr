#include "Fei4Emu.h"

Fei4Emu::Fei4Emu()
{
	m_feId = 0x00;
	m_l1IdCnt = 0x00;
	m_bcIdCnt = 0x00;

	m_fe = new Fei4(NULL, 0);
	m_txShm = new EmuShm(1337, 256, 0);
	m_rxShm = new EmuShm(1338, 256, 0);
}

Fei4Emu::~Fei4Emu()
{
	delete m_fe;
	delete m_txShm;
	delete m_rxShm;
}

void Fei4Emu::executeLoop()
{
	while (1)
	{
	        uint32_t command;
		uint32_t type;
		uint32_t name;
		uint32_t chipid;
		uint32_t address;

		uint32_t value;
		uint32_t bitstream[21];

		command = m_txShm->read32();
		type = command >> 14;

		switch (type)
		{
			case 0x7400:
				handleTrigger();
				break;
			case 0x0168:
				name = command >> 10 & 0xF;
				chipid = command >> 6 & 0xF;
				address = command & 0x3F;

				switch (name)
				{
					case 1:
//						printf("recieved a RdRegister command\n");
						break;
					case 2:
//						printf("recieved a WrRegister command\n");
						value = m_txShm->read32();
						value >>= 16;
						handleWrRegister(chipid, address, value);
						break;
					case 4:
//						printf("recieved a WrFrontEnd command\n");
						for (int i = 0; i < 21; i++)
						{
							bitstream[i] = m_txShm->read32();
						}
						handleWrFrontEnd(chipid, bitstream);
						break;
					case 8:
//						printf("recieved a GlobalReset command\n");
						break;
					case 9:
//						printf("recieved a GlobalPulse command\n");
						handleGlobalPulse(chipid);
						break;
					case 10:
//						printf("recieved a RunMode command\n");
						handleRunMode(chipid, command);
						break;
				}

				break;
			case 0:
				break;
			default:
				printf("Software Emulator: ERROR - unknown type recieved, %x\n", type);
				break;
		}

	}

}

void Fei4Emu::handleGlobalPulse(uint32_t chipid)
{
	int didSomething = 0;

	// ignore if we get a ReadErrorReq
	if (m_fe->getValue(&Fei4::ReadErrorReq) == 1)
	{
		didSomething = 1;
	}

	// eventually, I should change the FE that I use based on the chipid

	// check if I need to shift the Shift Register by one
	if (m_fe->getValue(&Fei4::S0) == 0 && m_fe->getValue(&Fei4::S1) == 0 && m_fe->getValue(&Fei4::HitLD) == 0 && m_fe->getValue(&Fei4::SR_Clock) == 1)
	{
		didSomething = 1;

		// use Fei4::Colpr_Mode to determine which dc to loop over
		int dc_step = 40;
		switch (m_fe->getValue(&Fei4::Colpr_Mode))
		{
			case 0:
				dc_step = 40;
				break;
			case 1:
				dc_step = 4;
				break;
			case 2:
				dc_step = 8;
				break;
			case 3:
				dc_step = 1;
				break;
		}

		// loop through the 40 double columns
		for (int i = 0; i < 40 / dc_step; i++)
		{
			int dc = m_fe->getValue(&Fei4::Colpr_Addr) + dc_step * i % 40;

			// use these to deal with overflow bits
			uint32_t current_last_bit = 0;
			uint32_t previous_last_bit = 0;

			// shift all bits left by 1, keeping track of the overflow bits
			for (int j = 0; j < 21; j++)
			{
				current_last_bit = m_shiftRegisterBuffer[j][dc] & 0x80000000;
				m_shiftRegisterBuffer[j][dc] <<= 1;
				m_shiftRegisterBuffer[j][dc] += previous_last_bit;
				previous_last_bit = current_last_bit;
			}
		}
	}

	// check if we should write to the shift registers from the pixel registers
	if (m_fe->getValue(&Fei4::S0) == 1 && m_fe->getValue(&Fei4::S1) == 1 && m_fe->getValue(&Fei4::HitLD) == 0 && m_fe->getValue(&Fei4::SR_Clock) == 1)
	{
		// use Fei4::Colpr_Mode to determine which dc to loop over
		int dc_step = 40;
		switch (m_fe->getValue(&Fei4::Colpr_Mode))
		{
			case 0:
				dc_step = 40;
				break;
			case 1:
				dc_step = 4;
				break;
			case 2:
				dc_step = 8;
				break;
			case 3:
				dc_step = 1;
				break;
		}

		// loop through the 40 double columns
		for (int i = 0; i < 40 / dc_step; i++)
		{
			int dc = m_fe->getValue(&Fei4::Colpr_Addr) + dc_step * i % 40;

			DoubleColumnBitOps* bitReg[] = { &m_fe->En(dc), &m_fe->TDAC(dc)[0], &m_fe->TDAC(dc)[1], &m_fe->TDAC(dc)[2], &m_fe->TDAC(dc)[3], &m_fe->TDAC(dc)[4], &m_fe->LCap(dc), &m_fe->SCap(dc), &m_fe->SCap(dc), &m_fe->Hitbus(dc), &m_fe->FDAC(dc)[0], &m_fe->FDAC(dc)[1], &m_fe->FDAC(dc)[2], &m_fe->FDAC(dc)[3] };


			// loop through the 13 double column bits
			for (int i = 0; i < 13; i++)
			{
				// if a double column bit is 1, write the contents of the corresponding pixel register to the Shift Register
				if (m_fe->getValue(&Fei4::Pixel_latch_strobe) & (unsigned) pow(2, i))
				{
					didSomething = 1;
					memcpy(&m_shiftRegisterBuffer[0][dc], bitReg[i]->getStream(), 84);
				}
			}
		}
	}

	// check if we should write to the pixel registers from the shift registers
	if (m_fe->getValue(&Fei4::S0) == 0 && m_fe->getValue(&Fei4::S1) == 0 && m_fe->getValue(&Fei4::HitLD) == 0 && m_fe->getValue(&Fei4::Latch_Enable) == 1)
	{
		// use Fei4::Colpr_Mode to determine which dc to loop over
		int dc_step = 40;
		switch (m_fe->getValue(&Fei4::Colpr_Mode))
		{
			case 0:
				dc_step = 40;
				break;
			case 1:
				dc_step = 4;
				break;
			case 2:
				dc_step = 8;
				break;
			case 3:
				dc_step = 1;
				break;
		}

		// loop through the 40 double columns
		for (int i = 0; i < 40 / dc_step; i++)
		{
			int dc = m_fe->getValue(&Fei4::Colpr_Addr) + dc_step * i % 40;

			DoubleColumnBitOps* bitReg[] = { &m_fe->En(dc), &m_fe->TDAC(dc)[0], &m_fe->TDAC(dc)[1], &m_fe->TDAC(dc)[2], &m_fe->TDAC(dc)[3], &m_fe->TDAC(dc)[4], &m_fe->LCap(dc), &m_fe->SCap(dc), &m_fe->SCap(dc), &m_fe->Hitbus(dc), &m_fe->FDAC(dc)[0], &m_fe->FDAC(dc)[1], &m_fe->FDAC(dc)[2], &m_fe->FDAC(dc)[3] };

			// loop through the 13 double column bits
			for (int i = 0; i < 13; i++)
			{
				// if a double column bit is 1, write the contents of the Shift Register to the corresponding pixel register
				if (m_fe->getValue(&Fei4::Pixel_latch_strobe) & (unsigned) pow(2, i))
				{
					bitReg[i]->set(&m_shiftRegisterBuffer[0][dc]);
					didSomething = 1;
				}
			}
		}
	}

	if (!didSomething)
	{
		// print some info about the state of the global register
		printf("did not do anything\t");
		printf("Fei4::S0 = %x\t", m_fe->getValue(&Fei4::S0));
		printf("Fei4::S1 = %x\t", m_fe->getValue(&Fei4::S1));
		printf("Fei4::HitLD = %x\t", m_fe->getValue(&Fei4::HitLD));
		printf("Fei4::Colpr_Mode = %x\t", m_fe->getValue(&Fei4::Colpr_Mode));
		printf("Fei4::SR_Clock = %x\t", m_fe->getValue(&Fei4::SR_Clock));
		printf("Fei4::Latch_Enable = %x\t", m_fe->getValue(&Fei4::Latch_Enable));
		printf("Fei4::Pixel_latch_strobe = %x\n", m_fe->getValue(&Fei4::Pixel_latch_strobe));
	}
}

void Fei4Emu::handleRunMode(uint32_t chipid, int command)
{
	// eventually, I should change the FE that I use based on the chipId

	// decode the modeBits from the command word
	m_modeBits = command & 0x3F;
}

void Fei4Emu::handleWrRegister(uint32_t chipid, uint32_t address, uint32_t value)
{
	// write value to address in the Global Register (of FE chipid - ignoring this part for now)
	m_fe->cfg[address] = value;
}

void Fei4Emu::handleWrFrontEnd(uint32_t chipid, uint32_t bitstream[21])
{
	// write the bitstream to our Shift Register buffer (eventually, I should have one of these for every FE, and maybe every dc)
	int dc_step = 40;
	switch (m_fe->getValue(&Fei4::Colpr_Mode))
	{
		case 0:
			dc_step = 40;
			break;
		case 1:
			dc_step = 4;
			break;
		case 2:
			dc_step = 8;
			break;
		case 3:
			dc_step = 1;
			break;
	}

	// loop through the 40 double columns
	for (int i = 0; i < 40 / dc_step; i++)
	{
		int dc = m_fe->getValue(&Fei4::Colpr_Addr) + dc_step * i % 40;
		memcpy(&m_shiftRegisterBuffer[0][dc], &bitstream[0], 84);
	}
}

void Fei4Emu::handleTrigger()
{
	this->addDataHeader(false); // No Error flags

	// use Fei4::Colpr_Mode to determine which dc to loop over
	int dc_step = 40;
	switch (m_fe->getValue(&Fei4::Colpr_Mode))
	{
		case 0:
			dc_step = 40;
			break;
		case 1:
			dc_step = 4;
			break;
		case 2:
			dc_step = 8;
			break;
		case 3:
			dc_step = 1;
			break;
	}

	// loop through the 40 double columns
	for (int i = 0; i < 40 / dc_step; i++)
	{
		int dc = m_fe->getValue(&Fei4::Colpr_Addr) + dc_step * i % 40;

		for (unsigned row = 1; row <= m_fe->n_Row; row++)
		{
			if (m_fe->getEn(dc * 2 + 1, row))
			{
				this->addHit(dc * 2 + 1, row, 10, 0);
			}
			if (m_fe->getEn(dc * 2 + 1 + 1, row))
			{
				this->addHit(dc * 2 + 1 + 1, row, 10, 0);
			}
		}
	}
}

void Fei4Emu::addDataHeader(bool hasErrorFlags)
{
	pushOutput( (m_feId << 24) | (0xe9 << 16) | (((uint32_t) hasErrorFlags) << 15) | ((m_l1IdCnt & 0x1F) << 10) | (m_bcIdCnt & 0x3FF) );
}

void Fei4Emu::addDataRecord(uint16_t col, uint16_t row, uint8_t tot1, uint8_t tot2)
{
	pushOutput( (m_feId << 24) | ((col & 0x7F) << 17) | ((row & 0x1FF) << 8) | ((tot1 & 0xF) << 4) | (tot2 & 0xF) );
}

void Fei4Emu::addHit(uint16_t col, uint16_t row, uint8_t tot1, uint8_t tot2)
{
	addDataRecord(col, row, getToTCode(tot1), getToTCode(tot2));
}

uint8_t Fei4Emu::getToTCode(uint8_t dec_tot)
{
	const uint8_t totCodes[3][17] = {
		{ 0xF, 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xD, 0xD },
		{ 0xF, 0xE, 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xD },
		{ 0xF, 0xE, 0xE, 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xA, 0xB, 0xC, 0xD }
	};

	uint8_t hitDiscCfg = m_fe->getValue(&Fei4Cfg::HitDiscCnfg);
	if (dec_tot >= 16)
	{
		dec_tot = 16;
	}
	return totCodes[hitDiscCfg][dec_tot];
}

void Fei4Emu::pushOutput(uint32_t value)
{
	if (m_rxShm)
	{
		m_rxShm->write32(value);
	}
}
