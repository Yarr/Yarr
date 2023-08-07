#include <thread>

#include <chrono>
#include <iostream>
#include <algorithm>

#include "BdaqRxCore.h"
#include "Bdaq.h"

#include "BdaqTxCore.h"
#include "StdDataLoop.h"

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

#define HEADER_ID        0x00010000
#define NEW_STREAM_ID    0x00008000

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


std::vector<RawDataPtr> BdaqRxCore::readData() {
     uint size = fifo.getAvailableWords();
     std::vector<RawDataPtr> dataVec;
     std::vector<uint32_t> inBuf;
     fifo.readData(inBuf, size); 

     if (chipType == 0){ // it is RD53A
         if (size > 0) {
             for (const auto& word : inBuf) {
                 if ((word & TLU_MASK) == TLU_ID) {
                     logger->critical("TLU data is not supported.");
                     exit(-1);
                 }
                 if (checkTDC(word)) {
                     logger->critical("TDC data is not supported.");
                     exit(-1);
                 }
                 uint channel = 0;
                 for (auto& channelId : activeChannels) {
                     // Testing Aurora RX Identifier
                     if (((word >> 20) & 0xF) == channelId) {
                         dataMap[channelId].push_back(word);
                     }
                     ++channel;
                 }
             }

             uint channel = 0;
             for (auto& data_map: dataMap) {
                 channel = data_map.first;
                 if (data_map.second.size() != 0){
                     if ((data_map.second.size() > 3) &&
                         (data_map.second.at(0) & USERK_FRAME_MASK) == USERK_FRAME_ID) {

                         // Building USERK frame (userkWordA and userkWordB)
                         uint32_t hi = data_map.second.front() & 0xFFFF;
                         logger->debug("USERK: 0x{0:X}", data_map.second.front());
                         data_map.second.erase(data_map.second.begin());
                         uint32_t lo = data_map.second.front() & 0xFFFF;
                         logger->debug("USERK: 0x{0:X}", data_map.second.front());
                         data_map.second.erase(data_map.second.begin());
                         uint64_t userkWordA = (hi << 16) | lo;
                         hi = data_map.second.front()  & 0xFFFF;
                         logger->debug("USERK: 0x{0:X}", data_map.second.front());
                         data_map.second.erase(data_map.second.begin());
                         lo = data_map.second.front() & 0xFFFF;
                         logger->debug("USERK: 0x{0:X}", data_map.second.front());
                         data_map.second.erase(data_map.second.begin());
                         uint64_t userkWordB = (hi << 16) | lo;

                         // Interpreting the USERK frame and getting register data
                         BdaqRxCore::userkDataT_RD53A userkData_RD53A =
                         interpretUserkFrame_RD53A(userkWordA, userkWordB);
                         std::vector<regDataT> regData = getRegData_RD53A(userkData_RD53A);
                         for (const auto& reg : regData) {
                             dataMap_copy[channel].clear();
                             dataMap_copy[channel].push_back(0x55000000);
                             dataMap_copy[channel].push_back((reg.Address & 0x3FF) << 16 | (reg.Data  & 0xFFFF));
                             dataMap[channel].clear();
                         }
                     }else if(data_map.second.size() > 1){
                         // build Data
                         uint32_t dataWord;
                         uint64_t hi;
                         uint64_t lo;
                         dataMap_copy[channel].clear();
                         while(data_map.second.size() > 1){
                             hi = data_map.second.front() & 0xFFFF;
                             data_map.second.erase(data_map.second.begin());
                             lo = data_map.second.front() & 0xFFFF;
                             data_map.second.erase(data_map.second.begin());
                             dataWord = (hi << 16) | lo;
                             dataMap_copy[channel].push_back(dataWord);
                         }
                     }else{
                         dataMap_copy[channel].clear();
                         uint32_t null_dataWord = 0xFFFF0000;
                         dataMap_copy[channel].push_back(null_dataWord);
                     }
                 }else{
                     dataMap_copy[channel].clear();
                     uint32_t null_dataWord = 0xFFFF0000;
                     dataMap_copy[channel].push_back(null_dataWord);
                 }
             }

             for (const uint32_t channelId : activeChannels) {
                 RawDataPtr data;
                 data = std::make_shared <RawData> (channelId, dataMap_copy[channelId]);
                 data->getAdr() = channelId;  // set rx channel number as address for data
                 dataVec.push_back(data);
                 dataMap_copy[channelId].clear();
             }
         }else{
             return std::vector<RawDataPtr>();
         }
                                   
     }else if (chipType == 1){ // it is ItkPixV1 (RD53B)
         if (size > 0) {
             for (const auto& word : inBuf) {
                 if ((word & TLU_MASK) == TLU_ID) {
                     logger->critical("TLU data is not supported.");
                     exit(-1);
                 }
                 if (checkTDC(word)) {
                     logger->critical("TDC data is not supported.");
                     exit(-1);
                 }
                 for (auto& channelId : activeChannels) {
                     if (((word >> 20) & 0xF) == channelId) {
                         // Testing Aurora RX Identifier
                         dataMap[channelId].push_back(word);
                     }
                 }
             }

             uint channel = 0;
             for (auto& data_map: dataMap) {
                 channel = data_map.first;
                 if (data_map.second.size() != 0){
                     if ((data_map.second.size() > 3) &&
                         (data_map.second.front() & USERK_FRAME_MASK) == USERK_FRAME_ID) {
                         uint userk_word_cnt = 0 ;
                         uint64_t userkWordA, userkWordB, userk_word;
                         while(data_map.second.size() > 0){
                             // Building USERK frame (userkWordA and userkWordB)
                             if(userk_word_cnt == 0){
                                 userk_word = data_map.second.front() & 0x0FFF;
                                 userkWordA = data_map.second.front();
                                 data_map.second.erase(data_map.second.begin());
                             }else{
                                 userk_word = userk_word << 16 | data_map.second.front() & 0xFFFF;
                                 data_map.second.erase(data_map.second.begin());
                             }
                             userk_word_cnt++;
                             if(userk_word_cnt == 4){
                                 userkWordB = userk_word;
                                 BdaqRxCore::userkDataT_RD53B userkData_RD53B =
                                 interpretUserkFrame_RD53B(userkWordA, userkWordB);
                                 std::vector<regDataT> regData = getRegData_RD53B(userkData_RD53B);
                                 for (const auto& reg : regData) {
                                     dataMap_copy[channel].clear();
                                     dataMap_copy[channel].push_back(0x55000000);
                                     dataMap_copy[channel].push_back((reg.Address & 0x3FF) << 16 | (reg.Data  & 0xFFFF));
                                     dataMap[channel].clear();
                                 }
                                 userk_word_cnt = 0;
                             }
                         }


                     }else if(data_map.second.size() > 1){
                         // build Data
                         uint32_t dataWordFirstHalf;
                         uint32_t dataWordSecondHalf;
                         uint32_t HiOne;
                         uint32_t LoOne;
                         uint32_t HiTwo;
                         uint32_t LoTwo;

                         while(data_map.second.size() > 3){
                             HiOne = data_map.second.front() & 0xFFFF;
                             data_map.second.erase(data_map.second.begin());
                             LoOne = data_map.second.front() & 0xFFFF;
                             data_map.second.erase(data_map.second.begin());
                             dataWordFirstHalf = (HiOne << 16) | LoOne;
                             dataMap_copy[channel].push_back(dataWordFirstHalf);

                             HiTwo = data_map.second.front() & 0xFFFF;
                             data_map.second.erase(data_map.second.begin());
                             LoTwo = data_map.second.front() & 0xFFFF;
                             data_map.second.erase(data_map.second.begin());
                             dataWordSecondHalf = (HiTwo << 16) | LoTwo;
                             dataMap_copy[channel].push_back(dataWordSecondHalf);
                         }

                     }
                 }
             }

             for (const uint32_t channelId : activeChannels) {
                 if(dataMap_copy[channelId].size() > 0){
                     RawDataPtr data;
                     data = std::make_shared <RawData> (channelId, dataMap_copy[channelId]);
                     data->getAdr() = channelId;  // set rx channel number as address for data
                     dataVec.push_back(data);
                     dataMap_copy[channelId].clear();
                 }
             }
             dataMap_copy.clear();

         }else {
             return std::vector<RawDataPtr>();
         }
     }
     return dataVec;
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

void BdaqRxCore::initSortBuffer() {
    sBuffer.clear();
    for (uint c : activeChannels) {
        sBuffer.push_back(std::queue<uint32_t>());
    }
}


// USERK Decoding ==============================================================
// Extracts the data from an USERK frame (userkWordA + userkWordB) for RD53A.

BdaqRxCore::userkDataT_RD53A BdaqRxCore::interpretUserkFrame_RD53A(uint64_t userkWordA,
                                                        uint64_t userkWordB) {
    uint64_t userkBlock, Data0, Data1;
    userkDataT_RD53A u;

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


// Extracts the data from an USERK frame (userkWordA + userkWordB) for RD53B.
BdaqRxCore::userkDataT_RD53B BdaqRxCore::interpretUserkFrame_RD53B(uint64_t userkWordA,
                                                        uint64_t userkWordB) {
    uint64_t Data0, Data1;
    userkDataT_RD53B u;

    logger->debug("userkWordA = {}", userkWordA);
    logger->debug("userkWordB = {}", userkWordB);

    Data1 = (userkWordB >> 34) & 0x7FFFFFF;
    Data0 = (userkWordB >> 8) & 0x7FFFFFF;
    u.Status = (userkWordA >> 12) & 0x3;
    u.ChipID = (userkWordA >> 14) & 0x3;
    u.AuroraKWord = userkWordB & 0xFF;
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


// Get register data from AuroraKWord for RD53A.
std::vector<BdaqRxCore::regDataT> BdaqRxCore::getRegData_RD53A(BdaqRxCore::userkDataT_RD53A in) {
    BdaqRxCore::regDataT o;
    std::vector<BdaqRxCore::regDataT> regData;

    logger->debug("AuroraKWord = {}", in.AuroraKWord);

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


// Get register data from AuroraKWord for RD53B.
std::vector<BdaqRxCore::regDataT> BdaqRxCore::getRegData_RD53B(BdaqRxCore::userkDataT_RD53B in) {
    BdaqRxCore::regDataT o;
    std::vector<BdaqRxCore::regDataT> regData;

    logger->debug("AuroraKWord = {}", in.AuroraKWord);

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

