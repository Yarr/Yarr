// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: SPEC cpp library
// # Comment: Original driver taken from Marcus Guillermo
// #          Modified for SPEC card
// ################################

#include "specDriver.h"
#include "SpecDevice.h"
#include "Exception.h"
#include "KernelMemory.h"
#include "UserMemory.h"

#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/file.h>
#include <errno.h>
#include <iostream>

using namespace specDriver;

/**
 *
 * Construtor for the SpecDevice. Checks if the specified device exists and initializes
 * pagemask, pageshift and the mmap_mutex.
 *
 * @param number Number of the device, e.g. 0 for /dev/fpga0
 *
 */
SpecDevice::SpecDevice(int number)
{
	struct stat tmp_stat = {0};
	
	unsigned int temp = 0;
	
	device = number;
	snprintf(name, sizeof(name), "/dev/spec%d", number);

	if (stat(name, &tmp_stat) < 0)
		throw Exception( Exception::DEVICE_NOT_FOUND );

	pthread_mutex_init(&mmap_mutex, NULL);

	handle = -1;

	pagesize = getpagesize();

	// set pagemask and pageshift
	for( pagemask=0, pageshift=0, temp = pagesize; temp != 1; pageshift++ ) {
		temp = (temp >> 1);
		pagemask = (pagemask << 1)+1;
	}
}

/**
 *
 * Destructor of SpecDevice. Closes the device if it is opened and destroys the mmap_mutex.
 *
 */
SpecDevice::~SpecDevice()
{
	// Close device if open
	if (handle > 0)
		this->close();

	pthread_mutex_destroy(&mmap_mutex);
}

/**
 *
 * Gets the file handle.
 *
 * @returns file handle of the opened PCI device.
 *
 */
int SpecDevice::getHandle()
{
	if (handle == -1)
		throw Exception(Exception::NOT_OPEN);

	return handle;
}

/**
 *
 * Opens the PCI device.
 *
 */
void SpecDevice::open()
{
	int ret = 0;

	/* Check if the device is already opened and exit if yes */
	if (handle != -1)
		return;

	if ((ret = ::open(name, O_RDWR)) < 0)
		throw Exception( Exception::OPEN_FAILED );
		
	handle = ret;

    //if (fcntl(handle, F_GETLK, &filelock))
    //throw Exception( Exception::LOCK_FAILED );
    //filelock.l_type = F_RDLCK | F_WRLCK;
    //if (fcntl(handle, F_SETLK, &filelock))
	//	throw Exception( Exception::LOCK_FAILED );

}

/**
 *
 * Close the PCI device.
 *
 */
void SpecDevice::close()
{
	// do nothing, pass silently if closing a non-opened device.
	if (handle != -1) {
        // Unlock
        filelock.l_type = F_UNLCK;
        fcntl(handle, F_UNLCK, &filelock);
        // Close device
		::close(handle);
    }
	
	handle = -1;
}

/**
 *
 * Allocates kernel memory of the specified size.
 *
 * @param size How much memory to allocate
 * @returns A KernelMemory object
 * @see KernelMemory
 *
 */
KernelMemory& SpecDevice::allocKernelMemory(unsigned int size)
{
	KernelMemory *km = new KernelMemory(*this, size);
	
	return *km;
}

/**
 *
 * Maps user memory of the specified size.
 *
 * @returns A UserMemory object
 * @see UserMemory
 *
 */
UserMemory& SpecDevice::mapUserMemory(void *mem, unsigned int size, bool merged)
{
	UserMemory *um = new UserMemory(*this, mem, size, merged);
	
	return *um;
}

/**
 *
 * Waits for an interrupt.
 *
 */
int SpecDevice::waitForInterrupt(unsigned int int_id)
{
	if (handle == -1)
		throw Exception(Exception::NOT_OPEN);
	
	int ret = ioctl(handle, SPECDRIVER_IOC_WAITI, int_id);
    if (ret < 0)
		throw Exception(Exception::INTERRUPT_FAILED);

    return ret;
}

/**
 *
 * Clears the interrupt queue.
 *
 */
void SpecDevice::clearInterruptQueue(unsigned int int_id)
{
	if (handle == -1)
		throw Exception( Exception::NOT_OPEN );
	
	if(ioctl(handle, SPECDRIVER_IOC_CLEAR_IOQ, int_id) != 0 )
		throw Exception(Exception::INTERNAL_ERROR);
}

/**
 *
 * Gets the size of a BAR.
 *
 * @returns the size of the given BAR
 *
 */
unsigned int SpecDevice::getBARsize(unsigned int bar)
{
	pci_board_info info = {0};

	if (handle == -1)
		throw Exception( Exception::NOT_OPEN );

	if (bar > 5)
		throw Exception( Exception::INVALID_BAR );

	if (ioctl(handle, SPECDRIVER_IOC_PCI_INFO, &info) != 0)
		throw Exception( Exception::INTERNAL_ERROR );
		
	return info.bar_length[ bar ];	
}

/**
 *
 * Gets the bus ID of the PCI device
 *
 */
unsigned short SpecDevice::getBus() 
{
	pci_board_info info = {0};

	if (handle == -1)
		throw Exception(Exception::NOT_OPEN);

	if (ioctl(handle, SPECDRIVER_IOC_PCI_INFO, &info) != 0)
		throw Exception(Exception::INTERNAL_ERROR);

	return info.bus;
}

/**
 *
 * Gets the slot of the PCI device
 *
 */
