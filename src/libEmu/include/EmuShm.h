/*
 * Author: N. Whallon <alokin@uw.edu>
 * Date: 2017-I
 * Description: a class to facilitate communication between programs using shared memory
 */

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
#include <iostream>

#include "EmuCom.h"

class EmuShm : public EmuCom {
	public:
		EmuShm(key_t key, uint32_t size, bool _create);
		virtual ~EmuShm();

		bool create;

		// variables for dealing with shm
		uint32_t shm_size;	// in bytes
		key_t shm_key;
		int shm_id;
		char *shm_pointer;

		// this is the size of the elements in the shared memory buffer/array
		uint32_t element_size;

		// these indicate the index in the ring buffer where the write and read "cursors" are
		uint32_t write_index;
		uint32_t read_index;

		// this is the index past which writes and reads must wrap (EXCLUSIVE)
		uint32_t index_of_upper_bound;
		// these are the indices which store the write_index and read_index values
		uint32_t index_of_write_index;
		uint32_t index_of_read_index;

		// the main functionality of the class - write to and read from the ring buffer
		virtual void write32(uint32_t word);
		virtual uint32_t read32();
		virtual uint32_t readBlock32(uint32_t *buf, uint32_t length);

		// useful utility functions
		virtual bool isEmpty();
		virtual uint32_t getCurSize();
		virtual void dump();
	private:
};

#endif
