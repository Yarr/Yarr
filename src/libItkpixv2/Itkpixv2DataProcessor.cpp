#include <iostream>
#include <sstream>
#include <algorithm>
#include <bitset>

#include "Itkpixv2DataProcessor.h"
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
    auto logger = logging::make_log("Itkpixv2DataProcessor");
}

bool itkpixv2_proc_registered =
    StdDict::registerDataProcessor("ITKPIXV2", []() { return std::unique_ptr<DataProcessor>(new Itkpixv2DataProcessor()); });

Itkpixv2DataProcessor::Itkpixv2DataProcessor()
{
    m_input = NULL;

    _wordIdx = 0; // Index of the first 64-bit block. Starting from 0
    _bitIdx = 0;   // Index of the first bit within the 64-bit block. Starting from 0
    _rawDataIdx = 0;       // Index of the first raw data. Starting from 0
    _data = nullptr; // Pointer to the 32-bit data word. E.g. _data[0] is the first half of the 64-bit block and _data[1] is the second half

    _isCompressedHitmap = true; // Whether the hit map is compressed (binary tree + Huffman coding) or not (raw 16-bit hit map)
    _dropToT = false;           // Whether ToT values are kept in the data stream
    _enChipId = false;
    _chipIdShift = 0;
    _chipId = 15;
    _streamMask = 0x7FFFFFFF;

    // Data stream components
    _ccol = 0;
    // Core column index starts from 1. So _qrow[0] will never be used
    // Use 54 as total number of core columns to be compatible with CMS chip geometry
    for (int i = 0; i <= 54; i++)
        _qrow[i] = 0;
    _islast_isneighbor = 0;
    _hitmap = 0;
    _ToT = 0;

    // Status
    _status = INIT;
}

Itkpixv2DataProcessor::~Itkpixv2DataProcessor()= default;

void Itkpixv2DataProcessor::init()
{
    SPDLOG_LOGGER_TRACE(logger, "");

    _tag = 666;
    _l1id = 666;
    _bcid = 666;
    _wordCount = 0;
    _hits = 0;

    // Load decoder specific bits
    _isCompressedHitmap = (m_feCfg->DataEnRaw.read() == 0 ? true : false);
    _dropToT = (m_feCfg->DataEnBinaryRo.read() == 1 ? true : false);
    _enChipId = (m_feCfg->EnChipId.read() == 1 ? true : false);
    _chipIdShift = (m_feCfg->EnChipId.read() == 1 ? 2 : 0);
    _chipId = m_feCfg->getChipId() & 0x3;
    _streamMask = (_enChipId ? 0x1FFFFFFF : 0x7FFFFFFF);
}

void Itkpixv2DataProcessor::run()
{
    SPDLOG_LOGGER_TRACE(logger, "");

    thread_ptr.reset(new std::thread(&Itkpixv2DataProcessor::process, this));
}

void Itkpixv2DataProcessor::join()
{
    thread_ptr->join();
}

void Itkpixv2DataProcessor::process()
{
    logger->info("Started raw data processor thread for {}.", m_feCfg->getName());
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
    logger->info("Finished raw data processor thread for {}.", m_feCfg->getName());
}

