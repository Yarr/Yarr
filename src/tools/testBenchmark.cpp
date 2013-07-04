/*******************************************************************
 * This is a test program for the C++ interface of the 
 * pciDriver library.
 * 
 * $Revision: 1.5 $
 * $Date: 2007-02-09 17:03:09 $
 * 
 *******************************************************************/

/*******************************************************************
 * Change History:
 * 
 * $Log: not supported by cvs2svn $
 * Revision 1.4  2006/11/17 18:56:12  marcus
 * Added optional dump of contents of the BARs.
 *
 * Revision 1.3  2006/10/30 19:38:20  marcus
 * Added test to check buffer contents.
 *
 * Revision 1.2  2006/10/16 16:56:28  marcus
 * Added nice comment at the start.
 *
 *******************************************************************/

#include "Exception.h"
#include "SpecDevice.h"
#include "KernelMemory.h"
#include "UserMemory.h"

#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <string.h>
#include <stdint.h>
#include <sys/time.h>
#include <stdio.h>

#define DMACTRLR 0x0
#define DMASTATR 0x1
#define DMACSTARTR 0x2
#define DMAHSTARTLR 0x3
#define DMAHSTARTHR 0x4
#define DMALENR 0x5
#define DMAHNEXTLR 0x6
#define DMAHNEXTHR 0x7
#define DMAATTRIBR 0x8

using namespace specDriver;
using namespace std;

struct dma_linked_list {
    uint32_t carrier_start;
    uint32_t host_start_l;
    uint32_t host_start_h;
    uint32_t length;
    uint32_t host_next_l;
    uint32_t host_next_h;
    uint32_t attr;
};

enum {
	/* page 106 */
	GNPPCI_MSI_CONTROL	= 0x48,		/* actually, 3 smaller regs */
	GNPPCI_MSI_ADDRESS_LOW	= 0x4c,
	GNPPCI_MSI_ADDRESS_HIGH	= 0x50,
	GNPPCI_MSI_DATA		= 0x54,

	GNPCI_SYS_CFG_SYSTEM	= 0x800,

	/* page 130 ff */
    GNCLK_CSR       = 0x808,
	GNINT_CTRL		= 0x810,
	GNINT_STAT		= 0x814,
	GNINT_CFG_0		= 0x820,
	GNINT_CFG_1		= 0x824,
	GNINT_CFG_2		= 0x828,
	GNINT_CFG_3		= 0x82c,
	GNINT_CFG_4		= 0x830,
	GNINT_CFG_5		= 0x834,
	GNINT_CFG_6		= 0x838,
	GNINT_CFG_7		= 0x83c,
#define GNINT_CFG(x) (GNINT_CFG_0 + 4 * (x))

	/* page 146 ff */
	GNGPIO_BASE = 0xA00,
	GNGPIO_BYPASS_MODE	= GNGPIO_BASE,
	GNGPIO_DIRECTION_MODE	= GNGPIO_BASE + 0x04, /* 0 == output */
	GNGPIO_OUTPUT_ENABLE	= GNGPIO_BASE + 0x08,
	GNGPIO_OUTPUT_VALUE	= GNGPIO_BASE + 0x0C,
	GNGPIO_INPUT_VALUE	= GNGPIO_BASE + 0x10,
	GNGPIO_INT_MASK		= GNGPIO_BASE + 0x14, /* 1 == disabled */
	GNGPIO_INT_MASK_CLR	= GNGPIO_BASE + 0x18, /* irq enable */
	GNGPIO_INT_MASK_SET	= GNGPIO_BASE + 0x1C, /* irq disable */
	GNGPIO_INT_STATUS	= GNGPIO_BASE + 0x20,
	GNGPIO_INT_TYPE		= GNGPIO_BASE + 0x24, /* 1 == level */
	GNGPIO_INT_VALUE	= GNGPIO_BASE + 0x28, /* 1 == high/rise */
	GNGPIO_INT_ON_ANY	= GNGPIO_BASE + 0x2C, /* both edges */

	/* page 158 ff */
	FCL_BASE		= 0xB00,
	FCL_CTRL		= FCL_BASE,
	FCL_STATUS		= FCL_BASE + 0x04,
	FCL_IODATA_IN		= FCL_BASE + 0x08,
	FCL_IODATA_OUT		= FCL_BASE + 0x0C,
	FCL_EN			= FCL_BASE + 0x10,
	FCL_TIMER_0		= FCL_BASE + 0x14,
	FCL_TIMER_1		= FCL_BASE + 0x18,
	FCL_CLK_DIV		= FCL_BASE + 0x1C,
	FCL_IRQ			= FCL_BASE + 0x20,
	FCL_TIMER_CTRL		= FCL_BASE + 0x24,
	FCL_IM			= FCL_BASE + 0x28,
};

