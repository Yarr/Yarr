#include <thread>

#include "BdaqRxCore.h"
#include "logging.h"

namespace {
  auto logger = logging::make_log("BdaqRxCore");
}

//BDAQ word identifiers
#define USERK_FRAME_ID  0x01000000
#define HEADER_ID       0x00010000
#define TRIGGER_ID      0x80000000
#define TDC_ID_0        0x10000000
#define TDC_ID_1        0x20000000
#define TDC_ID_2        0x30000000
#define TDC_ID_3        0x40000000
#define TDC_HEADER_MASK 0xF0000000

BdaqRxCore::BdaqRxCore() {
    userkCounter = 0;
    
    isEventHeader = false;
    isHighWord = true;

    mSetupMode = true;
}

void BdaqRxCore::setupMode() {
    mSetupMode = true;
    logger->debug("Setup Mode");
    // Enable register data monitoring
    Bdaq::setMonitorFilter(BdaqAuroraRx::filter);
}

void BdaqRxCore::runMode() {
    mSetupMode = false;
    logger->debug("Run Mode");
    // Disable register data monitoring
    Bdaq::setMonitorFilter(BdaqAuroraRx::block);
}

// val: 0, 1, 2, 3
void BdaqRxCore::setRxEnable(uint32_t val) {
    logger->info("setRxEnable(): value = {}", val);
    activeChannels.clear();
    switch (val) {
        //Mapping rx[0, 1, 2, 3] to BDAQ rx[0, 4, 5, 6]
        case 0: activeChannels.push_back(0); break; 
        case 1: activeChannels.push_back(4); break;
        case 2: activeChannels.push_back(5); break;
        case 3: activeChannels.push_back(6); break;
    }
}

void BdaqRxCore::setRxEnable(std::vector<uint32_t> channels) {
    activeChannels.clear();
    for (uint32_t channel : channels) {
        logger->info("setRxEnable(): channels = {}", channel);
        switch (channel) {
            //Mapping rx[0, 1, 2, 3] to BDAQ rx[0, 4, 5, 6]
            case 0: activeChannels.push_back(0); break; 
            case 1: activeChannels.push_back(4); break;
            case 2: activeChannels.push_back(5); break;
            case 3: activeChannels.push_back(6); break;
        }
    }
    std::cin.get();
}

void BdaqRxCore::maskRxEnable(uint32_t val, uint32_t mask) {
    std::stringstream d; 
    d << __PRETTY_FUNCTION__ << " : Value 0x" << std::hex << val 
      << ", Mask 0x" << mask << std::dec;
    logger->debug(d.str());
}