// Method for retrieving bits from data
bool Itkpixv2DataProcessor::retrieve(uint64_t &variable, const unsigned length, const bool checkEOS, const bool skipNSCheck)
{
    // Should never happen: reading 0 bit
    if (unlikely(length == 0))
    {
        logger->warn("Retrieving 0 length from data stream");
        return true;
    }

    // Retrieving bits from the stream is highly non-trivial due to the varying length. There are several scenarios to cover. Using 64-bit instead of 32-bit data words can simplify the implementations
    // Check whether the bits to be retrieved are fully contained by the current 64-bit block
    if (_bitIdx + length <= BLOCKSIZE)
    {
        // Whether the bits to be retrieved are fully contained by the first half or second half of the 64-bit block (each corresponds to one 32-bit data word), or they span across the two 32-bit data words
        variable = (((_bitIdx + length) <= HALFBLOCKSIZE) || (_bitIdx >= HALFBLOCKSIZE)) ? ((_data[_bitIdx / HALFBLOCKSIZE] & (0xFFFFFFFFUL >> (_bitIdx - ((_bitIdx >> 5) << 5)))) >> ((((_bitIdx >> 5) + 1) << 5) - length - _bitIdx))
            : (((_data[0] & (0xFFFFFFFFUL >> _bitIdx)) << (length + _bitIdx - HALFBLOCKSIZE)) | (_data[1] >> (BLOCKSIZE - length - _bitIdx)));
        _bitIdx += length; // Move bit index
    }
    // If the bits overflow to the next 64-bit block
    else
    {
        // Retrieve all the remaining bits in the current 64-bit lock
        // Need different treatment depending on whether the bit index is in the first or second 32-bit data word of current 64-bit block
        variable = (((_bitIdx < HALFBLOCKSIZE) ? (((_data[0] & (0xFFFFFFFFUL >> _bitIdx)) << HALFBLOCKSIZE) | _data[1])
                    : (_data[1] & (0xFFFFFFFFUL >> (_bitIdx - HALFBLOCKSIZE)))) << (length + _bitIdx - BLOCKSIZE));

        // Check the ES bit of current block
        if ((_data[0] >> 31) & 0x1)
        {
            // If check end of stream is requested, roll back to previous block and return 0
            if (checkEOS)
            {
                // End of stream
                if (unlikely(variable != 0))
                    logger->error("The ES bit is 1 while the core column number read is non-zero ({} [{}]). Data processed so far are corrupted... Last block {:x}{:x}", variable, _bitIdx, _data[0], _data[1]);
                _bitIdx = 64;
                variable = 0;
                return true;
            }
            // Otherwise throw error message, unless over-draft is expected
            else if (!skipNSCheck)
            {
                logger->error("Expected unfinished stream while ES = 1: 0x{:x}{:x} [{} - {}]. Will start a new event... ({})", _data[0], _data[1], _bitIdx, length, _status);
                _status = INIT;
                return false;
            }
        }

        // Move to the next block
        // If the block index already reaches the end of the raw data container, stop here
        if (!getNextDataBlock())
        {
            getPreviousDataBlock();
            return false;
        }

        // Retrieve the bits overflow to the next 64-bit data block. Here we need to separate the cases where the overflow bits are fully contained in the first 32-bit data word, or extend to the second 32-bit data word
        // This check is needed because the maximum number of bits retrieved could be 64 bits (reading ToT for all 16 pixels from a full hit map)
        // Can be simplified if the data word is 64 instead of 32 bits, or if the ToT is read pixel-by-pixel (in which case the maximum number of bits retrieved will always be below 32 bits)
        variable |= (((_bitIdx + length) < (HALFBLOCKSIZE + BLOCKSIZE)) ? ((_data[0] & _streamMask) >> (95 - (_bitIdx + length) - _chipIdShift)) 
                : (((_data[0] & _streamMask) << (_bitIdx + length - 95-_chipIdShift)) | (_data[1] >> (127 - (length + _bitIdx) - _chipIdShift))));

        _bitIdx -= (63 - length - _chipIdShift); // Move bit index. Note the ES bit should also be counted
    }

    return true;
}

// Method for rolling back bit index
void Itkpixv2DataProcessor::rollBack(const unsigned length)
{
    // Should never happen: roll-back length = 0
    if (unlikely(length == 0))
        return;
    // After rolling back the index is still within the block. Keep in mind there is one extra bit from ES
    if (_bitIdx >= (length + 1 + _chipIdShift))
        _bitIdx -= length;
    // Rolling back to the previous block
    else
    {                              
        _bitIdx += (63 - length - _chipIdShift);  // Correct the bit index. Keep in mind there is ES bit
        getPreviousDataBlock();
    }
}

