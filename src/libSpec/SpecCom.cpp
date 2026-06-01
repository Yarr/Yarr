// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: SPEC cpp library
// # Comment: Original driver taken from Marcus Guillermo
// #          Modified for SPEC card
// ################################

#include <cstdlib>
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

#include "logging.h"

namespace {
    auto slog = logging::make_log("SpecCom");
}


SpecCom::SpecCom() {
    is_initialized = false;
    do_reset = true;
    specId = 0;
    fw_vers = 0x0;
    fw_ident = 0x0;
}

SpecCom::SpecCom(unsigned int id, bool do_reset_arg) {
    specId = id;
    is_initialized = false;
    do_reset = do_reset_arg;
    fw_vers = 0x0;
    fw_ident = 0x0;
    try {
        this->init();
        this->configure();
    } catch (Exception &e) {
        slog->critical("Error during initilisation:  {}", e.toString());
        slog->critical("Fatal Error! Aborting!");
        exit(-1);
    }
    is_initialized = true;
}

SpecCom::~SpecCom() {
    spec->unmapBAR(0, bar0);
    if (bar4 != nullptr)
        spec->unmapBAR(4, bar4);
    spec->close();
    delete spec;
}

bool SpecCom::isInitialized() const {
    return is_initialized;
}

int SpecCom::getId() const {
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
            slog->critical("Error during initilisation:  {}", e.toString());
            slog->critical("Fatal Error! Aborting!");
            exit(-1);
        }
        is_initialized = true;
    } else {
        slog->warn("SPEC is already initialzed!");
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
            slog->error("Interrupt timeout during DMA, aborting transfer!");
            this->abortDma();
        }

        // Ackowledge interrupt
        if (bar4 != nullptr) {
            volatile uint32_t irq_ack = this->read32(bar4, GNGPIO_INT_STATUS/4);
            (void) irq_ack;
        }

        delete km;
        delete um;
        return 0;
    } else {
        slog->error("DMA Transfer aborted (Status = 0x{:x})", status);
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
            slog->error("Interrupt timeout during DMA, aborting transfer!");
            this->abortDma();
        }
        
        // Ackowledge interrupt
        if (bar4 != nullptr) {
            volatile uint32_t irq_ack = this->read32(bar4, GNGPIO_INT_STATUS/4);
            (void) irq_ack;
        }
        um->sync(UserMemory::BIDIRECTIONAL);

        delete km;
        delete um;
        status = this->getDmaStatus(); 
        if (status == DMAABORTED || status == DMAERROR) {
            slog->error("DMA Transfer aborted (Status = 0x{:x})", status);
            return 1;
        } else {
            return 0;
        }
    } else {
        slog->error("DMA Transfer aborted (Status = 0x{:x})", status);
        return 1;
    }
}

void SpecCom::init() {
    slog->info("Opening SPEC with id #{}", specId);
    // Init SPEC
    try {
        spec = new SpecDevice(specId);
    } catch (Exception &e) {
        slog->error("Error while opening SPEC{}", e.toString());
        throw Exception(Exception::INIT_FAILED);
        return;
    }
    // Open SPEC
    spec->open();
    slog->info("Mapping BARs ...");
    // Map BARs
    try {
        bar0 = spec->mapBAR(0);
        slog->info("... Mapped BAR0 at 0x{:x} with size {}", uint64_t(bar0), spec->getBARsize(0));
    } catch (Exception &e) {
        slog->error("Error while mapping BAR: {}", e.toString());
        throw Exception(Exception::INIT_FAILED);
        return;
    }
    bar4 = nullptr;

    // Get FW info
    fw_vers = readSingle(SPEC_GREG | SPEC_GREG_FWVERS);
    fw_ident = readSingle(SPEC_GREG | SPEC_GREG_FWIDENT);
    if (fw_ident == 0xFFFFFFFF || fw_vers == 0xFFFFFFFF) {
        slog->error("Could not read FW version or identifier!");
    } else {
        auto status = getStatus();
        slog->info("~~~~~~~~~~~~~~~~~~~~~~~~~~~");
        slog->info("Firmware Hash: {}", static_cast<std::string>(status["firmware_hash"]));
        slog->info("Firmware Version: {}", static_cast<std::string>(status["firmware_vers"]));
        slog->info("Firmware Identifier: {}", static_cast<std::string>(status["firmware_identifier"]));
        slog->info("FPGA card: {}", static_cast<std::string>(status["fpga_card"]));
        slog->info("FE Chip Type: {}", static_cast<std::string>(status["fe_chip_type"]));
        slog->info("FMC Card Type: {}", static_cast<std::string>(status["fmc_card_type"]));
        slog->info("RX Speed: {}", static_cast<std::string>(status["rx_speed"]));
        slog->info("Channel Configuration: {}", static_cast<std::string>(status["channel_configuration"]));
        slog->info("LPM Status: {}", static_cast<uint32_t>(status["lpm_status"]));
        slog->info("~~~~~~~~~~~~~~~~~~~~~~~~~~~");
    } 
    // TODO decode firmware identifier

    if(do_reset) {
        slog->info("Soft resetting all ...");
        this->writeSingle(SPEC_GREG | SPEC_GREG_SOFTRST, SOFTRST_BRAM | SOFTRST_BRAM_CNT);
    }

    slog->info("Flushing buffers ...");
    this->flushDma();
    slog->info("Init success!");
    return;
}

