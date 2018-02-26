// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: SPEC cpp library
// # Comment: Original driver taken from Marcus Guillermo
// #          Modified for SPEC card
// ################################

#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <iomanip>

#include <SpecCom.h>
#include <GennumRegMap.h>
#include <BitOps.h>


SpecCom::SpecCom() {
    is_initialized = false;
    specId = 0;
}

SpecCom::SpecCom(unsigned int id) {
    specId = id;
    is_initialized = false;
    try {
        this->init();
        this->configure();
    } catch (Exception &e) {
        std::cerr << __PRETTY_FUNCTION__ << " -> " << e.toString() << std::endl;
        std::cerr << __PRETTY_FUNCTION__ <<  " -> Fatal Error! Aborting!"  << std::endl;
        exit(-1);
    }
    is_initialized = true;
}

SpecCom::~SpecCom() {
    spec->unmapBAR(0, bar0);
    if (bar4 != NULL)
        spec->unmapBAR(4, bar4);
    spec->close();
    delete spec;
}

bool SpecCom::isInitialized() {
    return is_initialized;
}

int SpecCom::getId() {
    return specId;
}

int SpecCom::getBarSize(unsigned int bar) {
    return spec->getBARsize(bar);
}

void SpecCom::init(unsigned int id) {
    if (!is_initialized) {
        specId = id;
        try {
            this->init();
            this->configure();
        } catch (Exception &e) {
            std::cerr << __PRETTY_FUNCTION__ << " -> " << e.toString() << std::endl;
            std::cerr << __PRETTY_FUNCTION__ <<  " -> Fatal Error! Aborting!"  << std::endl;
            exit(-1);
        }
        is_initialized = true;
    } else {
        std::cerr << __PRETTY_FUNCTION__ << " -> " << "Device is already initialzed!" << std::endl;
    }
}

void SpecCom::writeSingle(uint32_t off, uint32_t val) {
    this->write32(bar0, off, val);
}

uint32_t SpecCom::readSingle(uint32_t off) {
    volatile uint32_t tmp = this->read32(bar0, off);
    return tmp; 
}

void SpecCom::write32(uint32_t off, uint32_t *val, size_t words) {
    this->write32(bar0, off, val, words);
}

void SpecCom::read32(uint32_t off, uint32_t *val, size_t words) {
    this->read32(bar0, off, val, words);
}


void SpecCom::writeBlock(uint32_t off, uint32_t *val, size_t words) {
    this->writeBlock(bar0, off, val, words);
}

void SpecCom::readBlock(uint32_t off, uint32_t *val, size_t words) {
    this->readBlock(bar0, off, val, words);
}

int SpecCom::writeDma(uint32_t off, uint32_t *data, size_t words) {
    int status = this->getDmaStatus(); 
    if ( status == DMAIDLE || status == DMADONE || status == DMAABORTED) {
        UserMemory *um = &spec->mapUserMemory(data, words*4, false);
        KernelMemory *km = &spec->allocKernelMemory(sizeof(struct dma_linked_list)*um->getSGcount());

        struct dma_linked_list *llist = this->prepDmaList(um, km, off, 1);

        this->writeBlock(bar0, DMACSTARTR, (uint32_t*) &llist[0], sizeof(struct dma_linked_list)/sizeof(uint32_t));
        this->startDma();

        if (spec->waitForInterrupt(0) < 1) {
            std::cerr << __PRETTY_FUNCTION__ << " -> " 
            << "Interrupt timeout, aborting transfer!" << std::endl;
            this->abortDma();
        }

        // Ackowledge interrupt
        if (bar4 != NULL) {
            volatile uint32_t irq_ack = this->read32(bar4, GNGPIO_INT_STATUS/4);
            (void) irq_ack;
        }

        delete km;
        delete um;
        return 0;
    } else {
        std::cerr << __PRETTY_FUNCTION__ << " -> " 
            << "DMA Transfer aborted (Status = 0x" << std::hex << status << std::dec << ")" << std::endl;
        return 1;
    }
}