void mask_val(void *bar, uint32_t off, uint32_t mask, uint32_t val) {
    uint32_t *addr = (uint32_t*) bar + off;
    uint32_t v = *addr;
    v &= ~mask;
    v |= val;
    *addr = v;
}

void write_val(void *bar, uint32_t off, uint32_t val) {
    uint32_t *addr = (uint32_t*) bar + off;
    *addr = val;
}

uint32_t read_val(void *bar, uint32_t off) {
    uint32_t *addr = (uint32_t*) bar + off;
    return *addr;
}

void setup_gennum(void *gn) {
    std::cout << "Setting up gennum ..." << std::hex << std::endl;
    
    
    std::cout << "0x00 = 0x" << read_val(gn, 0x0) << std::endl;
    std::cout << "GNPCI_SYS_CFG_SYSTEM 0x800 = 0x" << read_val(gn, GNPCI_SYS_CFG_SYSTEM/4) << std::endl;
    std::cout << "GNCLK_CSR 0x808 = 0x" << read_val(gn, GNCLK_CSR/4) << std::endl; 
    
    // Check if 64-bit capable and msi activated
    std::cout << "GNPPCI_MSI_CONTROL = 0x" << read_val(gn,GNPPCI_MSI_CONTROL/4) << std::endl;
    if ((read_val(gn, GNPPCI_MSI_CONTROL/4) & 0x00A55805) != 0x00A55805) {
        std::cout << "INVALID MSI Control ... " << std::endl;
    }
    write_val(gn,GNPPCI_MSI_CONTROL/4, 0x00A55805);
    std::cout << "GNPPCI_MSI_CONTROL = 0x" << read_val(gn,GNPPCI_MSI_CONTROL/4) << std::endl;

    std::cout << "GNPPCI_MSI_DATA = 0x" << read_val(gn,GNPPCI_MSI_DATA/4) << std::endl;
    std::cout << "GNPPCI_MSI_ADDRESS_LOW = 0x" << read_val(gn,GNPPCI_MSI_ADDRESS_LOW/4) << std::endl;
    
    // Reset INTx vectors
    for (int i=0; i<8; i++) write_val(gn, GNINT_CFG(i)/4, 0x0);
    // Configure INTx vector given by MSI_DATA&0x3
    std::cout << "GNINT_CFG(" << (read_val(gn, GNPPCI_MSI_DATA/4)&0x3) << ") = 0x" << read_val(gn,GNINT_CFG(read_val(gn, GNPPCI_MSI_DATA/4)&0x3)/4) << std::endl;
    write_val(gn,GNINT_CFG(read_val(gn, GNPPCI_MSI_DATA/4)&0x3)/4, 0x800c); 
    std::cout << "GNINT_CFG(" << (read_val(gn, GNPPCI_MSI_DATA/4)&0x3) << ") = 0x" << read_val(gn,GNINT_CFG(read_val(gn, GNPPCI_MSI_DATA/4)&0x3)/4) << std::endl;
    std::cout << std::endl; 

    // We are using GPIO8/9 as interrupt, make sure they are not in bypass mode
    std::cout << "GNGPIO_BYPASS_MODE = 0x" << read_val(gn,GNGPIO_BYPASS_MODE/4) << std::endl;
    write_val(gn,GNGPIO_BYPASS_MODE/4, 0x0000); 
    std::cout << "GNGPIO_BYPASS_MODE = 0x" << read_val(gn,GNGPIO_BYPASS_MODE/4) << std::endl;
    std::cout << std::endl; 

    // Set intterupt GPIO 8 and 9 to be in input mode = 1
    std::cout << "GNGPIO_DIRECTION_MODE = 0x" << read_val(gn,GNGPIO_DIRECTION_MODE/4) << std::endl;
    write_val(gn,GNGPIO_DIRECTION_MODE/4, 0xFFFF);
    std::cout << "GNGPIO_DIRECTION_MODE = 0x" << read_val(gn,GNGPIO_DIRECTION_MODE/4) << std::endl;
    std::cout << std::endl; 
    
    // Disable output
    std::cout << "GNGPIO_OUTPUT_ENABLE = 0x" << read_val(gn,GNGPIO_OUTPUT_ENABLE/4) << std::endl;
    write_val(gn,GNGPIO_OUTPUT_ENABLE/4, 0x0000);
    std::cout << "GNGPIO_OUTPUT_ENABLE = 0x" << read_val(gn,GNGPIO_OUTPUT_ENABLE/4) << std::endl;
    std::cout << std::endl; 

    // Edge trigger mode = 0
    std::cout << "GNGPIO_INT_TYPE = 0x" << read_val(gn,GNGPIO_INT_TYPE/4) << std::endl;
	write_val(gn,GNGPIO_INT_TYPE/4, 0x0);
    std::cout << "GNGPIO_INT_TYPE = 0x" << read_val(gn,GNGPIO_INT_TYPE/4) << std::endl;
    std::cout << std::endl; 

    // Trigger on high value = 1
    std::cout << "GNGPIO_INT_VALUE = 0x" << read_val(gn,GNGPIO_INT_VALUE/4) << std::endl;
	write_val(gn,GNGPIO_INT_VALUE/4, 0x300);
    std::cout << "GNGPIO_INT_VALUE = 0x" << read_val(gn,GNGPIO_INT_VALUE/4) << std::endl;
    std::cout << std::endl; 

    // Trigger on edge specified in GNGPIO_INT_TYPE
    std::cout << "GNGPIO_INT_ON_ANY = 0x" << read_val(gn,GNGPIO_INT_ON_ANY/4) << std::endl;
	write_val(gn,GNGPIO_INT_ON_ANY/4, 0x0);
    std::cout << "GNGPIO_INT_ON_ANY = 0x" << read_val(gn,GNGPIO_INT_ON_ANY/4) << std::endl;
    std::cout << std::endl;
    
    // Enable our IRQ signals as interrupt and disable all others
    std::cout << "GNGPIO_INT_MASK = 0x" << read_val(gn,GNGPIO_INT_MASK/4) << std::endl;
	write_val(gn,GNGPIO_INT_MASK_SET/4, 0xFFFF);
    write_val(gn,GNGPIO_INT_MASK_CLR/4, 0x0300);
    std::cout << "GNGPIO_INT_MASK = 0x" << read_val(gn,GNGPIO_INT_MASK/4) << std::endl;
    std::cout << std::endl;

    std::cout << "GNINT_CTRL = 0x" << read_val(gn,GNINT_CTRL/4) << std::endl;
    std::cout << "GNGPIO_INT_STATUS = 0x" << read_val(gn,GNGPIO_INT_STATUS/4) << std::endl;
    write_val(gn,GNINT_STAT/4, 0xFFF0); //Clear IRQ sources
    std::cout << "GNINT_STAT = 0x" << read_val(gn,GNINT_STAT/4) << std::endl;
    std::cout << std::endl;
    
    // Reset GPIO INT STATUS
    std::cout << "GNGPIO_INT_STATUS = 0x" << read_val(gn,GNGPIO_INT_STATUS/4) << std::endl;
    std::cout << std::endl << "... Gennum configured!" << std::endl << std::endl;
    
    sleep(1);

    return;
}