void Itkpixv2DataProcessor::process_core()
{
    if (m_input->empty())
        return;
    if (_status == INIT)
    {
        // Get data containers
        if (!getNextDataBlock())
            return;

        _tag = (_data[0] >> (23-_chipIdShift)) & 0xFF;
        _bitIdx = 9+_chipIdShift; // Reset bit index = ES + tag

        // Create a new event
        // TODO RD53B does not have L1 ID and BCID output in data stream, so these are dummy values for now
        _curOut->newEvent(_tag, _l1id, _bcid);
        //logger->info("New Stream, New Event: {} ", _tag);
        _events++;
    }

    // Start looping over data words in the current packet
    while (true)
    {
        switch (_status){
        case INIT:
        case CCOL:
            _status = CCOL;
            // Start from getting core column index
            // This is also the ONLY place where we check end-of-stream. In other places we simply assuming continuation of stream, and will throw an error message if the end of stream is somehow reached.
            if (!retrieve(_ccol, 6, true))
                return;
        case CCC:
            _status = CCC;
            // End of stream is marked with 0b000000. This is ensured in software in spite of the chip orphan bit configuration
            if (_ccol == 0) {
                // Check ES bit
                if (((_data[0] >> 31) & 0x1) != 0x1) {
                    logger->error("The ES bit is 0 while the core column number read is zero. Data processed so far are corrupted... Last block {:x}{:x}", _data[0], _data[1]);
                    // TODO: keep skipping data until ES = 1, and then skip one more
                }
                    
                // Get data containers
                if (!getNextDataBlock()) {
                    getPreviousDataBlock(); // Necesarry to check  for ES bit
                    return;
                }
                _tag = (_data[0] >> (23-_chipIdShift)) & 0xFF;
                _bitIdx = 9 + _chipIdShift; // Reset bit index = ES + tag

                // Create a new event
                // TODO RD53B does not have L1 ID and BCID output in data stream, so these are dummy values for now
                _curOut->newEvent(_tag, _l1id, _bcid);
                //logger->info("New Stream, New Event: {} ", _tag);
                _events++;
                _status = CCOL;
                continue;
            }
            else if (_ccol >= 0x38) // Internal tag
            {
                // Internal tag is 11-bit. So need to retrieve 5 more bits
                uint64_t temp = 0;
                if (!retrieve(temp, 5, true))
                    return;

                _tag = (_ccol << 5) | temp;

                // Create a new event
                // There is no L1ID and BCID in RD53B data stream. Currently put dummy values
                _curOut->newEvent(_tag, _l1id, _bcid);
                //logger->info("Same Stream, New Event: {} ", _tag);
                _events++;
                _status = CCOL;
                continue;
            }
        default:
            break;
        }

        // Loop over all the hits in the current event
        do
        {
            switch(_status){
            case CCC:
            case ILIN:
                _status = ILIN;
                // logger->warn("Read islast/isneighbor");
                // ###### Step 1. read islast, isneighbor, and qrow ######
                // Read islast, isneighbor
                // Note the qrow index will be absent if isneighbor = 1
                if (!retrieve(_islast_isneighbor, 2))
                    return;

            case QROW:
                _status = QROW;
                // logger->warn("Read qrow");
                // If isneighbor = 1, aggregate qrow
                if (_islast_isneighbor & 0x1)
                    ++_qrow[_ccol];

                // Otherwise read the qrow value
                else if (!retrieve(_qrow[_ccol], 8))
                    return;
            case HMAP1:
                _status = HMAP1;
                // logger->warn("Read hitmap 1");
                // ############ Step 2. read hit map ############
                // The hit map decoding is based on look-up table
                if (!retrieve(_hitmap, 16, false, true))
                    return;

            case HMAP2:
                // logger->warn("Read hitmap 2");
                _status = HMAP2;
                // Compressed hit map
                if (_isCompressedHitmap)
                {
                    // Since the hit map length is unpredictable in the compressed case, here we will retrieve 16-bit and feed it into the look-up table (LUT). 16-bit is enough to cover the hit-map in most cases. Using more or less bits are found to be less efficient
                    // The LUT entry is 32-bit and contains three parts
                    // 1. 8-bit indicating whether the hit map is fully contained in the 16-bit retrieved. If not, (16 - length of the hit map for the first row) is saved (maximum length of the hit map for the first row is 16)
                    // 2. 8-bit saving (16 - length of the hit map) if the hit map is fully contained in the 16 bits
                    // 3. 16-bit decompressed raw hit map, one bit for each pixel
                    const uint16_t hitmap_raw = _hitmap;
                    _hitmap = (_LUT_BinaryTreeHitMap[hitmap_raw] & 0xFFFF);
                    const uint8_t hitmap_rollBack = ((_LUT_BinaryTreeHitMap[hitmap_raw] & 0xFF000000) >> 24);

                    // If the hit map is not fully contained in 16 bits
                    if (hitmap_rollBack > 0)
                    {
                        // Remove the offset and read the hit map for the second row
                        if (hitmap_rollBack != 0xff)
                            rollBack(hitmap_rollBack);
                        // The length of the hit map for the second row can be 14 bits maximum
                        uint64_t rowMap = 0;
                        if (!retrieve(rowMap, 14, false, true))
                            return;

                        // This part is handled by another LUT focusing on decoding hit map for a single row (8 pixels)
                        // The entry of this LUT contains two parts
                        // 1. 8-bit saving (14 - length) of the hit map for the second row
                        // 2. 8-bit raw hit map for the row (1 bit for each pixel)
                        _hitmap |= ((_LUT_BinaryTreeRowHMap[rowMap] & 0xFF) << 8);
                        rollBack((_LUT_BinaryTreeRowHMap[rowMap] & 0xFF00) >> 8);
                    }
                    // Otherwise, remove the offset as preparation for reading ToT
                    else
                    {
                        rollBack((_LUT_BinaryTreeHitMap[hitmap_raw] & 0xFF0000) >> 16);
                    }
                }
            case TOT:
                // logger->warn("Read ToT");
                _status = TOT;
                // ############ Step 3. read ToT ############
                // Check whether it is precision ToT (PToT) data. PToT data is indicated by unphysical qrow index 196, and it should not be aggregated by the isnext bit
                if (_qrow[_ccol] == 196 && !(_islast_isneighbor & 0x1))
                {
                    if (!retrieve(_ToT, _LUT_PlainHMap_To_ColRow_ArrSize[_hitmap] << 2))
                        return;

                    int idx = 0;
                    for (unsigned ibus = 0; ibus < 4; ibus++)
                    {
                        uint8_t hitsub = (_hitmap >> (ibus << 2)) & 0xF;
                        if (hitsub)
                        {
                            uint16_t ptot_ptoa_buf = 0xFFFF;
                            // There is a bug in ITkPix-V1 (First version of ATLAS RD53B) that suppress "0b1111" in PToT data. Therefore we have to add these missing bits back by hand
                            for (unsigned iread = 0; iread < 4; iread++)
                            {
                                if ((hitsub >> iread) & 0x1)
                                {
                                    ptot_ptoa_buf &= ~((~(_ToT >> ((_LUT_PlainHMap_To_ColRow_ArrSize[_hitmap] - (++idx)) << 2)) & 0xF) << (iread << 2));
                                }
                            }

                            // PToT data for each pixel contain 16 bits. First 5 bits are for time of arrival, and 11 bits for time over threshold. Both are counted in 640 MHz clock
                            uint16_t PToT = ptot_ptoa_buf & 0x7FF;
                            uint16_t PToA = ptot_ptoa_buf >> 11;

                            if (_events == 0)
                            {
                                // This is now possible if the event is so long that it spreads over raw data containers
                                // logger->warn("[{}] No header in data fragment!", _channel);
                                _curOut->newEvent(_tag, _l1id, _bcid);
                                _events++;
                            }

                            // Reverse enginner the pixel address using mask staging
                            static unsigned maskLoopIndex = 0;
                            static bool check_loop_index = true;
                            if (check_loop_index)
                            {
                                for (unsigned loop = 0; loop < _curOut->lStat.size(); loop++)
                                {
                                    if (_curOut->lStat.getStyle(loop) == LOOP_STYLE_MASK)
                                    {
                                        maskLoopIndex = loop;
                                        check_loop_index = false;
                                        break;
                                    }
                                }
                            }
                            const unsigned step = _curOut->lStat.get(maskLoopIndex);
                            const uint16_t pix_col = (_ccol - 1) * 8 + PToT_maskStaging[step % 4][ibus] + 1;
                            const uint16_t pix_row = step / 2 + 1;

                            _curOut->curEvent->addHit({pix_col, pix_row, static_cast<uint16_t>(PToT | (PToA << 11))});
                        }
                    }
                }
                else
                {
                    // If drop ToT, the ToT value saved in the output event will be 0
                    // Otherwise we will translate the raw 16-bit hit map into number of hits and pixel addresses using yet another LUT
                    if (!_dropToT)
                    {
                        if (!retrieve(_ToT, _LUT_PlainHMap_To_ColRow_ArrSize[_hitmap] << 2))
                            return;
                    }
                    if (_LUT_PlainHMap_To_ColRow_ArrSize[_hitmap] == 0)
                    {
                        logger->warn("Received fragment with no ToT! ({} , {})", _ccol, _qrow[_ccol]);
                    }
                    for (unsigned ihit = 0; ihit < _LUT_PlainHMap_To_ColRow_ArrSize[_hitmap]; ++ihit)
                    {
                        const uint8_t pix_tot = (_ToT >> (ihit << 2)) & 0xF;
                        // First pixel is 1,1, last pixel is 400,384
                        const uint16_t pix_col = ((_ccol - 1) * 8) + (_LUT_PlainHMap_To_ColRow[_hitmap][ihit] >> 4) + 1;
                        const uint16_t pix_row = ((_qrow[_ccol])*2) + (_LUT_PlainHMap_To_ColRow[_hitmap][ihit] & 0xF) + 1;

                        // For now fill in _events without checking whether the addresses are valid
                        if (_events == 0)
                        {
                            // This is now possible if an event is so long that it spread over raw data containers
                            // logger->warn("[{}] No header in data fragment!", _channel);
                            _curOut->newEvent(_tag, _l1id, _bcid);
                            _events++;
                        }

                        _curOut->curEvent->addHit({pix_col, pix_row, pix_tot});
                        _hits++;
                    }
                }
            default:
                break;
            }
            _status = ILIN;
        } while (!(_islast_isneighbor & 0x2)); // Need to keep track of block index to make sure it is within the packet while we loop over the hits in the event
        _status = CCOL;
    }
}