const json SpecCom::getStatus() {
    json j_status;
    // turn the version and id into hex strings here (it is easier to convert
    // a hex string to an int than it is an int to a hex string in C++, so just
    // do it here and rely on only std::string being in the returned JSON object)
    std::stringstream fw_string;
    fw_string << "0x" << std::hex << fw_vers;
    j_status["firmware_hash"] = fw_string.str(); fw_string.str("");
    fw_string << "0x" << std::hex << fw_ident;
    j_status["firmware_vers"] = specIdentFw[fw_vers];
    j_status["firmware_identifier"] = fw_string.str(); fw_string.str("");
    j_status["fpga_card"] = getSpecIdentHw(fw_ident);
    j_status["fe_chip_type"] = getSpecIdentChip(fw_ident);
    j_status["fmc_card_type"] = getSpecIdentFmc(fw_ident);
    j_status["rx_speed"] = getSpecIdentSpeed(fw_ident);
    j_status["channel_configuration"] = getSpecIdentChCfg(fw_ident);
    j_status["lpm_status"] = readSingle(SPEC_GREG | SPEC_GREG_LPMEN);
    return j_status;
}

void SpecCom::configure() {
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

void SpecCom::flushDma() {
    volatile uint32_t dma_addr = 1;
    volatile uint32_t dma_count = 1;
    unsigned cnt = 0;
    unsigned timeout = 10000000;
    if (bar4)
        timeout = 100;
    do {
        dma_addr = readSingle((0x3<<14) | 0x0);
        dma_count = readSingle((0x3<<14) | 0x1);
        cnt++;
        (void) dma_addr;
        (void) dma_count;
    } while (dma_count > 0 && cnt < timeout);
    if (cnt == timeout) {
        slog->critical("Timed out while flushing buffers, something is wrong ... aborting!");
        if (!bar4)
            exit(-1);
    }
    if(do_reset) {
        // Reset BRAM Counters now as they might be desynced
        this->writeSingle(SPEC_GREG | SPEC_GREG_SOFTRST, SOFTRST_BRAM_CNT);
    }

}

std::string SpecCom::getSpecIdentHw(uint32_t fw_ident) {
  unsigned index = (fw_ident>>24)&0xFF;
  if (array_length(specIdentHw)<=index) index = 0;
  return specIdentHw[index];
}
std::string SpecCom::getSpecIdentChip(uint32_t fw_ident) {
  unsigned index = (fw_ident>>16)&0xFF;
  if (array_length(specIdentChip)<=index) index = 0;
  return specIdentChip[index];
}
std::string SpecCom::getSpecIdentFmc(uint32_t fw_ident) {
  unsigned index = (fw_ident>>8)&0xFF;
  if (array_length(specIdentFmc)<=index) index = 0;
  return specIdentFmc[index];
}
std::string SpecCom::getSpecIdentSpeed(uint32_t fw_ident) {
  unsigned index = (fw_ident>>4)&0xF;
  if (array_length(specIdentSpeed)<=index) index = 0;
  return specIdentSpeed[index];
}
std::string SpecCom::getSpecIdentChCfg(uint32_t fw_ident) {
  unsigned index = (fw_ident)&0xF;
  if (array_length(specIdentChCfg)<=index) index = 0;
  return specIdentChCfg[index];
}

uint32_t SpecCom::getSpecIdentLaneCfg(uint32_t fw_ident) {
  unsigned index = (fw_ident)&0xF;
  if (array_length(specIdentLaneCfg)<=index) index = 0;
  return specIdentLaneCfg[index];
}
