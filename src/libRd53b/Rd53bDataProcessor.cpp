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

    _blockIdx = 0; // Index of block, starting from 0
    _bitIdx = 0;   // Index of bit within block, starting from 0
    _data = nullptr;

    _isCompressedHitmap = true; // True by default
    _dropToT = false;           /* False by default */
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

uint64_t Rd53bDataProcessor::retrieve(const unsigned length, const bool checkEOS)
{
    if (unlikely(length == 0))
        return 0;
    // Should never happen: should be enough protection when going through the data stream
    if (unlikely((_blockIdx >> 1) > _curIn->words))
        logger->error("Data error: end of current packet reached while still reading stream!");

    if (checkEOS && (_data[0] >> 31) && _bitIdx == 1)
    {                // Corner case where end of event mark (0000000) is suppressed and there is no orphan bits
        _blockIdx--; // Roll back block index by 1. A new stream will start in the next loop iteration
        return 0;
    }
    uint64_t variable = 0;

    if (_bitIdx + length < BLOCKSIZE)
    { // Need to read in next block
        variable = (((_bitIdx + length) <= HALFBLOCKSIZE) || (_bitIdx >= HALFBLOCKSIZE)) ? ((_data[_bitIdx / HALFBLOCKSIZE] & (0xFFFFFFFFUL >> (_bitIdx - ((_bitIdx >> 5) << 5)))) >> ((((_bitIdx >> 5) + 1) << 5) - length - _bitIdx))
                                                                                         : (((_data[0] & (0xFFFFFFFFUL >> _bitIdx)) << (length + _bitIdx - HALFBLOCKSIZE)) | (_data[1] >> (BLOCKSIZE - length - _bitIdx)));
        _bitIdx += length; // Move bit index
    }
    else
    {
        // Check end of stream
        if (checkEOS && (((_blockIdx << 1) < _curIn->words && (_data[2] >> 31)) || ((_blockIdx << 1) >= _curIn->words))){
            return 0;
        }

        variable = (((_bitIdx < HALFBLOCKSIZE) ? (((_data[0] & (0xFFFFFFFFUL >> _bitIdx)) << HALFBLOCKSIZE) | _data[1]) : (_data[1] & (0xFFFFFFFFUL >> (_bitIdx - HALFBLOCKSIZE)))) << (length + _bitIdx - BLOCKSIZE));

        if (unlikely((_blockIdx >> 1) >= _curIn->words))
        {
            // Corner case that we are literally reading the stream until the last bit
            // if (_bitIdx + length == BLOCKSIZE)
                return variable;
            // Otherwise there is an error...
            // logger->error("Data error: end of current packet reached while still reading stream!");
        }

        variable |= (((_bitIdx + length) < (HALFBLOCKSIZE + BLOCKSIZE)) ? ((_data[2] & 0x7FFFFFFFUL) >> (95 - (_bitIdx + length))) : (((_data[2] & 0x7FFFFFFFUL) << (_bitIdx + length - 95)) | (_data[3] >> (127 - (length + _bitIdx)))));

        ++_blockIdx; // Increase block index
        _data = &_data[2];

        _bitIdx -= (63 - length); // Reset bit index. Since we always read the NS bit the index should always start from 1
    }

    return variable;
}