unsigned short SpecDevice::getSlot()
{
	pci_board_info info = {0};

	if (handle == -1)
		throw Exception(Exception::NOT_OPEN);

	if (ioctl(handle, SPECDRIVER_IOC_PCI_INFO, &info) != 0)
		throw Exception(Exception::INTERNAL_ERROR);

	return info.slot;
}

/**
 *
 * Map the specified BAR.
 *
 * @param bar Which BAR to map (1-5).
 * @returns A pointer to the mapped bar.
 *
 */
void *SpecDevice::mapBAR(unsigned int bar)
{
	void *mem = NULL;
	pci_board_info info = {0};

	if (handle == -1)
		throw Exception(Exception::NOT_OPEN);

	if (bar > 5)
		throw Exception(Exception::INVALID_BAR);

	if (ioctl(handle, SPECDRIVER_IOC_PCI_INFO, &info) != 0)
		return NULL;

	/* Mmap */
	/* This is not fully safe, as a separate process can still open the device independently.
	 * That will use a separate mutex and the race condition can arise.
	 * Posible fix: Do not allow the driver for mutliple openings of a device */
	mmap_lock();

	if (ioctl(handle, SPECDRIVER_IOC_MMAP_MODE, SPECDRIVER_MMAP_PCI) != 0)
		throw Exception(Exception::INTERNAL_ERROR);

	if (ioctl( handle, SPECDRIVER_IOC_MMAP_AREA, SPECDRIVER_BAR0+bar) != 0)
		throw Exception(Exception::INTERNAL_ERROR);

	mem = mmap(0, info.bar_length[bar], PROT_WRITE | PROT_READ, MAP_SHARED, handle, 0);
	
	mmap_unlock();

	if ((mem == MAP_FAILED) || (mem == NULL))
		throw Exception(Exception::MMAP_FAILED);

	// check if the BAR is not page aligned. If it is not, adjust the pointer
	unsigned int offset = info.bar_start[bar] & pagemask;
	
	// adjust pointer
	if (offset != 0) {
		unsigned char* ptr = static_cast<unsigned char *>(mem);
		ptr += offset;
		mem = static_cast<void *>(ptr);
	}
		
	return mem;
}

/**
 *
 * Unmap the specified bar.
 *
 */
void SpecDevice::unmapBAR(unsigned int bar, void *ptr)
{
	pci_board_info info = {0};

	if (handle == -1)
		throw Exception(Exception::NOT_OPEN);

	if (bar > 5)
		throw Exception(Exception::INVALID_BAR);

	if (ioctl(handle, SPECDRIVER_IOC_PCI_INFO, &info) != 0)
		throw Exception(Exception::INVALID_BAR);

	unsigned int offset = info.bar_start[bar] & pagemask;
	
	// adjust pointer
	if (offset != 0) {
		unsigned long tmp = reinterpret_cast<unsigned long>(ptr);
		tmp -= offset;
		ptr = reinterpret_cast<void *>(tmp);
	}

	munmap(ptr, info.bar_length[bar]);
}
	
unsigned char SpecDevice::readConfigByte(unsigned int addr)
{
	pci_cfg_cmd cmd;

	if (handle == -1)
		throw Exception( Exception::NOT_OPEN );

	cmd.addr = addr;	
	cmd.size = SPECDRIVER_PCI_CFG_SZ_BYTE;
	ioctl( handle, SPECDRIVER_IOC_PCI_CFG_RD, &cmd );
	
	return cmd.val.byte;
}

unsigned short SpecDevice::readConfigWord(unsigned int addr)
{
	pci_cfg_cmd cmd;

	if (handle == -1)
		throw Exception( Exception::NOT_OPEN );

	cmd.addr = addr;	
	cmd.size = SPECDRIVER_PCI_CFG_SZ_WORD;
	ioctl( handle, SPECDRIVER_IOC_PCI_CFG_RD, &cmd );
	
	return cmd.val.word;
}

unsigned int SpecDevice::readConfigDWord(unsigned int addr)
{
	pci_cfg_cmd cmd;

	if (handle == -1)
		throw Exception( Exception::NOT_OPEN );

	cmd.addr = addr;	
	cmd.size = SPECDRIVER_PCI_CFG_SZ_DWORD;
	ioctl( handle, SPECDRIVER_IOC_PCI_CFG_RD, &cmd );
	
	return cmd.val.dword;
}
	
void SpecDevice::writeConfigByte(unsigned int addr, unsigned char val)
{
	pci_cfg_cmd cmd;

	if (handle == -1)
		throw Exception( Exception::NOT_OPEN );

	cmd.addr = addr;	
	cmd.size = SPECDRIVER_PCI_CFG_SZ_BYTE;
	cmd.val.byte = val;
	ioctl( handle, SPECDRIVER_IOC_PCI_CFG_WR, &cmd );
	
	return;
}

void SpecDevice::writeConfigWord(unsigned int addr, unsigned short val)
{
	pci_cfg_cmd cmd;

	if (handle == -1)
		throw Exception( Exception::NOT_OPEN );

	cmd.addr = addr;	
	cmd.size = SPECDRIVER_PCI_CFG_SZ_WORD;
	cmd.val.word = val;
	ioctl( handle, SPECDRIVER_IOC_PCI_CFG_WR, &cmd );
	
	return;
}

void SpecDevice::writeConfigDWord(unsigned int addr, unsigned int val)
{
	pci_cfg_cmd cmd;

	if (handle == -1)
		throw Exception( Exception::NOT_OPEN );

	cmd.addr = addr;	
	cmd.size = SPECDRIVER_PCI_CFG_SZ_DWORD;
	cmd.val.dword = val;
	ioctl( handle, SPECDRIVER_IOC_PCI_CFG_WR, &cmd );
	
	return;
}
