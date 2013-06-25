#ifndef USERMEMORY_H_
#define USERMEMORY_H_

/********************************************************************
 * 
 * October 10th, 2006
 * Guillermo Marcus - Universitaet Mannheim
 * 
 * $Revision: 1.2 $
 * $Date: 2006-11-17 18:50:17 $
 * 
 *******************************************************************/

/*******************************************************************
 * Change History:
 * 
 * $Log: not supported by cvs2svn $
 * Revision 1.1  2006/10/13 17:18:34  marcus
 * Implemented and tested most of C++ interface.
 *
 *******************************************************************/

#include "SpecDevice.h"

namespace specDriver {

class UserMemory {
	friend class SpecDevice;
private:
	struct sg_entry {
		unsigned long addr;
		unsigned long size;
	};
	
protected:
	unsigned long vma;
	unsigned long size;
	int handle_id;
	SpecDevice *device;
	int nents;
	struct sg_entry *sg;

	UserMemory(SpecDevice& device, void *mem, unsigned int size, bool merged );
public:
	~UserMemory();
	
	enum sync_dir {
		BIDIRECTIONAL = 0,
		TO_DEVICE = 1,
		FROM_DEVICE = 2
	};
	
	void sync(sync_dir dir);

	inline unsigned int getSGcount() { return nents; }	
	inline unsigned long getSGentryAddress(unsigned int entry ) { return sg[entry].addr; }
	inline unsigned long getSGentrySize(unsigned int entry ) { return sg[entry].size; }
	
};

}

#endif /*USERMEMORY_H_*/