void BdaqRxCore::checkRxSync() {
	uint time = 0;
    bool rxReady = true;
    for (uint c : activeChannels) {
        rxReady &= rx.at(c).getRxReady();
    }
	while (time < 1000 && rxReady == false) {
		++time;
        for (uint c : activeChannels) {
            rxReady &= rx.at(c).getRxReady();
        }
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	if (rxReady) {
		logger->info("Aurora link is synchronized!");
	}
	else {
        cmd.reset();
		logger->critical("Aurora link DID NOT synchronize");
        exit(-1);
	}
}

RawData* BdaqRxCore::readData() {
    uint size = readEqualized();
    if (size > 0) {
        std::vector<uint32_t> streamBuf;
        buildStream(streamBuf, size);
        // outBuf size is always < sortedSize. 
        uint32_t* outBuf = new uint32_t[size]; 
        size = decode(streamBuf, outBuf);
        // now, size has the number of decoded (thus, usable) words
        if (size > 0) {
            return new RawData(0x0, outBuf, size);
        } 
        return NULL;
    }
    return NULL;
}

void BdaqRxCore::flushBuffer() {
    std::stringstream d; 
    d << __PRETTY_FUNCTION__;
    logger->debug(d.str());
    //auroraRx->resetLogic();
    fifo.flushBuffer();
}

uint32_t BdaqRxCore::getDataRate() {
    std::stringstream d; 
    d << __PRETTY_FUNCTION__ << std::endl;
    logger->debug(d.str());
    return 0;
}

bool BdaqRxCore::isBridgeEmpty() {
    std::stringstream d; 
    d << __PRETTY_FUNCTION__ << std::endl;
    logger->debug(d.str());
    return true;
}

// =============================================================================
// BDAQ Decoding
// =============================================================================

// Readout stream processing is "global", assuming that USERK words
// (encapsulating service frames: register or monitoring) might arrive
// separetely (with other word types in between) or in separated 
// readout blocks. The same applies to Pixel Data words. This behavior
// is due to the arbiter (selecting either USERK or Pixel Data) inside
// the BDAQ RX core (rx_aurora).

uint BdaqRxCore::readEqualized() {
    uint trial = 0;
    bool equalSize = false;
    std::vector<uint32_t> inBuf;
    uint sorted = 0;

    uint available = fifo.getAvailableWords(); 
    if (available == 0) return 0;
    
    initSortBuffer();

    while (trial < 1000 && equalSize == false) {
        /*logger->info("--------------------------------------------");
        logger->info("trial = {}, available = {}", trial, available);*/
        ++trial;
        inBuf.clear();
        fifo.readData(inBuf, available);    
        sorted = sortChannels(inBuf);
        equalSize = testEqualSize();
        /*logger->info("sorted = {}, equalSize = {}", sorted, equalSize);
        logger->info("--------------------------------------------");*/
        // Wait?
        available = fifo.getAvailableWords(); 
    }
    return sorted;
}

void BdaqRxCore::initSortBuffer() {
    sBuffer.clear();
    for (uint c : activeChannels) {
        sBuffer.push_back(std::deque<uint32_t>(0));
    }
}

uint BdaqRxCore::sortChannels(std::vector<uint32_t>& in) {
    uint size = 0;
    // Sorting
    //initSortBuffer();
    for (const auto& word : in) {
        uint nChannel = 0;
        for (uint c : activeChannels) {
            // Testing Aurora RX Identifier 
            if (((word >> 20) & 0xF) == c) {
                sBuffer.at(nChannel).push_back(word);
                ++size;
            }
            ++nChannel;
        }
    }
    return size; // Total size.
}

bool BdaqRxCore::testEqualSize() {
    // Test if channels have the same size
    switch (activeChannels.size()) {
        case 0: return false; break; // No channels, no size.
        case 1: return true; break;
        case 2: 
            return sBuffer.at(0).size() == sBuffer.at(1).size();
        break;
        case 3: 
            return (sBuffer.at(0).size() == sBuffer.at(1).size()) &&
                   (sBuffer.at(1).size() == sBuffer.at(2).size());
        break;
        case 4: 
            return (sBuffer.at(0).size() == sBuffer.at(1).size()) &&
                   (sBuffer.at(1).size() == sBuffer.at(2).size()) &&
                   (sBuffer.at(2).size() == sBuffer.at(3).size());
        break;
        default: return false;
    }
}

void BdaqRxCore::buildStream(std::vector<uint32_t>& out, uint size) {
    // Building continuous stream
    for (uint i=0;i<size;++i) {
        // 4 words of each channel, sequentially
        uint nChannel = (i/4)%activeChannels.size();
        if (sBuffer.at(nChannel).size() == 0) continue; // Might delete when get equal sizes OR MIGHT INSERT SOMETHING THAT IS IGNORED BY YARR DATA PROCESSOR.
        out.push_back(sBuffer.at(nChannel).at(0));
        sBuffer.at(nChannel).pop_front();
    }
}


unsigned int BdaqRxCore::decode(std::vector<uint32_t>& in, uint32_t* out) {
    
    unsigned int index = 0;
    
    for (const auto& word : in) {

        if (word & TRIGGER_ID) {
            logger->critical("TLU data is not yet supported.");
            exit(-1);
        } 
        if (checkTDC(word)) {
            logger->critical("TDC data is not yet supported.");
            exit(-1);
        } 
        if (word & USERK_FRAME_ID) {
            index = decodeUserk(word, out, index);
            continue;
        } 
        if (word & HEADER_ID) {
            isEventHeader = true;
            isHighWord = true;
        }

        if (isHighWord) {
            dataWord = word & 0xFFFF;
            isHighWord = false; // Next low word
            continue;
        } else {
            dataWord = (dataWord << 16) | (word & 0xFFFF);
            isHighWord = true; // Next is high word
        }

        if (isEventHeader) {
            isEventHeader = false;
            out[index] = dataWord;
            ++index;
        } else {
            out[index] = dataWord;
            ++index;
        }

    }
    return index;
}

// USERK Decoding ==============================================================

unsigned int BdaqRxCore::decodeUserk(const uint32_t& word, uint32_t* out, 
                                        unsigned int index) {                                            
    logger->debug("USERK word {} = {}", userkCounter, word);
    buildUserkFrame(word, userkCounter);
    // 4 USERK readout words are necessary to build an USERK frame.
    if (userkCounter == 3) {
        userkCounter = 0;
        BdaqRxCore::userkDataT userkData = interpretUserkFrame();
        std::vector<regDataT> regData = getRegData(userkData);
        // regData might contain data from either 1 or 2 registers.
        // The code below will insert this data into the output stream
        // using one of the YARR expected formats.                                        
        for (const auto& reg : regData) {
            encodeToYarr(reg, out, index); //inserts 2 words in the out stream.
            index+=2;
        }
    } else {
        ++userkCounter;
    }
    return index;
}

// An USERK frame is composed by 2 32-bit words (userkWordA and userkWordB).
// To build the frame, 4 readout words are neccesary (identified by id).
void BdaqRxCore::buildUserkFrame(const uint32_t& word, unsigned int id) {
    switch(id) {
        case 0:
            userkWordA = word & 0xFFFF;
        break;
        case 1:
            userkWordA = userkWordA << 16 | (word & 0xFFFF);
        break;
        case 2:
            userkWordB = word & 0xFFFF;
        break;
        case 3:
            userkWordB = userkWordB << 16 | (word & 0xFFFF);
        break;
    }    
}

// Extracts the data from an USERK frame (userkWordA + userkWordB).
BdaqRxCore::userkDataT BdaqRxCore::interpretUserkFrame() {
    uint64_t userkBlock, Data0, Data1;
    userkDataT u;

    logger->debug("userkWordA = {}", userkWordA);
    logger->debug("userkWordB = {}", userkWordB);

    userkWordA = (userkWordB & 0x3) << 32 | userkWordA;
    userkBlock = userkWordB >> 2;
    Data1 = userkBlock & 0x7FFFFFF;
    Data0 = (userkWordA >> 8) & 0x7FFFFFF;
    u.AuroraKWord = userkWordA & 0xFF;
    u.Status = (userkBlock >> 30) & 0xF;
    u.Data1 = Data1;
    u.Data1_AddrFlag = (Data1 >> 25) & 0x1;
    u.Data1_Addr = (Data1 >> 16) & 0x1FF;
    u.Data1_Data = (Data1 >> 0) & 0xFFFF;
    u.Data0 = Data0;
    u.Data0_AddrFlag = (Data0 >> 25) & 0x1;
    u.Data0_Addr = (Data0 >> 16) & 0x1FF;
    u.Data0_Data = (Data0 >> 0) & 0xFFFF;
    
    return u;
}

// Get register data according to AuroraKWord.
std::vector<BdaqRxCore::regDataT> BdaqRxCore::getRegData(BdaqRxCore::userkDataT in) {
    BdaqRxCore::regDataT o;
    std::vector<BdaqRxCore::regDataT> regData;

    logger->info("AuroraKWord = {}", in.AuroraKWord);

    // There is data in both Data0 and Data1 (data from 2 different registers?)
    if (in.AuroraKWord == 0) {
        o.Address = in.Data1_Addr;
        o.Data = in.Data1;
        regData.push_back(o);
        o.Address = in.Data0_Addr;
        o.Data = in.Data0;
        regData.push_back(o);
    }
    // Register data in Data1
    if (in.AuroraKWord == 1) {
        o.Address = in.Data1_Addr;
        o.Data = in.Data1;
        regData.push_back(o);
    }
    // Register data in Data0
    if (in.AuroraKWord == 2) {
        o.Address = in.Data0_Addr;
        o.Data = in.Data0;
        regData.push_back(o);
    }
    return regData;
}

// Emulating the YARR expected readout format for a register frame
void BdaqRxCore::encodeToYarr(BdaqRxCore::regDataT in, uint32_t* out, 
                                unsigned int index) {
    out[index  ] = 0x55000000;
    out[index+1] = (in.Address & 0x3FF) << 16 | 
                   (in.Data    & 0xFFFF);                               
}

// TDC Data Decoding ===========================================================

bool BdaqRxCore::checkTDC(const uint32_t& word) {
    if (((word & TDC_HEADER_MASK) == TDC_ID_0) || 
        ((word & TDC_HEADER_MASK) == TDC_ID_1) ||
        ((word & TDC_HEADER_MASK) == TDC_ID_2) ||
        ((word & TDC_HEADER_MASK) == TDC_ID_3)) return true;
    else
        return false;
}
