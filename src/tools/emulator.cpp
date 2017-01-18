// emulator

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <semaphore.h>
#include <pthread.h>
#include <signal.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>

#include "Fei4.h"
#include "EmuShm.h"

uint32_t modeBits;
Fei4 *fe;
uint32_t shift_register_buffer[21][40];

void decode_command(uint32_t cmd, uint32_t val, uint32_t bstream[21])
{
	// decode the command header from the command word - this will tell us what type of command it is
	int header;
	header = cmd & 0x00FFFC00;

	switch (header)
	{
		case 0x005A2800:
		{
//			printf("got a runMode command\n");

			// decode the chipId from the command word
			int chipId;
			chipId = (cmd >> 6) & 0xF;

			// eventually, I should change the FE that I use based on the chipId

			// decode the modeBits from the command word
			modeBits = cmd & 0x3F;

			break;
		}
		case 0x005A2400:
		{
//			printf("got a globalPulse command\n");

			int didSomething = 0;

			// decode the chipId from the command word
			int chipId;
			chipId = (cmd >> 6) & 0xF;

			// ignore if we get a ReadErrorReq
			if (fe->getValue(&Fei4::ReadErrorReq) == 1)
			{
				didSomething = 1;
			}

			// eventually, I should change the FE that I use based on the chipId

			// check if I need to shift the Shift Register by one
			if (fe->getValue(&Fei4::S0) == 0 && fe->getValue(&Fei4::S1) == 0 && fe->getValue(&Fei4::HitLD) == 0 && fe->getValue(&Fei4::SR_Clock) == 1)
			{
				didSomething = 1;
				// use Fei4::Colpr_Mode to determine which dc to loop over
				int dc_step = 40;
				switch (fe->getValue(&Fei4::Colpr_Mode))
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
					int dc = fe->getValue(&Fei4::Colpr_Addr) + dc_step * i % 40;

					// use these to deal with overflow bits
					uint32_t current_first_bit = 0;
					uint32_t previous_first_bit = 0;

					// shift all bits left by 1, keeping track of the overflow bits
					for (int j = 20; j >= 0; j--)
					{
						current_first_bit = shift_register_buffer[j][dc] & 1;
						shift_register_buffer[j][dc] << 1;
						shift_register_buffer[j][dc] += previous_first_bit;
						previous_first_bit = current_first_bit;
					}
				}
			}

			// check if we should write to the shift registers from the pixel registers
			if (fe->getValue(&Fei4::S0) == 1 && fe->getValue(&Fei4::S1) == 1 && fe->getValue(&Fei4::HitLD) == 0 && fe->getValue(&Fei4::SR_Clock) == 1)
			{
				// use Fei4::Colpr_Mode to determine which dc to loop over
				int dc_step = 40;
				switch (fe->getValue(&Fei4::Colpr_Mode))
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
					int dc = fe->getValue(&Fei4::Colpr_Addr) + dc_step * i % 40;
//					printf("dc = %d\n", dc);

					// loop through the 13 double column bits
					for (int i = 0; i < 13; i++)
					{
						// if a double column bit is 1, write the contents of the corresponding pixel register to the Shift Register
						if (fe->getValue(&Fei4::Pixel_latch_strobe) & (unsigned) pow(2, i))
						{
//							printf("writing contents of the Shift Register to pixel register %d\n", i);
							didSomething = 1;
							switch (i)
							{
								case 0:
									memcpy(&shift_register_buffer[0][dc], fe->En(dc).getStream(), 84);
									break;
								case 1:
									memcpy(&shift_register_buffer[0][dc], fe->TDAC(dc)[0].getStream(), 84);
									break;
								case 2:
									memcpy(&shift_register_buffer[0][dc], fe->TDAC(dc)[1].getStream(), 84);
									break;
								case 3:
									memcpy(&shift_register_buffer[0][dc], fe->TDAC(dc)[2].getStream(), 84);
									break;
								case 4:
									memcpy(&shift_register_buffer[0][dc], fe->TDAC(dc)[3].getStream(), 84);
									break;
								case 5:
									memcpy(&shift_register_buffer[0][dc], fe->TDAC(dc)[4].getStream(), 84);
									break;
								case 6:
									memcpy(&shift_register_buffer[0][dc], fe->LCap(dc).getStream(), 84);
									break;
								case 7:
									memcpy(&shift_register_buffer[0][dc], fe->SCap(dc).getStream(), 84);
									break;
								case 8:
									memcpy(&shift_register_buffer[0][dc], fe->Hitbus(dc).getStream(), 84);
									break;
								case 9:
									memcpy(&shift_register_buffer[0][dc], fe->FDAC(dc)[0].getStream(), 84);
									break;
								case 10:
									memcpy(&shift_register_buffer[0][dc], fe->FDAC(dc)[1].getStream(), 84);
									break;
								case 11:
									memcpy(&shift_register_buffer[0][dc], fe->FDAC(dc)[2].getStream(), 84);
									break;
								case 12:
									memcpy(&shift_register_buffer[0][dc], fe->FDAC(dc)[3].getStream(), 84);
									break;
								default:
									printf("why am I trying to write to pixel register %d?\n", i);
									break;
							}
						}
					}
				}
			}

			// check if we should write to the pixel registers from the shift registers
			if (fe->getValue(&Fei4::S0) == 0 && fe->getValue(&Fei4::S1) == 0 && fe->getValue(&Fei4::HitLD) == 0 && fe->getValue(&Fei4::Latch_Enable) == 1)
			{
				// use Fei4::Colpr_Mode to determine which dc to loop over
				int dc_step = 40;
				switch (fe->getValue(&Fei4::Colpr_Mode))
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
					int dc = fe->getValue(&Fei4::Colpr_Addr) + dc_step * i % 40;
//					printf("dc = %d\n", dc);

					// loop through the 13 double column bits
					for (int i = 0; i < 13; i++)
					{
						// if a double column bit is 1, write the contents of the Shift Register to the corresponding pixel register
						if (fe->getValue(&Fei4::Pixel_latch_strobe) & (unsigned) pow(2, i))
						{
//							printf("writing contents of the Shift Register to pixel register %d\n", i);
							didSomething = 1;
							switch (i)
							{
								case 0:
									fe->En(dc).set(&shift_register_buffer[0][dc]);
									break;
								case 1:
									fe->TDAC(dc)[0].set(&shift_register_buffer[0][dc]);
									break;
								case 2:
									fe->TDAC(dc)[1].set(&shift_register_buffer[0][dc]);
									break;
								case 3:
									fe->TDAC(dc)[2].set(&shift_register_buffer[0][dc]);
									break;
								case 4:
									fe->TDAC(dc)[3].set(&shift_register_buffer[0][dc]);
									break;
								case 5:
									fe->TDAC(dc)[4].set(&shift_register_buffer[0][dc]);
									break;
								case 6:
									fe->LCap(dc).set(&shift_register_buffer[0][dc]);
									break;
								case 7:
									fe->SCap(dc).set(&shift_register_buffer[0][dc]);
									break;
								case 8:
									fe->Hitbus(dc).set(&shift_register_buffer[0][dc]);
									break;
								case 9:
									fe->FDAC(dc)[0].set(&shift_register_buffer[0][dc]);
									break;
								case 10:
									fe->FDAC(dc)[1].set(&shift_register_buffer[0][dc]);
									break;
								case 11:
									fe->FDAC(dc)[2].set(&shift_register_buffer[0][dc]);
									break;
								case 12:
									fe->FDAC(dc)[3].set(&shift_register_buffer[0][dc]);
									break;
								default:
									printf("why am I trying to write to pixel register %d?\n", i);
									break;
							}
						}
					}
				}
			}

			if (!didSomething)
			{
				// print some info about the state of the global register
				printf("did not do anything\t");
				printf("Fei4::S0 = %x\t", fe->getValue(&Fei4::S0));
				printf("Fei4::S1 = %x\t", fe->getValue(&Fei4::S1));
				printf("Fei4::HitLD = %x\t", fe->getValue(&Fei4::HitLD));
				printf("Fei4::Colpr_Mode = %x\t", fe->getValue(&Fei4::Colpr_Mode));
				printf("Fei4::SR_Clock = %x\t", fe->getValue(&Fei4::SR_Clock));
				printf("Fei4::Latch_Enable = %x\t", fe->getValue(&Fei4::Latch_Enable));
				printf("Fei4::Pixel_latch_strobe = %x\n", fe->getValue(&Fei4::Pixel_latch_strobe));
			}

			break;
		}
		case 0x005A0800:
		{
//			printf("got a wrRegister command\n");

			// decode the chipId from the command word
			int chipId;
			chipId = (cmd >> 6) & 0xF;

			// decode the address from the command word
			int address;
			address = cmd & 0x3F;

			// decode the value from the value word
			int value;
			value = val >> 16;

			// write value to address in the Global Register (of FE chipId - ignoring this part for now)
			fe->cfg[address] = value;

			break;
		}
		case 0x005A1000:
		{
//			printf("got a wrFrontEnd command\n");

			// decode the chipId from the command word
			int chipId;
			chipId = (cmd >> 6) & 0xF;

			// write the bitstream to our Shift Register buffer (eventually, I should have one of these for every FE, and maybe every dc)
			int dc = fe->getValue(&Fei4::Colpr_Addr);
			memcpy(&shift_register_buffer[0][dc], &bstream[0], 84);

			break;
		}
		default:
		{
			fprintf(stderr, "error: unknown command header: %x\n", header);
			exit(1);
		}
	}
}

int main(int argc, char *argv[])
{
	// set up the fe
	fe = new Fei4(NULL, 0);

	EmuShm *emu_shm = new EmuShm(1991, 200, 0);

	while (1)
	{
	        uint32_t command;
		uint32_t value;
		uint32_t bitstream[21];
		uint32_t padding;

		command = emu_shm->read32();

		if ((command & 0x005A0800) == 0x005A0800)
		{
			printf("got a 0x005A0800 command\n");
			value = emu_shm->read32();
		}
		if ((command & 0x005A1000) == 0x005A1000)
		{
			printf("got a 0x005A1000 command\n");
			for (int i = 0; i < 21; i++)
			{
				bitstream[i] = emu_shm->read32();
			}
		}

		padding = emu_shm->read32();

		// decode the command
		decode_command(command, value, bitstream);
	}

	return 0;
}
