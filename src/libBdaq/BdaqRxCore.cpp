#include "BdaqRxCore.h"

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
    verbose = false;
    userkCounter = 0;
    
    isEventHeader = false;
    isHighWord = true;

    mSetupMode = true;
}

void BdaqRxCore::setupMode() {
    mSetupMode = true;
}

void BdaqRxCore::runMode() {
    mSetupMode = false;
}

void BdaqRxCore::setRxEnable(uint32_t val) {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << " : Value 0x" << std::hex << val << std::dec << std::endl;
}

void BdaqRxCore::setRxEnable(std::vector<uint32_t>) {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
}

void BdaqRxCore::maskRxEnable(uint32_t val, uint32_t mask) {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << " : Value 0x" << std::hex << val 
        << ", Mask 0x" << mask << std::dec << std::endl;    
}

void BdaqRxCore::checkRxSync() {
	uint time = 0;
	while (time < 1000 && auroraRx.getRxReady() == false) {
		++time;
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	if (auroraRx.getRxReady()) {
		std::cout << "-> Aurora link is synchronized!" << std::endl;
	}
	else {
        cmd.reset();
		throw std::runtime_error("Aurora link DID NOT synchronize!");
	}
}

void printWords(std::vector<uint32_t>& buf, std::string id="", std::size_t max=30) {
    std::size_t wCount = buf.size();
    std::cout << "---> " << id << " Data Stream: " << std::endl;
    std::cout << "size: " << wCount << std::endl;
    uint col = 0;
    std::size_t count = (wCount > max ? max : wCount);
    for (std::size_t i=0; i<count; ++i) {
        std::cout << std::hex << "0x" << buf[i] << ", ";
        ++col;
        if (col == 10) {
            col = 0;
            std::cout << std::endl;
        }
    }
    std::cout << std::dec << std::endl;    
}

void printWords(uint32_t* buf, std::size_t wCount, std::string id="", std::size_t max=30) {
    std::cout << "---> " << id << " Data Stream: " << std::endl;
    std::cout << "size: " << wCount << std::endl;
    uint col = 0;
    std::size_t count = (wCount > max ? max : wCount);
    for (std::size_t i=0; i<count; ++i) {
        std::cout << std::hex << "0x" << buf[i] << ", ";
        ++col;
        if (col == 10) {
            col = 0;
            std::cout << std::endl;
        }
    }
    std::cout << std::dec << std::endl;    
}

void BdaqRxCore::printStats() {
    std::cout << "totalWords  : " << totalWords  << std::endl 
              << "userkWords  : " << userkWords  << std::endl
              << "dataWords   : " << dataWords   
                << " ===> headerWords: " << headerWords 
                << ", hitWords: " << hitWords << std::endl;
}

void BdaqRxCore::printBufferStatus() {
    std::cout << "---------------------------------------------" << std::endl;
    std::cout << "TCP buffer in bytes: " << fifo.getTcpSize() << std::endl;
    std::cout << "TCP Available words: " << fifo.getAvailableWords() << std::endl;
    std::cout << "---------------------------------------------" << std::endl;
}

RawData* BdaqRxCore::readData() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    std::size_t wCount = fifo.getAvailableWords();
    /*if (wCount % 2) {
        std::cout << "wCount: " << wCount << std::endl;
        std::cin.get();
    }*/
    // We can only decode pair of words
    //wCount = wCount - (wCount % 2); 
    std::vector<uint32_t> inBuf;
    fifo.readData(inBuf, wCount);

    if (wCount > 0) {
        std::size_t inSize = wCount;
        //printWords(inBuf, "inBuf", 60);
        // outBuf size is always < wCount. 
        uint32_t* outBuf = new uint32_t[wCount]; 
        // now wCount has the number of decoded (thus, usable) words
        wCount = decode(inBuf, outBuf);
        if (wCount > 0) {
            std::size_t outSize = wCount;
            if (outSize > inSize/2+1) {
                std::cout << "inSize: " << inSize << ", outSize: " << outSize << std::endl;
                std::cin.get();
            }
            //printWords(outBuf, wCount, "outBuf");
            //printStats();
            return new RawData(0x0, outBuf, wCount);
        } 
        return NULL;
    }
    return NULL;
}

void BdaqRxCore::flushBuffer() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;   
    //readout.reset();
    auroraRx.resetLogic();
    //std::this_thread::sleep_for(std::chrono::milliseconds(200));
    fifo.flushBuffer();
}

uint32_t BdaqRxCore::getDataRate() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    return 0;
}

bool BdaqRxCore::isBridgeEmpty() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
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
//
// Thought of using setupMode()...

unsigned int BdaqRxCore::decode(std::vector<uint32_t>& in, uint32_t* out) {
    
    unsigned int index = 0;
    

    for (const auto& word : in) {
        ++totalWords; // Counting 32-bit BDAQ Readout words

        if (word & TRIGGER_ID) {
            throw std::runtime_error("TLU data is not yet supported.");
        } 
        if (checkTDC(word)) {
            throw std::runtime_error("TDC data is not yet supported.");
        } 
        if (word & USERK_FRAME_ID) {
            ++userkWords; // Counting 32-bit BDAQ readout words which are USERK
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
            ++dataWords; //Counting 32-bit RD53A Data words
            dataWord = (dataWord << 16) | (word & 0xFFFF);
            isHighWord = true; // Next is high word
        }

        if (isEventHeader) {
            ++headerWords; // Counting 32-bit RD53A Event Headers
            isEventHeader = false;
            out[index] = dataWord;
            ++index;
        } else {
            ++hitWords; // Counting 32-bit RD53A Hit Data
            out[index] = dataWord;
            ++index;
        }

    }
    return index;
}

// USERK Decoding ==============================================================

unsigned int BdaqRxCore::decodeUserk(const uint32_t& word, uint32_t* out, 
                                        unsigned int index) {
    buildUserkFrame(word, userkCounter);
    // 4 USERK readout words are necessary to build an USERK frame.
    if (userkCounter == 3) {
        userkCounter = 0;
        BdaqRxCore::userkDataT userkData = interpretUserkFrame();
        std::vector<regDataT> regData = getRegData(userkData);
        // regData might contain data from either 1 or 2 registers.
        // The code below will insert this data into the output stream
        // using one of the YARR expected formats.                                        
        if (regData.size() == 0) 
            throw std::runtime_error("regData.size() = 0.");
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

    if (verbose) {
        std::cout << __PRETTY_FUNCTION__ << ": " << std::endl;
        for (const auto& p : regData) {
            std::cout << "[("
                << +p.Address << ", "
                << +p.Data << ")]" << std::endl;
        }
    }

    return regData;
}

// Emulating the YARR expected readout format for a register frame
void BdaqRxCore::encodeToYarr(BdaqRxCore::regDataT in, uint32_t* out, 
                                unsigned int index) {
    out[index  ] = 0x55000000;
    out[index+1] = (in.Address & 0x3FF) << 16 | 
                    in.Data    & 0xFFFF;                               
}

// TDC Data Decoding ===========================================================

bool BdaqRxCore::checkTDC(const uint32_t& word) {
    if ((word & TDC_HEADER_MASK == TDC_ID_0) || 
        (word & TDC_HEADER_MASK == TDC_ID_1) ||
        (word & TDC_HEADER_MASK == TDC_ID_2) ||
        (word & TDC_HEADER_MASK == TDC_ID_3)) return true;
    else
        return false;
}
