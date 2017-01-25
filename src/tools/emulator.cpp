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

Fei4Emu *g_emu;

void interrupt_received(int signum)
{
	g_emu->writePixelModelsToFile();

	delete(g_emu);

	exit(signum);
}

int main(int argc, char *argv[])
{
	signal(SIGINT, interrupt_received);

	// set up the emulator
	if (argc == 1)
	{
		g_emu = new Fei4Emu();
	}
	else if (argc == 2)
	{
		g_emu = new Fei4Emu(argv[1]);
	}
	else if (argc == 3)
	{
		g_emu = new Fei4Emu(argv[1], argv[2]);
	}
	g_emu->executeLoop();

	return 0;
}
