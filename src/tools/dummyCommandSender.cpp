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

#define SHM_SIZE 928
#define COMMAND_SIZE 92

int shm_command_id;
key_t shm_command_key;
char *shm_command_pointer;

void interrupt_received(int signum)
{
	// detach the shared memory
	shmdt(shm_command_pointer);

	// remove the shared memory
	struct shmid_ds shm_id_ds;

	if (shmctl(shm_command_id, IPC_RMID, &shm_id_ds) == -1)
	{
		fprintf(stderr, "shmctl failed");
	}

	exit(signum);
}

int main(int argc, char *argv[])
{
	signal(SIGINT, interrupt_received);

	// set up the data for the shared memory space to send commands to the emulator
	// make the key something random
	shm_command_key = 1991;

	// create the shared memory segment using the key
	if ((shm_command_id = shmget(shm_command_key, SHM_SIZE, IPC_CREAT | 0666)) < 0)
	{
		fprintf(stderr, "shmget failure\n");
		return 1;
	}

	// attach the shared memory segment to our data space
	if ((shm_command_pointer = (char *) shmat(shm_command_id, NULL, 0)) == (char *) -1)
	{
		fprintf(stderr, "shmat failure\n");
		return 1;
	}

	int32_t write_pointer_position;
	int32_t read_pointer_position;

	int command_num;
//	FILE * command_file = fopen("/home/nwhallon/Programming/git/YARR/src/test_raw_command.txt", "r");
	FILE * command_file = fopen("/home/nwhallon/Programming/git/YARR/src/digitalscan_onemaskstep_raw_commands.txt", "r");
	fscanf(command_file, "%d", &command_num);

	for (int i = 0; i < command_num; i++)
	{
		uint32_t command;
		uint32_t value;
		uint32_t bitstream[21];

		fscanf(command_file, "%x", &command);
		fscanf(command_file, "%x", &value);
		for (int j = 0; j < 21; j++)
		{
			fscanf(command_file, "%x", &bitstream[j]);
		}

		printf("%x\n", command);
		printf("%x\n", value);
		for (int j = 0; j < 21; j++)
		{
			printf("%x\n", bitstream[j]);
		}

		// write the first command, and write the positions of the read and write pointers
		if (i == 0)
		{
			write_pointer_position = 8;

			memcpy(&shm_command_pointer[write_pointer_position + 0], &command, 4);
			memcpy(&shm_command_pointer[write_pointer_position + 4], &value, 4);
			memcpy(&shm_command_pointer[write_pointer_position + 8], &bitstream, 84);

			write_pointer_position += COMMAND_SIZE;
			memcpy(&shm_command_pointer[0], &write_pointer_position, 4);

			read_pointer_position = 8;
			memcpy(&shm_command_pointer[4], &read_pointer_position, 4);
		}
		else
		{
			// get the read pointer position
			memcpy(&read_pointer_position, &shm_command_pointer[4], 4);

//			sleep(1);

			printf("1 write_pointer_position = %d\n", write_pointer_position);
			printf("1 read_pointer_position = %d\n", read_pointer_position);

			// if it is safe to write a new command
			if ((write_pointer_position * read_pointer_position > 0 && abs(write_pointer_position) > abs(read_pointer_position)) || \
			    (write_pointer_position * read_pointer_position < 0 && abs(write_pointer_position) < abs(read_pointer_position)))
			{
				// write the command
				memcpy(&shm_command_pointer[abs(write_pointer_position) + 0], &command, 4);
				memcpy(&shm_command_pointer[abs(write_pointer_position) + 4], &value, 4);
				memcpy(&shm_command_pointer[abs(write_pointer_position) + 8], &bitstream, 84);

				// update the write pointer position, as long as it won't catch up to the read pointer position
				while (1)
				{
//					printf("2 write_pointer_position = %d\n", write_pointer_position);
//					printf("2 read_pointer_position = %d\n", read_pointer_position);

					int32_t tmp_write_pointer_position = write_pointer_position;
					if (tmp_write_pointer_position > 0)
					{
						tmp_write_pointer_position += COMMAND_SIZE;
					}
					if (tmp_write_pointer_position < 0)
					{
						tmp_write_pointer_position -= COMMAND_SIZE;
					}

					// deal with loop around the buffer
					if (abs(tmp_write_pointer_position) >= SHM_SIZE)
					{
						if (tmp_write_pointer_position > 0)
						{
							tmp_write_pointer_position = -8;
						}
						else if (tmp_write_pointer_position < 0)
						{
							tmp_write_pointer_position = 8;
						}
					}

					// see if the new position is the same as the read pointer position
					if (abs(tmp_write_pointer_position) != abs(read_pointer_position))
					{
						write_pointer_position = tmp_write_pointer_position;
						break;
					}

					// grab the read pointer position again to see if it has moved further
//					printf("waiting for the read pointer to move\n");
//					printf("3 write_pointer_position = %d\n", write_pointer_position);
//					printf("3 read_pointer_position = %d\n", read_pointer_position);
//					sleep(1);
					memcpy(&read_pointer_position, &shm_command_pointer[4], 4);
				}

				// write the write pointer position to the shared memory
				memcpy(&shm_command_pointer[0], &write_pointer_position, 4);
			}
		}
	}

	printf("finished writing commands\n");

	while(1);

	return 0;
}