int SpecCom::readDma(uint32_t off, uint32_t *data, size_t words) {
    int status = this->getDmaStatus(); 
    if ( status == DMAIDLE || status == DMADONE || status == DMAABORTED) {
        UserMemory *um = &spec->mapUserMemory(data, words*4, false);
        KernelMemory *km = &spec->allocKernelMemory(sizeof(struct dma_linked_list)*um->getSGcount());

        struct dma_linked_list *llist = this->prepDmaList(um, km, off, 0);
        
        this->writeBlock(bar0, DMACSTARTR, (uint32_t*) &llist[0], sizeof(struct dma_linked_list)/sizeof(uint32_t));
        this->startDma();

        if (spec->waitForInterrupt(0) < 1) {
            std::cerr << __PRETTY_FUNCTION__ << " -> " 
            << "Interrupt timeout, aborting transfer!" << std::endl;
            this->abortDma();
        }
        
        // Ackowledge interrupt
        if (bar4 != NULL) {
            volatile uint32_t irq_ack = this->read32(bar4, GNGPIO_INT_STATUS/4);
            (void) irq_ack;
        }
        um->sync(UserMemory::BIDIRECTIONAL);

        delete km;
        delete um;
        status = this->getDmaStatus(); 
        if (status == DMAABORTED || status == DMAERROR) {
            std::cerr << __PRETTY_FUNCTION__ << " -> " 
                << "DMA Transfer failed! (Status = 0x" << std::hex << status << std::dec << ")" << std::endl;
            return 1;
        } else {
            return 0;
        }
    } else {
        std::cerr << __PRETTY_FUNCTION__ << " -> " 
            << "DMA Transfer aborted! (Status = 0x" << std::hex << status << std::dec << ")" << std::endl;
        return 1;
    }
}

void SpecCom::init() {
#ifdef DEBUG
    std::cout << __PRETTY_FUNCTION__ << " -> Opening SPEC with id #" << specId << std::endl;
#endif
    // Init SPEC
    try {
        spec = new SpecDevice(specId);
    } catch (Exception &e) {
        std::cerr << __PRETTY_FUNCTION__ << " -> " << e.toString() << std::endl;
        throw Exception(Exception::INIT_FAILED);
        return;
    }
    // Open SPEC
    spec->open();
#ifdef DEBUG
    std::cout << __PRETTY_FUNCTION__ << " -> Mapping BARs" << std::endl;
#endif
    // Map BARs
    try {
        bar0 = spec->mapBAR(0);
#ifdef DEBUG
        std::cout << __PRETTY_FUNCTION__ << " -> Mapped BAR0 at 0x" << std::hex << bar0 
            << " with size 0x" << spec->getBARsize(0) << std::dec << std::endl;
#endif
    } catch (Exception &e) {
        std::cerr << __PRETTY_FUNCTION__ << " -> " << e.toString() << std::endl;
        throw Exception(Exception::INIT_FAILED);
        return;
    }
    try {
        bar4 = spec->mapBAR(4);
#ifdef DEBUG
        std::cout << __PRETTY_FUNCTION__ << " -> Mapped BAR4 at 0x" << std::hex << bar4 
            << " with size 0x" << spec->getBARsize(4) << std::dec << std::endl;
#endif
    } catch (Exception &e) {
        std::cerr << __PRETTY_FUNCTION__ << " -> " << e.toString() << std::endl;
        // TODO check if it's the right firmware
        std::cerr << __PRETTY_FUNCTION__ << " -> Could not map BAR4, this might be OK!" << std::endl;
        bar4 = NULL;
    }
    this->flushDma();
    return;
}