struct dma_linked_list* prep_dma(UserMemory *um, KernelMemory *ll, int write) {
    struct dma_linked_list *llist = (struct dma_linked_list*) ll->getBuffer();
    std::cout << "DMA Linked list at: 0x" << std::hex << ll->getBuffer() << std::endl;
    uint32_t dev_off = 0;
    unsigned int j = 0;
    for (unsigned int i=0; i<um->getSGcount(); i++) {
        int sg_size = um->getSGentrySize(i);
        uint32_t sg_addr_h = ((uint64_t)um->getSGentryAddress(i) >> 32);
        uint32_t sg_addr_l = ((um->getSGentryAddress(i)) & 0xFFFFFFFF);
        do {
            uint32_t fixed_size = sg_size;
            if (sg_size > 4096) fixed_size = 4096;
            // FIXME How do we propagate the carrier address??
            //llist[j].carrier_start = dev_off; // Always start at 0x0in FPGA RAM
            llist[j].carrier_start = 0; // Always start at 0x0in FPGA RAM
            dev_off += fixed_size/4;;
            llist[j].host_start_l = sg_addr_l;
            llist[j].host_start_h = sg_addr_h;
            llist[j].length = fixed_size;
            uint64_t next = ll->getPhysicalAddress();
            next += (sizeof(struct dma_linked_list) * ( j + 1 ));
            llist[j].host_next_l = (uint32_t)((uint64_t)next & 0xFFFFFFFF);
            llist[j].host_next_h = (uint32_t)((uint64_t)next >> 32);
            llist[j].attr = 0x1 + (write << 1); // L2P, not last
#if 1
            std::cout << "Linked List Entry [" << std::dec << j << std::hex << "]:" << std::endl;
            std::cout << "  Carrier Start: 0x" << llist[j].carrier_start << std::endl;
            std::cout << "  Host Start H:  0x" << llist[j].host_start_h << std::endl;
            std::cout << "  Host Start L:  0x" << llist[j].host_start_l << std::endl;
            std::cout << "  Length:        " << std::dec << llist[j].length << std::hex << std::endl;
            std::cout << "  Host Next L    0x" << llist[j].host_next_l << std::endl;
            std::cout << "  Host Next H    0x" << llist[j].host_next_h << std::endl;
            std::cout << "  Attribute      0x" << llist[j].attr << std::endl;
#endif
            sg_size = sg_size - 4096;
            sg_addr_l = sg_addr_l + 4096; // FIXME: Can this overflow ?
            j++;
        } while (sg_size > 0);
    }
    if (j>148) j = 148;
    // Mark last item
    llist[j-1].host_next_l = 0x0;
    llist[j-1].host_next_h = 0x0;
    llist[j-1].attr = 0x0 + (write << 1); // last item
#if 1
    std::cout << "Modified Last Item[" << std::dec << j-1 << "]" << std::hex << std::endl;
    std::cout << "  Host Next L    0x" << llist[j-1].host_next_l << std::endl;
    std::cout << "  Host Next H    0x" << llist[j-1].host_next_h << std::endl;
    std::cout << "  Attribute      0x" << llist[j-1].attr << std::endl;// L2P, last item
#endif
    std::cout << std::dec; 
    ll->sync(KernelMemory::BIDIRECTIONAL);
    return llist;
}

