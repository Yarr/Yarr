#include <iostream>
#include <sstream>
#include <algorithm>
#include <bitset>

#include "Rd53bDataProcessor.h"
#include "EventData.h"
#include "AllProcessors.h"

#include "logging.h"

// Lookup tables
#include "LUT_PlainHMapToColRow.h"
#include "LUT_BinaryTreeRowHMap.h"
#include "LUT_BinaryTreeHitMap.h"

using namespace RD53BDecoding;

#define likely(x) __builtin_expect((x), 1)
#define unlikely(x) __builtin_expect((x), 0)

const uint8_t PToT_maskStaging[4][4] = {
    {0, 1, 2, 3},
    {4, 5, 6, 7},
    {2, 3, 0, 1},
    {6, 7, 4, 5}};

namespace
{
    auto logger = logging::make_log("Rd53bDataProcessor");
}

bool rd53b_proc_registered =
    StdDict::registerDataProcessor("RD53B", []() { return std::unique_ptr<DataProcessor>(new Rd53bDataProcessor()); });

Rd53bDataProcessor::Rd53bDataProcessor()
{
    m_input = NULL;
    m_numThreads = std::thread::hardware_concurrency();

    _blockIdx = 0; // Index of the first 64-bit block that is not processed yet. Starting from 0
    _bitIdx = 0;   // Index of the first bit within the 64-bit block that is not processed yet. Starting from 0
    _data = nullptr; // Pointer to the 32-bit data word. E.g. _data[0] is the first half of the 64-bit block and _data[1] is the second half

    _isCompressedHitmap = true; // Whether the hit map is compressed (binary tree + Huffman coding) or not (raw 16-bit hit map)
    _dropToT = false;           // Whether ToT values are kept in the data stream
}

Rd53bDataProcessor::~Rd53bDataProcessor()
{
}

void Rd53bDataProcessor::init()
{
    SPDLOG_LOGGER_TRACE(logger, "");

    for (auto &it : *m_outMap)
    {
        activeChannels.push_back(it.first);
    }
}

void Rd53bDataProcessor::run()
{
    SPDLOG_LOGGER_TRACE(logger, "");

    unsigned int numThreads = m_numThreads;
    for (unsigned i = 0; i < numThreads; i++)
    {
        thread_ptrs.emplace_back(new std::thread(&Rd53bDataProcessor::process, this));
        logger->info("  -> Processor thread #{} started!", i);
    }
}

void Rd53bDataProcessor::join()
{
    for (auto &thread : thread_ptrs)
    {
        if (thread->joinable())
            thread->join();
    }
}

void Rd53bDataProcessor::process()
{
    while (true)
    {
        m_input->waitNotEmptyOrDone();

        process_core();
        // TODO the timing on these seems sensitive
        std::this_thread::sleep_for(std::chrono::microseconds(200));
        if (m_input->isDone())
        {
            std::this_thread::sleep_for(std::chrono::microseconds(200));
            process_core(); // this line is needed if the data comes in before done_flag is changed.
            break;
        }
    }

    process_core();
}