void SpecCom::configure() {
    if (bar4 != NULL) {
#ifdef DEBUG
        std::cout << __PRETTY_FUNCTION__ << " -> Configuring GN412X" << std::endl;
#endif
        // Activate MSI if necessary
        if (read32(bar4, GNPPCI_MSI_CONTROL/4) != 0x00A55805) {
#ifdef DEBUG
            std::cout << __PRETTY_FUNCTION__ << " -> MSI needs to be configured!" << std::endl;
#endif
            this->write32(bar4,GNPPCI_MSI_CONTROL/4, 0x00A55805);
        }
        
        // Reset INTx vectors
        for (int i=0; i<8; i++) this->write32(bar4, GNINT_CFG(i)/4, 0x0);

        // Configure INTx vector given by MSI_DATA&0x3
        this->write32(bar4,GNINT_CFG(this->read32(bar4, GNPPCI_MSI_DATA/4)&0x3)/4, 0x800c); 

        // We are using GPIO8/9 as interrupt, make sure they are not in bypass mode
        this->write32(bar4,GNGPIO_BYPASS_MODE/4, 0x0000); 

        // Set intterupt GPIO 8 and 9 to be in input mode = 1
        this->write32(bar4,GNGPIO_DIRECTION_MODE/4, 0xFFFF);
        
        // Disable output
        this->write32(bar4,GNGPIO_OUTPUT_ENABLE/4, 0x0000);
        
        // Edge trigger mode = 0
        this->write32(bar4,GNGPIO_INT_TYPE/4, 0x0);

        // Trigger on high value = 1
        this->write32(bar4,GNGPIO_INT_VALUE/4, 0x300);

        // Trigger on edge specified in GNGPIO_INT_TYPE
        this->write32(bar4,GNGPIO_INT_ON_ANY/4, 0x0);
        
        // Enable our GPIOs as an interrupt source and disable all others
        this->write32(bar4,GNGPIO_INT_MASK_SET/4, 0xFFFF);
        this->write32(bar4,GNGPIO_INT_MASK_CLR/4, 0x0300);

        // Clear All IRQs
        this->write32(bar4,GNINT_STAT/4, 0xFFF0);
        this->write32(bar4,GNINT_STAT/4, 0x0000);
        volatile uint32_t res1 = this->read32(bar4,GNINT_STAT/4);
        (void) res1;

        // Reset GPIO INT STATUS
        volatile uint32_t res2 = this->read32(bar4,GNGPIO_INT_STATUS/4);
        (void) res2;

        usleep(200);
    }
    // Clear IRQ queues
    spec->clearInterruptQueue(0);
    spec->clearInterruptQueue(1);
}

void SpecCom::write32(void *bar, uint32_t off, uint32_t val) {
    uint32_t *addr = (uint32_t*) bar+off;
    *addr = val;
}

uint32_t SpecCom::read32(void *bar, uint32_t off) {
    uint32_t *addr = (uint32_t*) bar+off;
    return *addr;
}

void SpecCom::mask32(void *bar, uint32_t off, uint32_t mask, uint32_t val) {
    uint32_t *addr = (uint32_t*) bar+off;
    uint32_t tmp = *addr;
    tmp &= ~mask;
    tmp |= val;
    *addr = tmp;
}

void SpecCom::writeBlock(void *bar, uint32_t off, uint32_t *val, size_t words) {
    for (unsigned i=0; i<words; i++) {
        volatile uint32_t *addr = (uint32_t*) bar+off+(i);
        *addr = val[i];
    }
}

void SpecCom::write32(void *bar, uint32_t off, uint32_t *val, size_t words) {
    uint32_t *addr = (uint32_t*) bar+off;
    for (uint32_t i=0; i<words; i++)
        *addr = val[i];
}

void SpecCom::readBlock(void *bar, uint32_t off, uint32_t *val, size_t words) {
    uint32_t *addr = (uint32_t*) bar+off;
    for(unsigned int i=0; i<words; i++) 
        val[i] = *addr;
}

void SpecCom::read32(void *bar, uint32_t off, uint32_t *val, size_t words) {
    uint32_t *addr = (uint32_t*) bar+off;
    for(unsigned int i=0; i<words; i++) val[i] = *addr++;
}

