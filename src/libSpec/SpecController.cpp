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
#include <string>
#include <cstring>

#include <SpecController.h>
#include <GennumRegMap.h>
#include <BitOps.h>

SpecController::SpecController(unsigned int id) {
    specId = id;
    try {
        this->init();
        this->configure();
    } catch (Exception &e) {
        std::cerr << __PRETTY_FUNCTION__ << " -> " << e.toString() << std::endl;
        std::cerr << __PRETTY_FUNCTION__ <<  " -> Fatal Error! Aborting!"  << std::endl;
        exit(-1);
    }
}

SpecController::~SpecController() {
    spec->unmapBAR(0, bar0);
    spec->unmapBAR(4, bar4);
    spec->close();
    delete spec;
}

void SpecController::writeSingle(uint32_t off, uint32_t val) {
    this->write32(bar0, off, val);
}

uint32_t SpecController::readSingle(uint32_t off) {
    uint32_t tmp = this->read32(bar0, off);
    return tmp; 
}

void SpecController::write32(uint32_t off, uint32_t *val, size_t words) {
    this->write32(bar0, off, val, words);
}

void SpecController::read32(uint32_t off, uint32_t *val, size_t words) {
    this->read32(bar0, off, val, words);
}


void SpecController::writeBlock(uint32_t off, uint32_t *val, size_t words) {
    this->writeBlock(bar0, off, val, words);
}

void SpecController::readBlock(uint32_t off, uint32_t *val, size_t words) {
    this->readBlock(bar0, off, val, words);
}

int SpecController::writeDma(uint32_t off, uint32_t *data, size_t words) {
    int status = this->getDmaStatus(); 
    if ( status == DMAIDLE || status == DMADONE || status == DMAABORTED) {
        UserMemory *um = &spec->mapUserMemory(data, words*4, false);
        KernelMemory *km = &spec->allocKernelMemory(sizeof(struct dma_linked_list)*um->getSGcount());

        struct dma_linked_list *llist = this->prepDmaList(um, km, off, 1);
        
        uint32_t *addr = (uint32_t*) bar0+DMACSTARTR;
        memcpy(addr, &llist[0], sizeof(struct dma_linked_list));
        this->startDma();

        if (spec->waitForInterrupt(0) < 1) {
            std::cerr << __PRETTY_FUNCTION__ << " -> " 
            << "Interrupt timeout, aborting transfer!" << std::endl;
            this->abortDma();
        }
        
        // Ackowledge interrupt
        volatile uint32_t irq_ack = this->read32(bar4, GNGPIO_INT_STATUS/4);
        (void) irq_ack;
        
        delete km;
        delete um;
        return 0;
    } else {
        std::cerr << __PRETTY_FUNCTION__ << " -> " 
            << "DMA Transfer aborted (Status = 0x" << std::hex << status << std::dec << ")" << std::endl;
        return 1;
    }
}