// Method for retrieving bits from data
uint64_t Rd53bDataProcessor::retrieve(const unsigned length, const bool checkEOS, const bool skipNSCheck)
{
    // Should never happen: reading 0 bit
    if (unlikely(length == 0))
        return 0;
    // Should never happen: end of packet reached when this function is called
    if (unlikely((_blockIdx << 1) > _curIn->words))
        logger->error("Data error: end of current packet reached while still reading stream!");

    // If need to check end of stream (checkEOS = true), need to protect for the corner case where end of event mark (0000000) is suppressed and there is 0 orphan bit at the end of the stream
    if (checkEOS && (_data[0] >> 31) && _bitIdx == 1)
    {                
        _blockIdx--; // Roll back block index by 1 because we are already in the new stream when reading the core-column number. A new stream will start in the next loop iteration
        return 0;
    }
    uint64_t variable = 0;  // Used to save the retrieved bits

    // Retrieving bits from the stream is highly non-trivial due to the varying length. There are several scenarios to cover. Using 64-bit instead of 32-bit data words can simplify the implementations
    // Check whether the bits to be retrieved are fully contained by the current 64-bit block
    if (_bitIdx + length < BLOCKSIZE)
    {
        // Whether the bits to be retrieved are fully contained by the first half or second half of the 64-bit block (each corresponds to one 32-bit data word), or they span across the two 32-bit data words
        variable = (((_bitIdx + length) <= HALFBLOCKSIZE) || (_bitIdx >= HALFBLOCKSIZE)) ? ((_data[_bitIdx / HALFBLOCKSIZE] & (0xFFFFFFFFUL >> (_bitIdx - ((_bitIdx >> 5) << 5)))) >> ((((_bitIdx >> 5) + 1) << 5) - length - _bitIdx))
                                                                                         : (((_data[0] & (0xFFFFFFFFUL >> _bitIdx)) << (length + _bitIdx - HALFBLOCKSIZE)) | (_data[1] >> (BLOCKSIZE - length - _bitIdx)));
        _bitIdx += length; // Move bit index
    }
    // If the bits overflow to the next 64-bit block
    else
    {
        // Check whether end of stream is reached if requested
        if (checkEOS && (((_blockIdx << 1) < _curIn->words && (_data[2] >> 31)) || ((_blockIdx << 1) >= _curIn->words))){
            return 0;  // If the NS = 1 in the next 64-bit block or the block index has reached to the end of packet, return end of stream mark 0b000000
        }

        // Retrieve all the remaining bits in the current 64-bit lock
        // Need different treatment depending on whether the bit index is in the first or second 32-bit data word of current 64-bit block
        variable = (((_bitIdx < HALFBLOCKSIZE) ? (((_data[0] & (0xFFFFFFFFUL >> _bitIdx)) << HALFBLOCKSIZE) | _data[1]) : (_data[1] & (0xFFFFFFFFUL >> (_bitIdx - HALFBLOCKSIZE)))) << (length + _bitIdx - BLOCKSIZE));

        // If the block index already reaches the end of the packet, stop here
        if (unlikely((_blockIdx << 1) >= _curIn->words))
        {
            // Corner case that we are literally reading the stream until the last bit
            // if (_bitIdx + length == BLOCKSIZE)
                return variable;
            // Otherwise there is an error...
            // logger->error("Data error: end of current packet reached while still reading stream!");
        }

        // Check the NS bit of the next 64-bit block
        if (unlikely(((_data[2] >> 31) & 0x1)))
        {
            // We are over-drafting bits and would expect non-zero probablitiy of running into the end of the stream. In this case, stop retrieving more bits and return the current value
            if (skipNSCheck)
            {
                ++_blockIdx;              // Increase block index
                _data = &_data[2];        // Move data pointer to the next block
                _bitIdx -= (63 - length); // Still move the bit index as if we have retrieved the overflow bits in the next block. The index will be rolled back in the process method
                return variable;
            }
            // Otherwise only throw an error essage. Keep reading the next block neglecting the NS bit
            // TODO: apply corrective action?
            else
                logger->error("Expect unfinished stream while NS = 1");
        }

        // Retrieve the bits overflow to the next 64-bit data block. Here we need to separate the cases where the overflow bits are fully contained in the first 32-bit data word, or extend to the second 32-bit data word
        // This check is needed because the maximum number of bits retrieved could be 64 bits (reading ToT for all 16 pixels from a full hit map)
        // Can be simplified if the data word is 64 instead of 32 bits, or if the ToT is read pixel-by-pixel (in which case the maximum number of bits retrieved will always be below 32 bits)
        variable |= (((_bitIdx + length) < (HALFBLOCKSIZE + BLOCKSIZE)) ? ((_data[2] & 0x7FFFFFFFUL) >> (95 - (_bitIdx + length))) : (((_data[2] & 0x7FFFFFFFUL) << (_bitIdx + length - 95)) | (_data[3] >> (127 - (length + _bitIdx)))));

        ++_blockIdx;              // Increase block index
        _data = &_data[2];        // Move data pointer to the next block
        _bitIdx -= (63 - length); // Move bit index. Note the NS bit should also be counted
    }

    return variable;
}