struct dma_linked_list* SpecCom::prepDmaList(UserMemory *um, KernelMemory *km, uint32_t off, bool write) {
    struct dma_linked_list *llist = (struct dma_linked_list*) km->getBuffer();
    uint32_t dev_off = off*4;
    unsigned int j = 0;
    for (unsigned int i=0; i<um->getSGcount(); i++) {
        int sg_size = um->getSGentrySize(i);
        uint32_t sg_addr_h = ((uint64_t)um->getSGentryAddress(i) >> 32);
        uint32_t sg_addr_l = ((um->getSGentryAddress(i)) & 0xFFFFFFFF);
        do {
            uint32_t fixed_size = sg_size;
            if (sg_size > 4096) fixed_size = 4096;
            llist[j].carrier_start = dev_off;
            //llist[j].carrier_start = 0;
            dev_off += fixed_size;
            llist[j].host_start_l = sg_addr_l;
            llist[j].host_start_h = sg_addr_h;
            llist[j].length = fixed_size;
            uint64_t next = km->getPhysicalAddress();
            next += (sizeof(struct dma_linked_list) * ( j + 1 ));
            llist[j].host_next_l = (uint32_t)((uint64_t)next & 0xFFFFFFFF);
            llist[j].host_next_h = (uint32_t)((uint64_t)next >> 32);
            llist[j].attr = 0x1 + (write << 1); // L2P, not last
#if 0
            std::cout << "Linked List Entry [" << std::dec << j << std::hex << "]:" << std::endl;
            std::cout << "  Carrier Start: 0x" << llist[j].carrier_start << std::endl;
            std::cout << "  Host Start H:  0x" << llist[j].host_start_h << std::endl;
            std::cout << "  Host Start L:  0x" << llist[j].host_start_l << std::endl;
            std::cout << "  Length:        " << std::dec << llist[j].length << std::hex << std::endl;
            std::cout << "  Host Next L    0x" << llist[j].host_next_l << std::endl;
            std::cout << "  Host Next H    0x" << llist[j].host_next_h << std::endl;
            std::cout << "  Attribute      0x" << llist[j].attr << std::endl;
#endif
            sg_size = sg_size - fixed_size;
            sg_addr_l = sg_addr_l + fixed_size;
            // Prevent overflow
            sg_addr_h = sg_addr_h + ((uint64_t)(sg_addr_l + fixed_size) >> 32);
            j++;
        } while (sg_size > 0);
    }
    // Mark last item
    llist[j-1].host_next_l = 0x0;
    llist[j-1].host_next_h = 0x0;
    llist[j-1].attr = 0x0 + (write << 1); // last item
#if 0
    std::cout << "Modified Last Item[" << std::dec << j-1 << "]" << std::hex << std::endl;
    std::cout << "  Host Next L    0x" << llist[j-1].host_next_l << std::endl;
    std::cout << "  Host Next H    0x" << llist[j-1].host_next_h << std::endl;
    std::cout << "  Attribute      0x" << llist[j-1].attr << std::endl;// L2P, last item
    std::cout << std::dec;
#endif

    // Sync Memory
    km->sync(KernelMemory::BIDIRECTIONAL);
    return llist;
}

void SpecCom::startDma() {
    uint32_t *addr = (uint32_t*) bar0+DMACTRLR;
    // Set t 0x1 to start DMA transfer
    *addr = 0x1;
}

void SpecCom::abortDma() {
    uint32_t *addr = (uint32_t*) bar0+DMACTRLR;
    // Set t 0x2 to abort DMA transfer
    *addr = 0x2;
}

uint32_t SpecCom::getDmaStatus() {
    uint32_t *addr = (uint32_t*) bar0+DMASTATR;
    uint32_t status = *addr;
#if 0
    std::cout << __PRETTY_FUNCTION__ << " -> DMA Status: ";
    switch (status) {
        case DMAIDLE: std::cout << "DMA IDLE"; break;
        case DMADONE: std::cout << "DMA DONE"; break;
        case DMAABORTED: std::cout << "DMA ABORTED"; break;
        case DMAERROR: std::cout << "DMA ERROR"; break;
        default: std::cout << "UNKNOWN"; break;
    }
    std::cout << std::endl;
#endif
    return status;
}

