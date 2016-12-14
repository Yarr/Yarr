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

#define SHM_SIZE 928
#define COMMAND_SIZE 92

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

	// get the commands from the shared memory space that shm_yarr wrote to
	int shm_command_id;
	key_t shm_command_key;
	char *shm_command_pointer; // this is our data space

	// we need to use the same key that yarr set up
	shm_command_key = 1991;

	// get the shared memory segment
	if ((shm_command_id = shmget(shm_command_key, SHM_SIZE, 0666)) < 0)
	{
		fprintf(stderr, "error: shmget failure\n");
		exit(1);
	}

	// attach the shared memory segment to our data space
	if ((shm_command_pointer = (char*) shmat(shm_command_id, NULL, 0)) == (char*) -1)
	{
		fprintf(stderr, "error: shmat failure\n");
		exit(1);
	}

	// loop until killed, waiting for commands and acting upon commands
        uint32_t command = 0;
	uint32_t value = 0;
	uint32_t bitstream[21];

	// loop over commands in the shared memory, dealing with write/read access using a ring buffer and pointer positions
	int32_t write_pointer_position;
	int32_t read_pointer_position;

	// get the initial read pointer position
	memcpy(&read_pointer_position, &shm_command_pointer[4], 4);

	while (1)
	{
		// get the write pointer position
		memcpy(&write_pointer_position, &shm_command_pointer[0], 4);

		// if there is a new command
		if ((write_pointer_position * read_pointer_position > 0 && abs(write_pointer_position) > abs(read_pointer_position)) || \
		    (write_pointer_position * read_pointer_position < 0 && abs(write_pointer_position) < abs(read_pointer_position)))
		{
//			printf("write_pointer_position = %d\n", write_pointer_position);
//			printf("read_pointer_position = %d\n", read_pointer_position);

			// get the command from the shared memory space
		        memcpy(&command, &shm_command_pointer[abs(read_pointer_position) + 0], 4);
		        memcpy(&value, &shm_command_pointer[abs(read_pointer_position) + 4], 4);
			memcpy(&bitstream[0], &shm_command_pointer[abs(read_pointer_position) + 8], 84);

			// decode the command
			decode_command(command, value, bitstream);

			// update the read pointer position, as long as it won't catch up to the write pointer position
			while (1)
			{
				int32_t tmp_read_pointer_position = read_pointer_position;
				if (tmp_read_pointer_position > 0)
				{
					tmp_read_pointer_position += COMMAND_SIZE;
				}
				if (tmp_read_pointer_position < 0)
				{
					tmp_read_pointer_position -= COMMAND_SIZE;
				}

				// deal with loop around the buffer
				if (abs(tmp_read_pointer_position) >= SHM_SIZE)
				{
					if (tmp_read_pointer_position > 0)
					{
						tmp_read_pointer_position = -8;
					}
					else if (tmp_read_pointer_position < 0)
					{
						tmp_read_pointer_position = 8;
					}
				}

				// see if the new position is the same as the write pointer position
				if (abs(tmp_read_pointer_position) != abs(write_pointer_position))
				{
					read_pointer_position = tmp_read_pointer_position;
					break;
				}

				// grab the write pointer position again to see if it has moved further
//				printf("waiting for the write pointer to move\n");
//				sleep(1);
				memcpy(&write_pointer_position, &shm_command_pointer[0], 4);
			}

			// write the read pointer position to the second position of the shared memory
			memcpy(&shm_command_pointer[4], &read_pointer_position, 4);
		}
	}

	return 0;
}
