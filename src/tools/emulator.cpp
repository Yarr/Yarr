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

#include "Fei4Emu.h"

int main(int argc, char *argv[])
{
	// set up the emulator
	Fei4Emu *emu = new Fei4Emu();
	emu->executeLoop();

	return 0;
}