int SpecCom::progFpga(const void *data, size_t size) {
#ifdef DEBUG
    std::cout << __PRETTY_FUNCTION__ << " -> Setting up programming of FPGA" <<std::endl;
#endif
    int size32 = (size + 3) >> 2;
    const uint32_t *data32 = (uint32_t*)data;

    // Stuff perhaps missing, but in manual
    // FCL_IM -> enable the right 
    
    // Setup BOOT_SEL signals in GPIO 14,15 to 0, 1
    this->mask32(bar4, GNGPIO_DIRECTION_MODE/4, 0xC000, 0);
    this->mask32(bar4, GNGPIO_OUTPUT_ENABLE/4, 0xC000, 0xC000);
    this->mask32(bar4, GNGPIO_OUTPUT_VALUE/4, 0xC000, 0x8000);

    // FCL_CLK_DIV -> 0x0 -> PCLK/2 (PCLK = 125MHz)
    this->write32(bar4, FCL_CLK_DIV/4, 0x1);
    // FCL_CTRL -> 0x40 -> Reset
    this->write32(bar4, FCL_CTRL/4, 0x40);
    // Check reset is high
    if (0x40 != this->read32(bar4, FCL_CTRL/4)) {
        std::cerr << __PRETTY_FUNCTION__ << " -> Error setting FCL_CTRL ... aborting!" << std::endl;
        return -1;
    }

    // FCL_CTRL -> 0x0
    this->write32(bar4, FCL_CTRL/4, 0x0);
   
    // FCL_IRQ -> 0x0 -> Clear pending IRQ
    volatile uint32_t foo = this->read32(bar4, FCL_IRQ/4);
    (void) foo;

    // Get number of uneven bytes in word sense
    uint32_t ctrl = 0;
	switch(size & 3) {
        case 3: ctrl = 0x116; break;
        case 2: ctrl = 0x126; break;
        case 1: ctrl = 0x136; break;
        case 0: ctrl = 0x106; break;
	}
    // Setup FCL CTRL
    // 0x2 - SPRI_EN
    // 0x4 - FSM_EN
    // 0x30 - Last Byte CNT -> (size & 0x3)
    // 0x100 - SPRI_CLK_STOP_EN
    this->write32(bar4, FCL_CTRL/4, ctrl);

    // FCL_TIMER_CTRL -> 0x0
    // FCL_TIMER_0 -> 0x10
    // FCL_TIMER_1 -> 0x0
    this->write32(bar4, FCL_TIMER_CTRL/4, 0x0);
    this->write32(bar4, FCL_TIMER_0/4, 0x10);
    this->write32(bar4, FCL_TIMER_1/4, 0x0);
    
    // Enable the right lines
    // FCL_EN -> 0x17
    this->write32(bar4, FCL_EN/4, 0x17);
    
    // Start FSM
    // FCL_CTRL += 0x1
    ctrl |= 0x1;
    this->write32(bar4, FCL_CTRL/4, ctrl);

#ifdef DEBUG
    std::cout << __PRETTY_FUNCTION__ << " -> Starting programming!" <<std::endl;
#endif
    // Write a bit of data to FCL_FIFO
    int done = 0;
    int wrote = 0;
	while(size32 > 0)
	{
        // Check if FPGA configuration has errors

		int i = read32(bar4, FCL_IRQ/4);
		if ( (i & 8) && wrote) {
		    done = 1;
		} else if ( (i & 0x4) && !done) {
            std::cerr << __PRETTY_FUNCTION__<< " -> Error while programming after " << wrote << " words ... aborting!" << std::endl;
            return -1;
		}

        // Wait until FCL_IRQ & 0x5 = 1
		while (read32(bar4, FCL_IRQ/4)  & (1<<5))
			;

        // Loop
		for (i = 0; size32 && i < 32; i++) {
			write32(bar4, FCL_FIFO/4, BitOps::unaligned_bitswap_le32(data32));
			data32++; size32--; wrote++;
		}
	}
    // FCL_CTRL -> 0x186 (last data written)
    this->write32(bar4, FCL_CTRL/4, 0x186);
#ifdef DEBUG
    std::cout << __PRETTY_FUNCTION__ << " -> Programming done!!" <<std::endl;
#endif
    
    //Wait a bit for the FPGA to boot up
    sleep(1);

    uint32_t irq = read32(bar4, FCL_IRQ/4);
    uint32_t status = read32(bar4, FCL_STATUS/4);
    
    (void) status;

    if (irq & 0x4)
        std::cerr << __PRETTY_FUNCTION__<< " -> FCL IRQ indicates an error, read: 0x" << std::hex << irq << std::dec << std::endl;

#ifdef DEBUG
    std::cout << __PRETTY_FUNCTION__ << " -> FCL IRQ: 0x" << std::hex << irq << std::dec << std::endl;
    if (irq & 0x8)
        std::cout << __PRETTY_FUNCTION__ << " -> FCL IRQ indicates CONFIG_DONE" <<std::endl;
    std::cout << __PRETTY_FUNCTION__ << " -> FCL Status: 0x" << std::hex << status << std::dec << std::endl;
    if (status & 0x8)
        std::cout << __PRETTY_FUNCTION__ << " -> FCL STATUS indicates SPRI_DONE" <<std::endl;
#endif

    return wrote*4;
}