bool Itkpixv2DataProcessor::getNextDataBlock()
{
    if (_curInV != nullptr && _curInV->size() > 0)
    {
        // Cross raw data container
        if (unlikely(_rawDataIdx < 0))
        {
            _rawDataIdx = 0;
            _wordIdx = 0;
            _data = &_curInV->data[0]->get(0);
            if (_data[0] == 0xFFFFDEAD && _data[1] == 0xFFFFDEAD)
                 return getNextDataBlock();
            if (((_data[0] >> 29) & 0x3) != _chipId && _enChipId)
                 return getNextDataBlock();
            return true;
        }
        _wordIdx += 2; // Increase block index
        if (_wordIdx >= _curInV->data[_rawDataIdx]->getSize())
        {
            _rawDataIdx++;
            _wordIdx = 0;
        }
    }

    // Cannot get more data: return failure code
    if (_curInV == nullptr || _curInV->size() == 0 || _rawDataIdx >= _curInV->size())
    {
        // Reset raw data index and word index
        _rawDataIdx = 0;
        _wordIdx = 0;

        // Keep track of last block
        if (_curInV != nullptr && _curInV->size() > 0)
        {
            _data_pre[0] = _data[0];
            _data_pre[1] = _data[1];

            // Push out data accumulated so far
            if (_events > 0)
            {
                // logger->error("Pushing out data {} events", _events[_activeChannels[i]]);
                _events = 0;
                m_out->pushData(std::move(_curOut));
            }
            else
            {
                // Maybe wait for end of method instead of deleting here?
                _curOut.reset();
            }
            //Cleanup
        }

        // Try to get data
        _curInV = m_input->popData();
        if (_curInV == nullptr || _curInV->size() == 0)
            return false;

        // Debug output
        /*
        for (unsigned i=0; i < _curInV->size(); i++) {
            for (unsigned j=0; j < _curInV->data[i]->getSize(); j+=2) {
                logger->info("[{}][{}] {} 0x{:08x}{:08x}", i, j, _curInV->data[i]->get(j) >> 31,  _curInV->data[i]->get(j), _curInV->data[i]->get(j+1));
            }
        }
        */

        _curOut.reset(new FrontEndData(_curInV->stat));
        _events = 0;

        // Increase word count
        for (unsigned c = 0; c < _curInV->size(); c++)
            _wordCount += _curInV->data[c]->getSize();
    }

    // Upate the data pointer. Note the meaning of block index is the first block that is *unprocessed*
    _data = &_curInV->data[_rawDataIdx]->get(_wordIdx);
    //logger->info("[{}] {} 0x{:x}{:x}", _wordIdx, _data[0]>>31, _data[0], _data[1]);

    // Return success code
    if (_data[0] == 0xFFFFDEAD && _data[1] == 0xFFFFDEAD)
         return getNextDataBlock();
    if (((_data[0] >> 29) & 0x3) != _chipId && _enChipId)
         return getNextDataBlock();
    return true;
}

void Itkpixv2DataProcessor::getPreviousDataBlock()
{
    // Correct raw data index and processed raw data size if needed
    _wordIdx -= 2;
    if (_wordIdx < 0)
    {
        // Special case that we need to go back to the previous raw data container
        if (--_rawDataIdx < 0)
        {
            _data = _data_pre;
            return;
        }
        _wordIdx = _curInV->data[_rawDataIdx]->getSize() - 2;
    }
    _data = &_curInV->data[_rawDataIdx]->get(_wordIdx); // Also roll back the block index and data word pointer

    if (_data[0] == 0xFFFFDEAD && _data[1] == 0xFFFFDEAD)
        getPreviousDataBlock();
    if (((_data[0] >> 29) & 0x3) != _chipId && _enChipId)
        getPreviousDataBlock();
}