void Rd53bDataProcessor::rollBack(const unsigned length)
{
    if (unlikely(length == 0))
        return;
    if (_bitIdx >= (length + 1))
        _bitIdx -= length; // Keep in mind there is one extra bit from NS
    else
    {                              // Across block, roll back by length
        _bitIdx += (63 & ~length); /* Cannot handle length larger than 63 */
        _data = &_curIn->buf[2 * (--_blockIdx - 1)];
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
    /* Make qrow a global variable, to cover the corner case that 
    isneighbor is set to 1 after islast is set to 1 (due to too many
    hits in the same row */
    uint8_t qrow = 0;
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
            _blockIdx = 0;
            _bitIdx = 0;
            _data = &_curIn->buf[2 * _blockIdx];

            tag[channel] = (_data[0] >> 23) & 0xFF;
            ++_blockIdx; // Increase block index
            _bitIdx = 9; // Reset bit index = NS + tag
            curOut[channel]->newEvent(tag[channel], l1id[channel], bcid[channel]);
            //logger->info("New Stream, New Event: {} ", tag[channel]);
            events[channel]++;

            while (_blockIdx <= blocks)
            {
                // Start from getting core column
                const uint8_t ccol = retrieve(6, true);
                // if(_debug) std::cout << "Column number: " << HEXF(6, ccol) << std::endl;
                // End of stream marked with 000000 in current stream
                if (ccol == 0)
                {
                    if (unlikely(_blockIdx >= blocks))
                        break; // End of data processing
                    _data = &_curIn->buf[2 * _blockIdx];
                    if (unlikely(!(_data[0] >> 31 & 0x1)))
                        logger->error("Expect new stream while NS = 0");
                    tag[channel] = (_data[0] >> 23) & 0xFF;
                    ++_blockIdx; // Increase block index
                    _bitIdx = 9; // Reset bit index = NS + tag
                    curOut[channel]->newEvent(tag[channel], l1id[channel], bcid[channel]);
                    //logger->info("Same Stream, New Event: {} ", tag[channel]);
                    events[channel]++;
                    continue;
                }
                else if (ccol >= 0x38)
                { // Internal tag
                    tag[channel] = (ccol << 5) | retrieve(5);
                    // There is no L1ID and BCID in RD53B data stream. Has to come from somewhere else
                    curOut[channel]->newEvent(tag[channel], l1id[channel], bcid[channel]);
                    //logger->info("Same Stream, New Event: {} ", tag[channel]);
                    events[channel]++;
                    continue;
                }

                // Loop over all the hits
                uint16_t islast_isneighbor_qrow = 0;
                do
                {
                    islast_isneighbor_qrow = retrieve(10);
                    if (islast_isneighbor_qrow & 0x100)
                    {
                        ++qrow;
                        rollBack(8);
                    }
                    else
                    {
                        qrow = islast_isneighbor_qrow & 0xFF;
                    }

                    uint16_t hitmap = retrieve(16);
                    if (_isCompressedHitmap)
                    {
                        // First read 16-bit, then see whether it is enough
                        const uint16_t hitmap_raw = hitmap;
                        hitmap = (_LUT_BinaryTreeHitMap[hitmap_raw] & 0xFFFF);
                        const uint8_t hitmap_rollBack = ((_LUT_BinaryTreeHitMap[hitmap_raw] & 0xFF000000) >> 24);

                        /* If the hit map is not fully covered yet */
                        if (hitmap_rollBack > 0)
                        {
                            /* Remove the offset and read the second row */
                            if (hitmap_rollBack != 0xff)
                                rollBack(hitmap_rollBack);
                            const uint16_t rowMap = retrieve(14);
                            hitmap |= (_LUT_BinaryTreeRowHMap[rowMap] << 8);
                            rollBack((_LUT_BinaryTreeRowHMap[rowMap] & 0xFF00) >> 8);
                        }
                        /* Otherwise, remove the offset to read ToT */
                        else
                        {
                            rollBack((_LUT_BinaryTreeHitMap[hitmap_raw] & 0xFF0000) >> 16);
                        }
                    }

                    // Check whether it is precision ToT data
                    if (qrow >= 196)
                    {
                        // logger->info("Hit: ccol({}) qrow({}) hitmap (0x{:x})) ", ccol, qrow, hitmap);
                        for (unsigned ibus = 0; ibus < 4; ibus++)
                        {
                            uint8_t hitsub = (hitmap >> (ibus << 2)) & 0xF;
                            if (hitsub)
                            {
                                uint16_t ptot_ptoa_buf = 0xFFFF;
                                // PToT first 8 bits
                                for (unsigned iread = 0; iread < 4; iread++)
                                {
                                    if ((hitsub >> iread) & 0x1)
                                        ptot_ptoa_buf &= ~((~retrieve(4) & 0xF) << (iread << 2));
                                }

                                uint16_t PToT = ptot_ptoa_buf & 0x7FF;
                                uint16_t PToA = ptot_ptoa_buf >> 11;

                                if (events[channel] == 0)
                                {
                                    logger->warn("[{}] No header in data fragment!", channel);
                                    curOut[channel]->newEvent(666, l1id[channel], bcid[channel]);
                                    events[channel]++;
                                }

                                // Reverse enginner the pixel address
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
                        /* If drop ToT, the ToT value saved in the output event will be 0 */
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
                } while (!(islast_isneighbor_qrow & 0x200) && (_blockIdx <= blocks));
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