int main() 
{
    
	specDriver::SpecDevice *device;
	try {
		cout << "Trying device " << 0 << " ... ";
        device = new specDriver::SpecDevice( 0 );
		cout << "found" << endl;
	} catch (Exception& e) {
		cout << "failed: " << e.toString() << endl;
		return 1;
	}

    device->open();  

    int size;
    void *bar = NULL;
    void *gn = NULL;

    cout << "Mapping BAR " << "0 and 4" << " ... " << std::endl;
    try {
        bar = device->mapBAR(0);
        gn = device->mapBAR(4);
        size = device->getBARsize(0);
        cout << std::hex << "Bar[0] = 0x" << size << std::dec << std::endl;
        size = device->getBARsize(4);
        cout << std::hex << "Bar[4] = 0x" << size << std::dec << std::endl;
    } catch (Exception& e) {
        cout << "failed" << endl;
    }

    //uint32_t *addr;
    setup_gennum(gn);
    
    timeval start, end, start_outer, end_outer;
    device->clearInterruptQueue(0);

#if 0
    // Single Read/Write
    printf("Starting write benchmark ...\n");
    for (int cycles=0; cycles<10; cycles++) {
        const unsigned int size = 64;
        uint32_t value[size];
        for (uint32_t i=0; i<size; i++) value[i] = i+0x100;
        //256 byte:
        addr = (uint32_t *)bar+0x20000;
        //printf("Old 0x%p , New 0x%p\n", access_address, addr);
        //256 MB total
        int loops = 1024*100;
        start=clock();
        for (int i=0; i<loops; i++) {
            memcpy(addr, value, size*4);
        }
        end=clock();
        double data = loops*size*4/1024/1024;
        double time = ((double)(end-start))/CLOCKS_PER_SEC;
        printf("Transferred %f MB in %f s, throughput = %f MB/s\n", data, time, data/time); 
    }

    printf("Starting read benchmark ...\n");
    for (int cycles=0; cycles<3; cycles++) {
        const unsigned int size = 64;
        uint32_t value[size];
        //printf("Old 0x%p , New 0x%p\n", access_address, addr);
        int loops = 1024*10;
        start=clock();
        for (int i=0; i<loops; i++) {
            addr = (uint32_t *)bar+0x10000;
            for (unsigned int length=0; length<size; length++) { 
                addr = (uint32_t *)bar + 0x10000 + length;
                value[length] = *addr;
            }
        }
        end=clock();
        double data = loops*size*4/1024/1024;
        double time = ((double)(end-start))/CLOCKS_PER_SEC;
        printf("Read:\t"); 
        for (uint32_t i=0; i<size; i++)  {
            printf("%x\t", value[i]);
            if ((i+1)%8 == 0) printf("\n\t");
            value[i] = value[i]<<1;
        }
        printf("\n");
        printf("Transferred %f MB in %f s, throughput = %f MB/s\n", data, time, data/time); 
    }
#endif
    
    // DMA
    std::cout << "Starting DMA test ..." << std::endl;
    size_t data_size = 1024*500;
    gettimeofday(&start_outer, NULL);
    for (int cycles=1; cycles<50; cycles++) {
        //data_size = 1024*(cycles*cycles*10);
        data_size = 1024*(cycles*10)+1024*500;
        uint32_t data[data_size/4];
        for(unsigned int i=0; i<data_size/4; i++) data[i] = i;
        //memset(data, 0x5A, data_size);
        uint32_t write = 0x1;

        UserMemory *um = &device->mapUserMemory(data, data_size, false);
        if (um == NULL) {
            std::cout << "Error allocating User Memory" << std::endl;
            break;
        }
        // Need to demerge sg entries
        unsigned int pages = 0;
        for (unsigned int i=0; i<um->getSGcount(); i++) {
#if 1
            std::cout << "SG Entry [" << i << "] :" << std::endl;
            std::cout << "  Address = 0x" << std::hex << um->getSGentryAddress(i) << std::dec << std::endl;
            std::cout << "  Size = " << um->getSGentrySize(i) << std::endl;
#endif
            int entry_size = um->getSGentrySize(i);
            pages++;
            while(entry_size > 4096) {
                entry_size = entry_size - 4096;
                pages++;
            }
        }
        std::cout << "Total Nr of pages: " << pages << std::endl;
        um->sync(UserMemory::BIDIRECTIONAL);

        std::string temp;
        KernelMemory *ll = &device->allocKernelMemory(sizeof(struct dma_linked_list)*pages);
        if (ll == NULL) {
            std::cout << "ERROR allocating Kernel Memory!" << std::endl;
            break;
        }

        struct dma_linked_list *llist = prep_dma(um, ll, write);

        // Copy linked list
        uint32_t *dmactlr = (uint32_t*) bar + DMACSTARTR;
        memcpy(dmactlr, &llist[0], sizeof(struct dma_linked_list));

        //DMA Controller Status
        volatile uint32_t *dst = (uint32_t*) bar;
        uint32_t stat = *(dst+DMASTATR);   
        printf("\nDMA Controller Status: ");
        switch (stat) {
            case 0x0:
                printf("IDLE\n");
                break;
            case 0x1:
                printf("DONE\n");
                break;
            case 0x2:
                printf("BUSY\n");
                break;
            case 0x3:
                printf("ERROR\n");
                break;
            case 0x4:
                printf("ABORTED\n");
                break;
            default:
                printf("UNKNOWN -> 0x%x\n", stat);
                break;
        }
        dst = (uint32_t*) bar;
        if (stat < 0x5 && stat != 0x2) {
            // Start Transfer
            std::cout << "DMA Setup done! Starting tranfer of " << data_size/1024.0 << "kB ..." << std::endl;
            //std::cin >> temp;
            gettimeofday(&start, NULL);
            *(dst+DMACTRLR) = 0x01; // Start DMA transfer
            device->waitForInterrupt(0);
            //sleep(1);
            gettimeofday(&end, NULL);
            std::cout << std::hex;
            std::cout << "GNINT_STAT = 0x" << read_val(gn,GNINT_STAT/4) << std::endl;
            std::cout << "GNGPIO_INT_STATUS = 0x" << read_val(gn,GNGPIO_INT_STATUS/4) << std::endl;
            std::cout << "GNINT_STAT = 0x" << read_val(gn,GNINT_STAT/4) << std::endl;
            std::cout << std::dec;
            double total_data = data_size/1024.0;
            double time = (end.tv_sec - start.tv_sec) * 1000.0; //msecs
            time += (end.tv_usec - start.tv_usec) / 1000.0; //usecs
            printf("Transferred %f kB in %f ms, throughput = %f MB/s\n", total_data, time, total_data/time*1000.0/1024); 
        } else {
            std::cout << "Not ready for tranfer ... aborting" << std::endl;
            *(dst+DMACTRLR) = 0x02; // Abort DMA transfer
            break;
        }
        
        delete ll;
        delete um;
        std::cin >> temp;
    }
    gettimeofday(&end_outer, NULL);
    double time_outer = (end_outer.tv_sec - start_outer.tv_sec) * 1000.0; //msecs
    time_outer += (end_outer.tv_usec - start_outer.tv_usec) / 1000.0; //usecs
    printf("Transferred %f kB in %f ms, throughput = %f MB/s\n", 500.0*49, time_outer, 500.0*49/time_outer*1000.0/1024); 

    // Cleanup
    device->unmapBAR(0, bar);
    device->unmapBAR(4, gn);
    device->close();
    delete device;
	return 0;
}