uint32_t SpecCom::readEeprom(uint8_t * buffer, uint32_t len) {

	uint32_t totalTransfer=0;
	unsigned int k = 0;
	unsigned int j = 0;
	uint32_t tmp = 0;

    std::cout << "Starting read" << std::endl;
    //Set TWI to run in host mode
    usleep(1000);
    this->mask32(bar4, LB_CTL/4, 0x0, 0x10000);

    //Set TWI to run with both divisors to maximum (it is assumed
    //that the initial frequency is 125MHz, as suggested in the manual, that would lead to a
    //resulting frequency of 488.3kHz - the EEPROM on the board most probably is a 
    //24LC024 - which can handle up to 1MHz), clear FIFO count register,
    //unset slave monitor (->normal operation), unset hold, allow acknowledge bit,
    //uses normal (7-bit) addresses (not 10-bit), set as master node, set as master transmitter
    usleep(1000);
    this->write32(bar4, TWI_CTRL/4, 0xFF4E);

    //Wait while TWI BUS active bit is set, stop if takes too long
    usleep(1000);
    //std::cout << std::unitbuf << "Waiting for TWI BUS"; //DEBUG
    for(j = 1000000; (this->read32(bar4, TWI_STATUS/4) & 0x100); j--, usleep(1000)) {
        if(j == 0) {
            std::cout << "TWI BUS busy. Cannot initialize data transfer. \n";
            exit(-1);
            //std::cout << "."; //DEBUG
            //j = 1000000; //DEBUG
        }
    }
    //std::cout << "\n" << std::nounitbuf; //DEBUG

    //This read is supposed to clear the TWI_IRT_STATUS register.
    usleep(1000);
    this->read32(bar4, TWI_IRT_STATUS/4);

    //Sets the internal address counter of the EEPROM (that automatically incremates
    //after every read access) to a desired start address.
    //this->write32(bar4, TWI_DATA/4, offset & 0xFF);
    usleep(1000);
    this->write32(bar4, TWI_DATA/4, 0);

    //Write first 7 bit (7-bit-addresses!!!) of function parameter devAddr
    //to TWI_ADDRESS register and thus initiate a data transfer.
    //TWI slave address of the EEPROM is 0x56 (1010110).
    //this->write32(bar4, TWI_ADDRESS/4, devAddr & 0x7F);
    usleep(1000);
    this->write32(bar4, TWI_ADDRESS/4, 0x56);

    //Wait until data transfer is finished or error occurs, stop if takes too long.
    usleep(1000);
    //std::cout << std::unitbuf << "Waiting for Gennum to write slave address"; //DEBUG
    for(j = 1000000000, tmp = 0; !(tmp & 0x1); j--, usleep(1000)) {
        tmp = this->read32(bar4, TWI_IRT_STATUS/4);
        if(tmp & 0xC) {
            std::cout << "NACK or timeout occured. Aborting... \n";
            exit(-2);
        }
        if(j==0) {
            std::cout << "Transfer takes too long. Aborting... \n";
            exit(-1);
            //std::cout << "."; //DEBUG
            //j = 1000000000; //DEBUG
        }
    }
    //std::cout << "\n" << std::nounitbuf; //DEBUG

    //Set as master receiver
    usleep(1000);
    this->mask32(bar4, TWI_CTRL/4, 0x0, 0x1);

    //Read pieces of FIFO size until data of function parameter len is read - FiFo has 15 byte maximum
    for(k = 1, tmp = 0; len>0; ) {
        if(len > 1) {
		    k = 1;
		    len -= k;
        } else {
            k = len;
            len = 0;
        }

        /* Tell EEPROM how much data to send */
        usleep(1000);
        this->write32(bar4, TWI_TR_SIZE/4, k);

        /* Write EEPROM slave address (0x56) to BUS to initiate data transfer */
        usleep(1000);
        this->write32(bar4, TWI_ADDRESS/4, 0x56);

        /* Wait until data transfer is complete */
        //std::cout << std::unitbuf << "Transferring data, " << len << " left"; //DEBUG
        for(j = 1000000, tmp = 0; !(tmp & 0x1); j--) {
            usleep(1000);
            tmp = this->read32(bar4, TWI_IRT_STATUS/4);
            //std::cout << tmp << std::endl; //DEBUG
            /*if(tmp & 0xC) {
                std::cout << "NACK or timeout occured. Aborting... \n";
                exit(-3);
            }*/
            //std::cout << "."; //DEBUG
            if(j==0) {
                std::cout << "Transfer takes too long. Aborting... \n";
                exit(-1);
                //std::cout << "."; //DEBUG
                //j = 1000000; //DEBUG
            }
        }
        //std::cout << "\n" << std::nounitbuf; //DEBUG

        /* Read data from FiFo */
        usleep(1000);
        //std::cout << std::unitbuf << "Reading data from FiFo"; //DEBUG
        for(tmp = 0; k>0; k--, usleep(1000)) {
            tmp = this->read32(bar4, TWI_DATA/4);
            *(buffer + totalTransfer) = tmp & 0xFF;
            totalTransfer++;
            //std::cout << "."; //DEBUG
        }
    }
    //std::cout << "\n" << std::nounitbuf; //DEBUG

    //Possibly clear status register to unlock BUS for next use?
    usleep(1000);
    this->read32(bar4, TWI_IRT_STATUS/4);

    return totalTransfer;
}