// Method for rolling back bit index
void Rd53bDataProcessor::rollBack(const unsigned length)
{
    // Should never happen: roll-back length = 0
    if (unlikely(length == 0))
        return;
    // After rolling back the index is still within the block. Keep in mind there is one extra bit from NS
    if (_bitIdx >= (length + 1))
        _bitIdx -= length;
    // Rolling back to the previous block
    else
    {                              
        _bitIdx += (63 & ~length);  // Correct the bit index. Keep in mind there is NS bit
        _data = &_curIn->buf[2 * (--_blockIdx - 1)];  // Also roll back the block index and data word pointer
    }
}

void Rd53bDataProcessor::process_core()
{
    // TODO put data from channels back into input, so other processors can use it
    for (auto &i : activeChannels)
    {
        tag[i] = 666;
        l1id[i] = 666;
        bcid[i] = 666;
        wordCount[i] = 0;
        hits[i] = 0;
    }

    unsigned dataCnt = 0;
    uint8_t qrow = 0;  // Quarter-row index
    while (!m_input->empty())
    {
        // Get data containers
        auto curInV = m_input->popData();
        if (curInV == nullptr)
            continue;

        // Create Output Container
        std::map<unsigned, std::unique_ptr<FrontEndData>> curOut;
        std::map<unsigned, int> events;
        for (unsigned i = 0; i < activeChannels.size(); i++)
        {
            curOut[activeChannels[i]].reset(new FrontEndData(curInV->stat));
            events[activeChannels[i]] = 0;
        }
        /* For now use only the first active channel */
        /* Support for multi-channel will be added later */
        unsigned channel = activeChannels[0];
        unsigned size = curInV->size();
        for (unsigned c = 0; c < size; c++)
        {
            _curIn.reset(new RawData(curInV->adr[c], curInV->buf[c], curInV->words[c]));

            const unsigned blocks = _curIn->words / 2;
            dataCnt += _curIn->words;
            // Start data processing by setting the index and pointer to initial values
            _blockIdx = 0;
            _bitIdx = 0;
            _data = &_curIn->buf[2 * _blockIdx];

            // Get event tag. TODO: add support of chip ID
            tag[channel] = (_data[0] >> 23) & 0xFF;
            ++_blockIdx; // Increase block index
            _bitIdx = 9; // Reset bit index = NS + tag

            // Create a new event
            // RD53B does not have L1 ID and BCID output in data stream, so these are dummy values for now
            curOut[channel]->newEvent(tag[channel], l1id[channel], bcid[channel]);
            //logger->info("New Stream, New Event: {} ", tag[channel]);
            events[channel]++;

            // Start looping over data words in the current packet
            while (_blockIdx <= blocks)
            {
                // Start from getting core column index
                // This is also the ONLY place where we check end-of-stream. In other places we simply assuming continuation of stream, and will throw an error message if the end of stream is somehow reached.
                const uint8_t ccol = retrieve(6, true);
                // if(_debug) std::cout << "Column number: " << HEXF(6, ccol) << std::endl;

                // End of stream is marked with 0b000000. This is ensured in software in spite of the chip orphan bit configuration
                if (ccol == 0)
                {
                    if (unlikely(_blockIdx >= blocks))
                        break; // End of data processing as end of packet is reached

                    // ++++++++++++++ Start a new stream ++++++++++++++++
                    // Move the data word pointer to the next 64-bit block. Note the meaning of block index is the first block that is *unprocessed*
                    _data = &_curIn->buf[2 * _blockIdx];

                    // Check the NS bit. If it is not 0, throw an error message
                    // TODO: implement corrective action?
                    if (unlikely(!(_data[0] >> 31 & 0x1)))
                        logger->error("Expect new stream while NS = 0");
                    // Get 8-bit event tag
                    tag[channel] = (_data[0] >> 23) & 0xFF;
                    ++_blockIdx; // Increase block index
                    _bitIdx = 9; // Reset bit index = NS + tag. TODO: add support for chip ID
                    // Create a new event
                    curOut[channel]->newEvent(tag[channel], l1id[channel], bcid[channel]);
                    //logger->info("Same Stream, New Event: {} ", tag[channel]);
                    events[channel]++;
                    continue;
                }
                else if (ccol >= 0x38)  // Internal tag
                {
                    // Internal tag is 11-bit. So need to retrieve 5 more bits
                    tag[channel] = (ccol << 5) | retrieve(5);

                    // Create a new event
                    // There is no L1ID and BCID in RD53B data stream. Currently put dummy values
                    curOut[channel]->newEvent(tag[channel], l1id[channel], bcid[channel]);
                    //logger->info("Same Stream, New Event: {} ", tag[channel]);
                    events[channel]++;
                    continue;
                }

                // Loop over all the hits in the current event
                uint16_t islast_isneighbor_qrow = 0;
                do
                {
                    // ###### Step 1. read islast, isneighbor, and qrow ######
                    // Read islast, isneighbor, and 8-bit qrow together, in total 10 bits
                    // Note the qrow index will be absent if isneighbor = 1
                    // Reading more bits is nonetheless found creating larger bandwidth
                    islast_isneighbor_qrow = retrieve(10, false, true);

                    // If isneighbor = 1, roll back 8 bits to cancel the qrow readout
                    if (islast_isneighbor_qrow & 0x100)
                    {
                        ++qrow;
                        rollBack(8);
                    }
                    // Otherwise keep the qrow value
                    else
                    {
                        qrow = islast_isneighbor_qrow & 0xFF;
                    }
                    // ############ Step 2. read hit map ############
                    // The hit map decoding is based on look-up table
                    uint16_t hitmap = retrieve(16, false, true);

                    // Compressed hit map
                    if (_isCompressedHitmap)
                    {
                        // Since the hit map length is unpredictable in the compressed case, here we will retrieve 16-bit and feed it into the look-up table (LUT). 16-bit is enough to cover the hit-map in most cases. Using more or less bits are found to be less efficient
                        // The LUT entry is 32-bit and contains three parts
                        // 1. 8-bit indicating whether the hit map is fully contained in the 16-bit retrieved. If not, (16 - length of the hit map for the first row) is saved (maximum length of the hit map for the first row is 16)
                        // 2. 8-bit saving (16 - length of the hit map) if the hit map is fully contained in the 16 bits
                        // 3. 16-bit decompressed raw hit map, one bit for each pixel
                        const uint16_t hitmap_raw = hitmap;
                        hitmap = (_LUT_BinaryTreeHitMap[hitmap_raw] & 0xFFFF);
                        const uint8_t hitmap_rollBack = ((_LUT_BinaryTreeHitMap[hitmap_raw] & 0xFF000000) >> 24);

                        // If the hit map is not fully contained in 16 bits
                        if (hitmap_rollBack > 0)
                        {
                            // Remove the offset and read the hit map for the second row
                            if (hitmap_rollBack != 0xff)
                                rollBack(hitmap_rollBack);
                            // The length of the hit map for the second row can be 14 bits maximum
                            const uint16_t rowMap = retrieve(14, false, true);
                            // This part is handled by another LUT focusing on decoding hit map for a single row (8 pixels)
                            // The entry of this LUT contains two parts
                            // 1. 8-bit saving (14 - length of the hit map for the second row)
                            // 2. 8-bit raw hit map for the row (1 bit for each pixel)
                            hitmap |= (_LUT_BinaryTreeRowHMap[rowMap] << 8);
                            rollBack((_LUT_BinaryTreeRowHMap[rowMap] & 0xFF00) >> 8);
                        }
                        // Otherwise, remove the offset as preparation for reading ToT
                        else
                        {
                            rollBack((_LUT_BinaryTreeHitMap[hitmap_raw] & 0xFF0000) >> 16);
                        }
                    }

                    // ############ Step 3. read ToT ############
                    // Check whether it is precision ToT (PToT) data. PToT data is indicated by unphysical qrow index 196
                    if (qrow >= 196)
                    {
                        // logger->info("Hit: ccol({}) qrow({}) hitmap (0x{:x})) ", ccol, qrow, hitmap);
                        for (unsigned ibus = 0; ibus < 4; ibus++)
                        {
                            uint8_t hitsub = (hitmap >> (ibus << 2)) & 0xF;
                            if (hitsub)
                            {
                                uint16_t ptot_ptoa_buf = 0xFFFF;
                                // There is a bug in ITkPix-V1 (First version of ATLAS RD53B) that suppress "0b1111" in PToT data. Therefore we have to add these missing bits back by hand
                                for (unsigned iread = 0; iread < 4; iread++)
                                {
                                    if ((hitsub >> iread) & 0x1)
                                        ptot_ptoa_buf &= ~((~retrieve(4) & 0xF) << (iread << 2));
                                }

                                // PToT data for each pixel contain 16 bits. First 5 bits are for time of arrival, and 11 bits for time over threshold. Both are counted in 640 MHz clock
                                uint16_t PToT = ptot_ptoa_buf & 0x7FF;
                                uint16_t PToA = ptot_ptoa_buf >> 11;

                                if (events[channel] == 0)
                                {
                                    logger->warn("[{}] No header in data fragment!", channel);
                                    curOut[channel]->newEvent(666, l1id[channel], bcid[channel]);
                                    events[channel]++;
                                }

                                // Reverse enginner the pixel address using mask staging
                                static unsigned maskLoopIndex = 0;
                                static bool check_loop_index = true;
                                if(check_loop_index) {
                                    for (unsigned loop=0; loop<curOut[channel]->lStat.size(); loop++) {
                                        if (curOut[channel]->lStat.getStyle(loop) == LOOP_STYLE_MASK) {
                                            maskLoopIndex = loop;
                                            check_loop_index = false;
                                            break;
                                        }
                                    }
                                }
                                const unsigned step = curOut[channel]->lStat.get(maskLoopIndex);
                                const uint16_t pix_col = (ccol - 1) * 8 + PToT_maskStaging[step % 4][ibus] + 1;
                                const uint16_t pix_row = step / 2 + 1;
                                // logger->info("Hit: row({}) col({}) tot({}) ", pix_row, pix_col, PToT);
                                // curOut[channel]->curEvent->addHit(pix_row, pix_col, (PToT >> 4) - 1);

                                curOut[channel]->curEvent->addHit({pix_col, pix_row, static_cast<uint16_t>(PToT | (PToA << 11))});
                            }
                        }
                    }
                    else
                    {
                        // If drop ToT, the ToT value saved in the output event will be 0
                        // Otherwise we will translate the raw 16-bit hit map into number of hits and pixel addresses using yet another LUT
                        uint64_t ToT = _dropToT ? 0 : retrieve(_LUT_PlainHMap_To_ColRow_ArrSize[hitmap] << 2);
                        if (_LUT_PlainHMap_To_ColRow_ArrSize[hitmap] == 0)
                        {
                            logger->warn("Received fragment with no ToT! ({} , {})", ccol, qrow);
                        }
                        for (unsigned ihit = 0; ihit < _LUT_PlainHMap_To_ColRow_ArrSize[hitmap]; ++ihit)
                        {
                            const uint8_t pix_tot = (ToT >> (ihit << 2)) & 0xF;
                            // First pixel is 1,1, last pixel is 400,384
                            const uint16_t pix_col = ((ccol - 1) * 8) + (_LUT_PlainHMap_To_ColRow[hitmap][ihit] >> 4) + 1;
                            const uint16_t pix_row = ((qrow)*2) + (_LUT_PlainHMap_To_ColRow[hitmap][ihit] & 0xF) + 1;

                            // For now fill in events without checking whether the addresses are valid
                            if (events[channel] == 0)
                            {
                                logger->warn("[{}] No header in data fragment!", channel);
                                curOut[channel]->newEvent(666, l1id[channel], bcid[channel]);
                                events[channel]++;
                            }

                            curOut[channel]->curEvent->addHit({pix_col,pix_row,pix_tot});
                            //logger->info("Hit: row({}) col({}) tot({}) ", pix_row, pix_col, pix_tot);
                            hits[channel]++;
                        }
                    }
                } while (!(islast_isneighbor_qrow & 0x200) && (_blockIdx <= blocks)); // Need to keep track of block index to make sure it is within the packet while we loop over the hits in the event
            }
            //logger->info("total number of hits: {}", hits[channel]);
        }
        // Push data out
        for (unsigned i = 0; i < activeChannels.size(); i++)
        {
            if (events[activeChannels[i]] > 0)
            {
                m_outMap->at(activeChannels[i]).pushData(std::move(curOut[activeChannels[i]]));
            }
            else
            {
                // Maybe wait for end of method instead of deleting here?
                curOut[activeChannels[i]].reset();
            }
        }
        //Cleanup
    }
}
