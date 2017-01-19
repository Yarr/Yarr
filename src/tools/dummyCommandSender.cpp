// dummyCommandSender
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <unistd.h>
#include <semaphore.h>
#include <pthread.h>
#include <signal.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>

#include "Fei4.h" // not actually needed
#include "EmuShm.h"

int main(int argc, char *argv[])
{
	EmuShm *emu_shm = new EmuShm(1337, 64, 1);

	int command_num;
//	FILE * command_file = fopen("/home/nwhallon/Programming/git/YARR/src/test_raw_command.txt", "r");
	FILE * command_file = fopen("/home/nwhallon/Programming/git/YARR/src/digitalscan_onemaskstep_raw_commands.txt", "r");
	fscanf(command_file, "%d", &command_num);

	for (int i = 0; i < command_num; i++)
	{
		uint32_t word;

		fscanf(command_file, "%x", &word);

		emu_shm->write32(word);
	}

	printf("finished writing commands\n");

	while (1);

	return 0;
}