uint32_t SpecCom::writeEeprom(uint8_t * buffer, uint32_t len, uint32_t offs) {

	uint32_t totalTransfer=0;
	unsigned int k = 0;
	unsigned int j = 0;
	uint32_t tmp = 0;

    //Set TWI to run in host mode
    usleep(1000);
    this->mask32(bar4, LB_CTL/4, 0x0, 0x10000);

    //Set TWI to run with both divisors to maximum (it is assumed
    //that the initial frequency is 125MHz, as suggested in the manual, that would lead to a
    //resulting frequency of 488.3kHz - the EEPROM on the board most probably is a 
    //24LC024 - which can handle up to 1MHz), clear FIFO count register,
    //unset slave monitor (->normal operation), unset hold, allow acknowledge bit,
    //uses normal (7-bit) addresses (not 10-bit), set as master node, set as master transmitter
    usleep(1000);
    this->write32(bar4, TWI_CTRL/4, 0xFF4E);

    //Wait while TWI BUS active bit is set, stop if takes too long
    usleep(1000);
    for(j = 1000000; (this->read32(bar4, TWI_STATUS/4) & 0x100); j--, usleep(1000)) {
        if(j == 0) {
            std::cout << "TWI BUS busy. Cannot initialize data transfer. \n";
            exit(-1);
        }
    }

    //This read is supposed to clear the TWI_IRT_STATUS register.
    usleep(1000);
    this->read32(bar4, TWI_IRT_STATUS/4);

    //Write data into EEPROM
    usleep(1000);
    for(k = 0; k<len; k++) {
        //Sets the internal address counter of the EEPROM to the desired address.
        usleep(5000);
        this->write32(bar4, TWI_DATA/4, (offs + k) & 0xFF);
        //std::cout << "Setting EEPROM register counter... \n"; //DEBUG

        //Write piece of data into data register. Upon write access to the
        //address register with slave device address, this piece of data
        //will be flashed to the EEPROM
        usleep(5000);
        this->write32(bar4, TWI_DATA/4, *(buffer + k));
        //std::cout << "Writing new EEPROM data to data register... \n"; //DEBUG

        //Write 7 bit slave device address
        //to TWI_ADDRESS register and thus initiate a data transfer.
        //TWI slave address of the EEPROM is 0x56 (1010110).
        usleep(5000);
        this->write32(bar4, TWI_ADDRESS/4, 0x56);
        //std::cout << "Flashing data to EEPROM... \n"; //DEBUG

        //Wait until data transfer is finished or error occurs, stop if takes too long.
        usleep(1000);
        for(j = 1000000000, tmp = 0; !(tmp & 0x1); j--, usleep(1000)) {
            tmp = this->read32(bar4, TWI_IRT_STATUS/4);
            if(tmp & 0xC) {
                if(tmp & 0x4)
                    std::cout << "NACK occured. Aborting... \n";
                if(tmp & 0x8)
                    std::cout << "Timeout occured. Aborting... \n";
                exit(-1);
            }
            if(j==0) {
                std::cout << "Transfer takes too long. Aborting... \n";
                exit(-1);
            }
        }

        totalTransfer++;
    }

    //Possibly clear status register to unlock BUS for next use?
    usleep(1000);
    this->read32(bar4, TWI_IRT_STATUS/4);

    return totalTransfer;
}

