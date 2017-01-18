#ifndef EMUSHM
#define EMUSHM

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

class EmuShm {
	public:
		EmuShm(key_t key, uint32_t size, bool create);
		~EmuShm();

		uint32_t shm_size;
		key_t shm_key;
		int shm_id;
		char *shm_pointer;

		bool just_initialized;

		uint32_t write_pointer;
		uint32_t read_pointer;

		void write32(uint32_t word);
		uint32_t read32();
	private:
};

#endif
