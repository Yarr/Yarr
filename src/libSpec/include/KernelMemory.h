#ifndef KERNELMEMORY_H_
#define KERNELMEMORY_H_

/********************************************************************
 * 
 * October 10th, 2006
 * Guillermo Marcus - Universitaet Mannheim
 * 
 * $Revision: 1.1 $
 * $Date: 2006-10-13 17:18:34 $
 * 
 *******************************************************************/

#include "SpecDevice.h"

namespace specDriver {

class KernelMemory {
	friend class SpecDevice;
	
protected:
	unsigned long pa;
	unsigned long size;
	int handle_id;
	void *mem;
	SpecDevice *device;

	KernelMemory(SpecDevice& device, unsigned int size);
public:
	~KernelMemory();

	/**
	 *
	 * @returns the physical address of the kernel memory.
	 *
	 */
	inline unsigned long getPhysicalAddress() { return pa; }
	/**
	 *
	 * @returns the size of the kernel memory.
	 *
	 */
	inline unsigned long getSize() { return size; }
	/**
	 *
	 * @returns the pointer to the memory.
	 *
	 */
	inline void *getBuffer() { return mem; }

	enum sync_dir {
		BIDIRECTIONAL = 0,
		TO_DEVICE = 1,
		FROM_DEVICE = 2
	};

	void sync(sync_dir dir);
};
	
}

#endif /*KERNELMEMORY_H_*/