void SpecCom::createSbeFile(std::string fName, uint8_t * buffer, uint32_t length) {
    if(length != ARRAYLENGTH) {
        std::cout << "Wrong uint8_t array length. Aborting... \n";
        exit(-1);
    }

    if(fName==""){
        struct tm * timeinfo;
        time_t rawtime;
        time(&rawtime);
        timeinfo = localtime(&rawtime);
        fName =   "util/auto_"
                + std::to_string(1900+(timeinfo->tm_year)) + '_'
                + std::to_string(1+(timeinfo->tm_mon)) + '_'
                + std::to_string((timeinfo->tm_mday)) + '_'
                + std::to_string((timeinfo->tm_hour)) + '_'
                + std::to_string((timeinfo->tm_min)) + '_'
                + std::to_string((timeinfo->tm_sec)) + ".sbe"; //SpecBoard EEPROM content file (sbe)
    }

    std::ofstream oF(fName);
    if(!oF) {
        std::cout << "Could not create output file. Aborting... \n";
        exit(-1);
    }

    oF << std::hex;
    oF << std::showbase;
    oF << std::setw(9) << "addr" << std::setw(5) << "msk" << std::setw(12) << "data" << std::endl;
    //256/6 = 42; 256%6 = 4
    {
        uint16_t a;     //address
        uint8_t  m;     //mask
        uint32_t d;     //data
        for(unsigned int i = 0; i<(ARRAYLENGTH / 6); i++) {
            a  = ((buffer[i*6] | (buffer[i*6+1] << 8)) & 0xffc);
            m  = ((buffer[i*6+1] & 0xf0) >> 4);
            d  = (buffer[i*6+2] | (buffer[i*6+3] << 8) | (buffer[i*6+4] << 16) | (buffer[i*6+5] << 24));
            oF << std::setw(9) << a << std::setw(5) << (int)m << std::setw(12) << d << std::endl;
        }
    }
    oF << std::dec;
    oF << std::noshowbase;
    oF.close();

}

void SpecCom::getSbeFile(std::string pathname, uint8_t * buffer, uint32_t length) {
    if(length != ARRAYLENGTH) {
        std::cout << "Wrong uint8_t array length. Aborting... \n";
        exit(-1);
    }

    std::ifstream iF(pathname);

    {
        std::string tmpStr;
        std::getline(iF, tmpStr);
        if(tmpStr != "     addr  msk        data") {
            std::cerr << "Invalid headline. \n "
                      << "Note that it is encouraged to automatically create an sbe file \n"
                      << "using the EEPROM read functionality, copy the file, do desired \n"
                      << "changes by hand and use this file to write to the EEPROM. \n";
            return;
        }
    }

    {
        uint32_t tmpInt;
        unsigned int i = 0;
        iF >> std::hex;
        iF >> std::showbase;
        while(iF >> tmpInt) {
            if(i == ARRAYLENGTH) {
                std::cerr << "sbe file too big. Aborting... \nWARNING the EEPROM content has probably been modified!\n";
                return;
            }
            switch(i%3) {
            case 0:
                if(tmpInt>0xFFF) {
                    std::cout << "Invalid address size in line " << i/3 + 1 << ". Aborting... \n";
                    return;
                }
                *(buffer+i) = (tmpInt & 0xFF);
                i++;
                *(buffer+i) = ((tmpInt >> 8) & 0xF);
                break;
            case 1:
                if(tmpInt>0xF) {
                    std::cerr << "Invalid mask size in line " << i/3 + 1 << ". Aborting... \n";
                    return;
                }
                *(buffer+i) |= ((tmpInt & 0xF) << 4);
                i++;
                break;
            case 2:
                if(tmpInt>0xFFFFFFFF) {
                    std::cerr << "Invalid data size in line " << i/3 + 1 << ". Aborting... \n";
                    return;
                }
                *(buffer+i)   = (tmpInt & 0xFF);
                *(buffer+i+1) = ((tmpInt >>  8) & 0xFF);
                *(buffer+i+2) = ((tmpInt >> 16) & 0xFF);
                *(buffer+i+3) = ((tmpInt >> 24) & 0xFF);
                i+=4; //equivalent i->i+3+1, case structure invariant under i->i+3
                break;
            }
        }
        iF >> std::dec;
        iF >> std::noshowbase;
        if(i!=ARRAYLENGTH) {
            std::cerr << "Input file incomplete. Aborting... \nWARNING the EEPROM content has probably been modified!\n";
            return;
        }
    }

    return;
}

void SpecCom::flushDma() {
    volatile uint32_t dma_addr = 1;
    volatile uint32_t dma_count = 1;
    unsigned cnt = 0;
    unsigned timeout = 10000000;
    do {
        dma_addr = readSingle((0x3<<14) | 0x0);
        dma_count = readSingle((0x3<<14) | 0x1);
        cnt++;
        (void) dma_addr;
        (void) dma_count;
    } while (dma_count > 0 && cnt < timeout);
    if (cnt == timeout) {
        std::cerr << __PRETTY_FUNCTION__ << " -> Timed out while flushing buffers, might read rubbish!" << std::endl;
        exit(-1);
    }
}

