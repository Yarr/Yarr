#include <thread>

#include "BdaqRxCore.h"
#include "logging.h"

namespace {
  auto logger = logging::make_log("BdaqRxCore");
}

//BDAQ word identifiers
#define USERK_FRAME_ID   0x01000000
#define USERK_FRAME_MASK 0x0F000000
#define TLU_ID           0x80000000
#define TLU_MASK         0xF0000000
#define TDC_ID_0         0x10000000
#define TDC_ID_1         0x20000000
#define TDC_ID_2         0x30000000
#define TDC_ID_3         0x40000000
#define TDC_HEADER_MASK  0xF0000000

BdaqRxCore::BdaqRxCore() {
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

// val: whatever is passed in "rx"
void BdaqRxCore::setRxEnable(uint32_t val) {
    logger->debug("setRxEnable(): value = {}", val);
    activeChannels.clear();
    activeChannels.push_back(val);
    initSortBuffer();
}

void BdaqRxCore::setRxEnable(std::vector<uint32_t> channels) {
    activeChannels.clear();
    for (uint32_t channel : channels) {
        logger->debug("setRxEnable(): channels = {}", channel);
        activeChannels.push_back(channel);
    }
    initSortBuffer();
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
    uint size = fifo.getAvailableWords();
    if (size > 0) {
        std::vector<uint32_t> inBuf;
        fifo.readData(inBuf, size);
        size = sortChannels(inBuf);
        uint32_t* outBuf = new uint32_t[size];
        size = buildStream(outBuf, size);
        if (size > 0) {
            return new RawData(0x0, outBuf, size);
        } 
        //delete[] outBuf;
        return NULL;
    }
    return NULL;
}

void BdaqRxCore::flushBuffer() {
    std::stringstream d; 
    d << __PRETTY_FUNCTION__;
    logger->debug(d.str());
    for (uint i=0;i<7;++i) {
        rx.at(i).resetLogic();
    }
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

void BdaqRxCore::printSortStatus() {
    uint nChannel = 0;
    for (const auto& c : activeChannels) {    
        logger->info("RX ID = {}, Buffer Index = {}, Size = {}", c, nChannel, sBuffer.at(nChannel).size());
        ++nChannel;
    }
}

void BdaqRxCore::initSortBuffer() {
    sBuffer.clear();
    for (uint c : activeChannels) {
        //sBuffer.push_back(std::deque<uint32_t>(0));
        sBuffer.push_back(std::queue<uint32_t>());
    }
}

uint BdaqRxCore::sortChannels(std::vector<uint32_t>& in) {
    // Sorting
    for (const auto& word : in) {
        if ((word & TLU_MASK) == TLU_ID) {
            logger->critical("TLU data is not supported.");
            exit(-1);
        } 
        if (checkTDC(word)) {
            logger->critical("TDC data is not supported.");
            exit(-1);
        } 
        uint bIndex = 0;
        for (const auto& channelId : activeChannels) {
            // Testing Aurora RX Identifier 
            if (((word >> 20) & 0xF) == channelId) {
                sBuffer.at(bIndex).push(word);
            }
            ++bIndex;
        }
    }
    // Calculating Size
    uint acc = 0;
    uint bIndex = 0;
    for (const auto& channelId : activeChannels) {
        acc += sBuffer.at(bIndex).size();
        ++bIndex;
    }
    return acc;
}

void BdaqRxCore::buildData(uint32_t* out, uint bIndex, uint oIndex) {
    uint32_t hi = sBuffer.at(bIndex).front() & 0xFFFF;
    sBuffer.at(bIndex).pop();
    uint32_t lo = sBuffer.at(bIndex).front() & 0xFFFF;
    sBuffer.at(bIndex).pop();
    out[oIndex] = (hi << 16) | lo;
    /*hi = sBuffer.at(bIndex).front() & 0xFFFF;
    sBuffer.at(bIndex).pop();
    lo = sBuffer.at(bIndex).front() & 0xFFFF;
    sBuffer.at(bIndex).pop();
    out[oIndex+1] = (hi << 16) | lo;*/
}

void BdaqRxCore::buildUserk(uint32_t* out, uint bIndex, uint oIndex) {
    // Building USERK frame (userkWordA and userkWordB)
    uint32_t hi = sBuffer.at(bIndex).front() & 0xFFFF;
    logger->critical("USERK: 0x{0:X}", sBuffer.at(bIndex).front());
    sBuffer.at(bIndex).pop();
    uint32_t lo = sBuffer.at(bIndex).front() & 0xFFFF;
    logger->critical("USERK: 0x{0:X}", sBuffer.at(bIndex).front());
    sBuffer.at(bIndex).pop();
    uint64_t userkWordA = (hi << 16) | lo;
    hi = sBuffer.at(bIndex).front() & 0xFFFF;
    logger->critical("USERK: 0x{0:X}", sBuffer.at(bIndex).front());
    sBuffer.at(bIndex).pop();
    lo = sBuffer.at(bIndex).front() & 0xFFFF;
    logger->critical("USERK: 0x{0:X}", sBuffer.at(bIndex).front());
    sBuffer.at(bIndex).pop();
    uint64_t userkWordB = (hi << 16) | lo;

    // Interpreting the USERK frame ang getting register data
    BdaqRxCore::userkDataT userkData = 
        interpretUserkFrame(userkWordA, userkWordB);
    std::vector<regDataT> regData = getRegData(userkData);
    // Encoding to YARR format
    for (const auto& reg : regData) {
        encodeToYarr(reg, out, oIndex); //inserts 2 words in the out stream.
        oIndex+=2;
    }
}

uint BdaqRxCore::buildStream(uint32_t* out, uint size) {
    uint procSize = 0;
    uint oIndex = 0;
    //logger->critical("buildStream()");
    //logger->info("size = {:d}", size);
    while (procSize < size) {
        for (uint bIndex=0; bIndex<activeChannels.size(); ++bIndex) {
            //logger->info("procSize = {:d}, bIndex = {:d}, bSize = {:d}, word = 0x{:X}", procSize, bIndex, sBuffer.at(bIndex).size(), sBuffer.at(bIndex).front());  
            if (sBuffer.at(bIndex).size() < 4) {
                out[oIndex  ] = 0xFFFF0000;
                out[oIndex+1] = 0xFFFF0000;
                procSize += sBuffer.at(bIndex).size();
            } else if ((sBuffer.at(bIndex).front() & USERK_FRAME_MASK) == USERK_FRAME_ID) {
                logger->critical("buildUserk()");
                buildUserk(out, bIndex, oIndex);
                procSize += 4;
                oIndex += 2;
            } else {
                buildData(out, bIndex, oIndex);
                procSize += 2; //4;
                oIndex += 1;
            }
            //oIndex += 2;
        }
    }
    return oIndex; // Output stream size
}

// USERK Decoding ==============================================================

// Extracts the data from an USERK frame (userkWordA + userkWordB).
BdaqRxCore::userkDataT BdaqRxCore::interpretUserkFrame(uint64_t userkWordA, 
                                                        uint64_t userkWordB) {  
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

// Get register data from AuroraKWord.
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

// Emulating the data format, expected by YARR, for a register read
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
