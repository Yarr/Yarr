#include "EmuShm.h"

EmuShm::EmuShm(key_t key, uint32_t size, bool create)
{
	shm_key = key;
	shm_size = size;

	// create or get the shared memory segment using the key
	if (create)
	{
		if ((shm_id = shmget(shm_key, shm_size, IPC_CREAT | 0666)) < 0)
		{
			fprintf(stderr, "shmget failure\n");
		}
	}
	else
	{
		if ((shm_id = shmget(shm_key, shm_size, 0666)) < 0)
		{
			fprintf(stderr, "shmget failure\n");
		}
	}

	// attach the shared memory segment to our data space
	if ((shm_pointer = (char *) shmat(shm_id, NULL, 0)) == (char *) -1)
	{
		fprintf(stderr, "shmat failure\n");
	}

	if (create)
	{
		just_initialized = 1;

		read_pointer = 0;
		write_pointer = 0;
	}
	else
	{
		memcpy(&write_pointer, &shm_pointer[(shm_size - 2) * 4], 4);	// shm_size - 2 is where the write pointer lives
		memcpy(&read_pointer, &shm_pointer[(shm_size - 1) * 4], 4);	// shm_size - 1 is where the read pointer lives
	}
}

EmuShm::~EmuShm()
{
	// detach the shared memory
	shmdt(shm_pointer);

	// remove the shared memory
	struct shmid_ds shm_id_ds;

	if (shmctl(shm_id, IPC_RMID, &shm_id_ds) == -1)
	{
		fprintf(stderr, "shmctl failed");
	}
}

void EmuShm::write32(uint32_t word)
{
	printf("writing the word %x\n", word);

	if (just_initialized)
	{
		// write the word
		memcpy(&shm_pointer[write_pointer], &word, 4);

		// update the write pointer
		write_pointer += 1;

		if (write_pointer > shm_size - 2)	// the ring buffer only goes from [0, shm_size - 2), in other words, from [0, shm_size - 3]
		{
			write_pointer = 0;
		}

		memcpy(&shm_pointer[(shm_size - 2) * 4], &write_pointer, 4); // shm_size - 2 is where the write pointer lives

		// set jet_initialized to false
		just_initialized = 0;
	}
	else
	{
		// wait if the write pointer would catch up to the read pointer
		while (((write_pointer + 1 > shm_size - 2) ? 0 : write_pointer + 1) == read_pointer)
		{
			memcpy(&read_pointer, &shm_pointer[(shm_size - 1) * 4], 4);	// shm_size - 1 is where the read pointer lives
		}

		// do the write
		memcpy(&shm_pointer[write_pointer], &word, 4);

		// update the write pointer
		write_pointer += 1;

		if (write_pointer > shm_size - 2)	// the ring buffer only goes from [0, shm_size - 2), in other words, from [0, shm_size - 3]
		{
			write_pointer = 0;
		}

		memcpy(&shm_pointer[(shm_size - 2) * 4], &write_pointer, 4); // shm_size - 2 is where the write pointer lives
	}
}

uint32_t EmuShm::read32()
{
	uint32_t word;

	// wait if the read pointer has caught up to the write pointer
	while (read_pointer == write_pointer)
	{
		memcpy(&write_pointer, &shm_pointer[(shm_size - 2) * 4], 4);	// shm_size - 2 is where the write pointer lives
	}

	// do the read
	memcpy(&word, &shm_pointer[read_pointer], 4);

	// update the read pointer
	read_pointer += 1;

	if (read_pointer > shm_size - 2)	// the ring buffer only goes from [0, shm_size - 2), in other words, from [0, shm_size - 3]
	{
		read_pointer = 0;
	}

	memcpy(&shm_pointer[(shm_size - 1) * 4], &read_pointer, 4); // shm_size - 1 is where the write pointer lives

	printf("read the word %x\n", word);

	return word;
}
