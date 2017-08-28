/*
 * Author: N. Whallon <alokin@uw.edu>
 * Date: 2017-I
 * Description: a class to facilitate communication between programs using shared memory
 */

#include "EmuShm.h"

EmuShm::EmuShm(key_t key, uint32_t size, bool _create)
{
    create = _create;

    shm_key = key;
    shm_size = size;
    element_size = sizeof(uint32_t);

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

    // TODO check for minimum size, else seg fault
    // initialize these indices to be at the back of the shm buffer
    index_of_upper_bound = shm_size - (3 * element_size);
    index_of_write_index = shm_size - (2 * element_size);
    index_of_read_index = shm_size - (1 * element_size);

    if (create)
    {
        write_index = 0;
        read_index = 0;

        memcpy(&shm_pointer[index_of_write_index], &write_index, element_size);	// write the write_index value to the shm buffer
        memcpy(&shm_pointer[index_of_read_index], &read_index, element_size);		// write the read_index value to the shm buffer
    }
    else
    {
        memcpy(&write_index, &shm_pointer[index_of_write_index], element_size);	// read the write_index value from the shm buffer
        memcpy(&read_index, &shm_pointer[index_of_read_index], element_size);		// read the read_index value from the shm buffer
    }
}

EmuShm::~EmuShm()
{
    if (create)
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
}

void EmuShm::write32(uint32_t word)
{
    //printf("writing the word 0x%x (%d, %d)\n", word, write_index, read_index);

    // wait if the write index would catch up to the read index
    while (((write_index + element_size > index_of_upper_bound) ? 0 : write_index + element_size) == read_index)
    {
        memcpy(&read_index, &shm_pointer[index_of_read_index], element_size);		// read the read_index value from the shm buffer
    }

    // do the write
    memcpy(&shm_pointer[write_index], &word, element_size);

    // update the write pointer
    write_index += element_size;

    // check if the write_index must wrap
    if (write_index > index_of_upper_bound)
    {
        write_index = 0;
    }

    memcpy(&shm_pointer[index_of_write_index], &write_index, element_size);		// write the write_index value to the shm buffer
}

uint32_t EmuShm::read32()
{
    uint32_t word;

    // wait if the read pointer has caught up to the write pointer
    while (read_index == write_index)
    {
        memcpy(&write_index, &shm_pointer[index_of_write_index], element_size);	// read the write_index value from the shm buffer
    }

    // do the read
    memcpy(&word, &shm_pointer[read_index], element_size);

    // update the read pointer
    read_index += element_size;

    // check if the read_index must wrap
    if (read_index > index_of_upper_bound)
    {
        read_index = 0;
    }

    memcpy(&shm_pointer[index_of_read_index], &read_index, element_size);			// write the read_index value to the shm buffer

//    printf("read the word 0x%x\n", word);

    return word;
}

uint32_t EmuShm::readBlock32(uint32_t* buf, uint32_t length) {
    if ((length*element_size) > this->getCurSize()) {
        std::cerr << __PRETTY_FUNCTION__ 
            << " -> ERROR : not enough data in buffer! This should not be possible! Requested: " 
            << length*element_size << ", actual: " << this->getCurSize() << std::endl;
        return 0;
    }
    //     
    // 0 1 2 3 4 5 6 7
    //     w     r | | 
    if (read_index+length*element_size > (index_of_upper_bound)) {
        //split transfer
        // length of first part

        uint32_t length_p1 = index_of_upper_bound - read_index + element_size;
        uint32_t length_p2 = length*element_size - length_p1;
        memcpy(&buf[0], &shm_pointer[read_index], length_p1);
        memcpy(&buf[(length_p1)/element_size], &shm_pointer[0], length_p2);
    } else {
        //one transfer
        memcpy(&buf[0], &shm_pointer[read_index], element_size*length);
    }
    // update the read pointer

    read_index += element_size*length;

    // check if the read_index must wrap
    if (read_index > index_of_upper_bound)
    {
        read_index = read_index-index_of_upper_bound-element_size;
    }

    // write the read_index value to the shm buffer
    memcpy(&shm_pointer[index_of_read_index], &read_index, element_size);
    
    return 1;
}


bool EmuShm::isEmpty()
{
    // Update write and read index values
    memcpy(&write_index, &shm_pointer[index_of_write_index], element_size);
    memcpy(&read_index, &shm_pointer[index_of_read_index], element_size);
    if (write_index == read_index)
    {
        return true;
    }
    return false;
}

uint32_t EmuShm::getCurSize() {
    memcpy(&write_index, &shm_pointer[index_of_write_index], element_size);
    memcpy(&read_index, &shm_pointer[index_of_read_index], element_size);
    return ((write_index - read_index) + (shm_size - 2 * element_size)) % (shm_size-2 * element_size);
}

void EmuShm::dump()
{
    for (uint32_t i = 0; i < shm_size / element_size; i++)
    {
        std::cout << "[" << i << "]\t\t0x" << std::hex << (uint32_t) *((uint32_t*) &shm_pointer[i * element_size]) << std::dec << std::endl;
    }
}
