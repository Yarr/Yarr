#include "EmuShm.h"

EmuShm::EmuShm(key_t key, uint32_t size, bool create)
{
	shm_key = key;
	shm_size = size;
	cur_size = 0;

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

	// initialize these indices to be at the back of the shm buffer
	index_of_upper_bound = shm_size / 4 - 2;
	index_of_write_index = shm_size / 4 - 2;
	index_of_read_index = shm_size / 4 - 1;

	if (create)
	{
		write_index = 0;
		read_index = 0;

		memcpy(&shm_pointer[index_of_write_index * 4], &write_index, 4);	// write the write_index value to the shm buffer
		memcpy(&shm_pointer[index_of_read_index * 4], &read_index, 4);		// write the read_index value to the shm buffer
	}
	else
	{
		memcpy(&write_index, &shm_pointer[index_of_write_index * 4], 4);	// read the write_index value from the shm buffer
		memcpy(&read_index, &shm_pointer[index_of_read_index * 4], 4);		// read the read_index value from the shm buffer
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
//	printf("writing the word %x\n", word);

	// wait if the write index would catch up to the read index
	while (((write_index + 1 > shm_size - 2) ? 0 : write_index + 1) == read_index)
	{
		memcpy(&read_index, &shm_pointer[index_of_read_index * 4], 4);		// read the read_index value from the shm buffer
	}

	// do the write
	memcpy(&shm_pointer[write_index * 4], &word, 4);

	// update the write pointer
	write_index += 1;
	cur_size += 1 * 4;

	// check if the write_index must wrap
	if (write_index > index_of_upper_bound)
	{
		write_index = 0;
	}

	memcpy(&shm_pointer[index_of_write_index * 4], &write_index, 4);		// write the write_index value to the shm buffer

	this->dump();
}

uint32_t EmuShm::read32()
{
	uint32_t word;

	// wait if the read pointer has caught up to the write pointer
	while (read_index == write_index)
	{
		memcpy(&write_index, &shm_pointer[index_of_write_index * 4], 4);	// read the write_index value from the shm buffer
	}

	// do the read
	memcpy(&word, &shm_pointer[read_index * 4], 4);

	// update the read pointer
	read_index += 1;
	cur_size -= 1 * 4;

	// check if the read_index must wrap
	if (read_index > index_of_upper_bound)
	{
		read_index = 0;
	}

	memcpy(&shm_pointer[index_of_read_index * 4], &read_index, 4);			// write the read_index value to the shm buffer

//	printf("read the word %x\n", word);

	return word;
}

bool EmuShm::isEmpty()
{
	if (cur_size == 0)
	{
		return true;
	}
	return false;
}

void EmuShm::dump()
{
	for (uint32_t i = 0; i < shm_size / 4; i++)
	{
		std::cout << "[" << i << "]\t\t0x" << std::hex << (uint32_t) *((uint32_t*) &shm_pointer[i * 4]) << std::endl;
	}
}