int SpecController::readDma(uint32_t off, uint32_t *data, size_t words) {
    int status = this->getDmaStatus(); 
    if ( status == DMAIDLE || status == DMADONE || status == DMAABORTED) {
        UserMemory *um = &spec->mapUserMemory(data, words*4, false);
        KernelMemory *km = &spec->allocKernelMemory(sizeof(struct dma_linked_list)*um->getSGcount());

        struct dma_linked_list *llist = this->prepDmaList(um, km, off, 0);
        
        uint32_t *addr = (uint32_t*) bar0+DMACSTARTR;
        memcpy(addr, &llist[0], sizeof(struct dma_linked_list));
        this->startDma();

        if (spec->waitForInterrupt(0) < 1) {
            std::cerr << __PRETTY_FUNCTION__ << " -> " 
            << "Interrupt timeout, aborting transfer!" << std::endl;
            this->abortDma();
        }
        
        // Ackowledge interrupt
        volatile uint32_t irq_ack = this->read32(bar4, GNGPIO_INT_STATUS/4);
        (void) irq_ack;

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

void SpecController::init() {
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
        bar4 = spec->mapBAR(4);
#ifdef DEBUG
        std::cout << __PRETTY_FUNCTION__ << " -> Mapped BAR4 at 0x" << std::hex << bar4 
            << " with size 0x" << spec->getBARsize(4) << std::dec << std::endl;
#endif
    } catch (Exception &e) {
        std::cerr << __PRETTY_FUNCTION__ << " -> " << e.toString() << std::endl;
        throw Exception(Exception::INIT_FAILED);
        return;
    }
    return;
}

void SpecController::configure() {
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

    // Clear IRQ queues
    spec->clearInterruptQueue(0);
    spec->clearInterruptQueue(1);
}

void SpecController::write32(void *bar, uint32_t off, uint32_t val) {
    uint32_t *addr = (uint32_t*) bar+off;
    *addr = val;
}

uint32_t SpecController::read32(void *bar, uint32_t off) {
    uint32_t *addr = (uint32_t*) bar+off;
    return *addr;
}

void SpecController::mask32(void *bar, uint32_t off, uint32_t mask, uint32_t val) {
    uint32_t *addr = (uint32_t*) bar+off;
    uint32_t tmp = *addr;
    tmp &= ~mask;
    tmp |= val;
    *addr = tmp;
}

void SpecController::writeBlock(void *bar, uint32_t off, uint32_t *val, size_t words) {
    uint32_t *addr = (uint32_t*) bar+off;
    memcpy(addr, val, words*4);
}

void SpecController::write32(void *bar, uint32_t off, uint32_t *val, size_t words) {
    uint32_t *addr = (uint32_t*) bar+off;
    for (uint32_t i=0; i<words; i++)
        *addr = val[i];
}

void SpecController::readBlock(void *bar, uint32_t off, uint32_t *val, size_t words) {
    uint32_t *addr = (uint32_t*) bar+off;
    for(unsigned int i=0; i<words; i++) 
        val[i] = *addr;
}

void SpecController::read32(void *bar, uint32_t off, uint32_t *val, size_t words) {
    uint32_t *addr = (uint32_t*) bar+off;
    for(unsigned int i=0; i<words; i++) val[i] = *addr++;
}

struct dma_linked_list* SpecController::prepDmaList(UserMemory *um, KernelMemory *km, uint32_t off, bool write) {
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

void SpecController::startDma() {
    uint32_t *addr = (uint32_t*) bar0+DMACTRLR;
    // Set t 0x1 to start DMA transfer
    *addr = 0x1;
}

void SpecController::abortDma() {
    uint32_t *addr = (uint32_t*) bar0+DMACTRLR;
    // Set t 0x2 to abort DMA transfer
    *addr = 0x2;
}

uint32_t SpecController::getDmaStatus() {
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

int SpecController::progFpga(const void *data, size_t size) {
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

//uint32_t SpecController::readEprom(void * buffer, uint32_t devAddr, uint32_t offset, uint32_t len) {
uint32_t SpecController::readEeprom(uint8_t * buffer, uint32_t len) {

	//std::cout << "Creating local variables.\n"; //debug
	uint32_t totalTransfer=0;
	int k = 0;
	int j = 0;
	uint32_t tmp = 0;
	//uint8_t * ptrBuff = static_cast<uint8_t*>(buffer);

    //int size32 = (size + 3) >> 2;
    //const uint32_t *data32 = (uint32_t*)data;


    // Set TWI to run in host mode
    std::cout << "Setting Gennum TWI interface into host mode.\n"; //debug
    this->mask32(bar4, LB_CTL/4, 0x0, 0x10000);

    //Set TWI to run with both divisors to maximum (it is assumed
    //that the initial frequency is 125MHz, as suggested in the manual, that would lead to a
    //resulting frequency of 488.3kHz - the EEPROM on the board most probably is a 
    //24LC024 - which can handle up to 1MHz), clear FIFO count register,
    //unset slave monitor (->normal operation), unset hold, allow acknowledge bit,
    //uses normal (7-bit) addresses (not 10-bit), set as master node, set as master transmitter
    std::cout << "Setting clock divisors and other important bits.\n"; //debug
    this->write32(bar4, TWI_CTRL/4, 0xFF4E);

    //Wait while TWI BUS active bit is set, stop if takes too long
    std::cout << "Waiting while TWI BUS active bit is set.\n"; //debug
    for(k = 1000000; (this->read32(bar4, TWI_STATUS/4) & 0x100); k--) {
        if(k == 0) {
            std::cout << "TWI BUS busy. Cannot initialize data transfer. \n";
            exit(-1);
        }
    }

    //This read is supposed to clear the TWI_IRT_STATUS register.
    //Not sure if this works or is even necessary?
    std::cout << "Clear TWI status register.\n"; //debug
    this->read32(bar4, TWI_IRT_STATUS/4);

    //Writes first byte of function parameter offset to TWI_DATA register (8 bit long).
    //I have no idea why this should be done. It is not necessary according to the Gennum manual.
    //I think it sets the internal address counter of the EEPROM (that automatically incremates
    //after every read access) to a desired start address.
    //this->write32(bar4, TWI_DATA/4, offset & 0xFF);
    this->write32(bar4, TWI_DATA/4, 0);

    //Write first 7 bit (7-bit-addresses!!!) of function parameter devAddr
    //to TWI_ADDRESS register and thus initiate a data transfer.
    //TWI slave address of the EEPROM is 0x56 (1010110).
    //I think this is not necessary since the start address is supposed to be zero.
    //this->write32(bar4, TWI_ADDRESS/4, devAddr & 0x7F);
    this->write32(bar4, TWI_ADDRESS/4, 0x56);

    //Wait until data transfer is finished or error occurs, stop if takes too long.
    //I think this is not necessary since the start addr is supposed to be zero.
    for(j = 1000000000, tmp = 0; !(tmp & 0x1); k--) {
        tmp = this->read32(bar4, TWI_IRT_STATUS/4);
        if(tmp & 0xC) {
            std::cout << "NACK or timeout occured. Aborting... \n";
            exit(-2);
        }
        if(k==0) {
            std::cout << "Transfer takes too long. Aborting... \n";
            exit(-2);
        }
    }

    //Set as master receiver
    std::cout << "Setting Gennum TWI interface into master receiver mode.\n"; //debug
    this->mask32(bar4, TWI_CTRL/4, 0x0, 0x1);

    //Read pieces of FIFO size until data of function parameter len is read - FiFo has 15 byte maximum
    std::cout << "Reading EEPROM.\n"; //debug
    for(k = 1, tmp = 0; len>0; ) {
        if(len > 1) {
		    k = 1;
		    len -= k;
        } else {
            k = len;
            len = 0;
        }
        //std::cout << "Remaining length: " << len << std::endl; //debug
        //std::cout << "Words to read: " << k << std::endl; //debug

        /* Tell EEPROM how much data to send */
        this->write32(bar4, TWI_TR_SIZE/4, k);
        //std::cout << "Told EEPROM to write " << k << " words to data register.\n"; //debug

        /* Write EEPROM slave address (0x56) to BUS to initiate data transfer */
        this->write32(bar4, TWI_ADDRESS/4, 0x56);
        //std::cout << "Initialized data transfer.\n"; //debug

        /* Wait until data transfer is complete */
        for(j = 1000000000, tmp = 0; !(tmp & 0x1); j--) {
            //std::cout << "Reading status register, " << j << " attempts remaining.\n"; //debug
            tmp = this->read32(bar4, TWI_IRT_STATUS/4);
            /*if(tmp & 0xC) {
                std::cout << "NACK or timeout occured. Aborting... \n";
                exit(-3);
            }*/
            if(j==0) {
                std::cout << "Transfer takes too long. Aborting... \n";
                exit(-3);
            }
        }
        //std::cout << "All requested data written to data register.\n"; //debug

        /* Read data from FiFo */
        for(tmp = 0; k>0; k--) {
            tmp = read32(bar4, TWI_DATA/4);
            *(buffer + totalTransfer) = tmp & 0xFF;
            //std::cout << "Just read value: " << std::hex << tmp << std::dec << std::endl; //debug
            totalTransfer++;
        }
    }

    //Possibly clear status register to unlock BUS for next use?
    this->read32(bar4, TWI_IRT_STATUS/4);

    return totalTransfer;
}

