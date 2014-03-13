#ifndef PD_SPECDEVICE_H_
#define PD_SPECDEVICE_H_
// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: SPEC cpp library
// # Comment: Original driver taken from Marcus Guillermo
// #          Modified for SPEC card
// ################################

#include <pthread.h>
#include <fcntl.h>

namespace specDriver {

// Forward references
class KernelMemory;
class UserMemory;
	
class SpecDevice {
private:
	unsigned int pagesize;
	unsigned int pageshift;
	unsigned int pagemask;
    struct flock filelock;	
protected:
	int handle;
	int device;
	char name[50];
	pthread_mutex_t mmap_mutex;
public:
	SpecDevice(int number);
	~SpecDevice();
	
	void open();
	void close();

	int getHandle();
	unsigned short getBus();
	unsigned short getSlot();

	KernelMemory& allocKernelMemory( unsigned int size );
	UserMemory& mapUserMemory( void *mem, unsigned int size, bool merged );
	inline UserMemory& mapUserMemory( void *mem, unsigned int size ) 
		{ return mapUserMemory(mem,size,true); }

	inline void mmap_lock() { pthread_mutex_lock( &mmap_mutex ); }
	inline void mmap_unlock() { pthread_mutex_unlock( &mmap_mutex ); }

    int waitForInterrupt(unsigned int int_id);
	void clearInterruptQueue(unsigned int int_id);
	
	unsigned int getBARsize(unsigned int bar);
	void *mapBAR(unsigned int bar);
	void unmapBAR(unsigned int bar, void *ptr);
	
	unsigned char readConfigByte(unsigned int addr);
	unsigned short readConfigWord(unsigned int addr);
	unsigned int readConfigDWord(unsigned int addr);
	
	void writeConfigByte(unsigned int addr, unsigned char val);
	void writeConfigWord(unsigned int addr, unsigned short val);
	void writeConfigDWord(unsigned int addr, unsigned int val);
};
	
}

#endif /*PD_SPECDEVICE_H_*/
