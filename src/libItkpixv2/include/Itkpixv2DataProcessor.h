#ifndef ITKPIXV2DATAPROCESSOR_H
#define ITKPIXV2DATAPROCESSOR_H

#include <vector>
#include <array>
#include <map>
#include <thread>

#include "FeDataProcessor.h"
#include "ClipBoard.h"
#include "RawData.h"
#include "EventData.h"
#include "Itkpixv2.h"

#define BINARYTREE_DEPTH 4
#define BLOCKSIZE 64
#define HALFBLOCKSIZE 32

#ifndef USE_ITKPIX_DEBUG_BUFFER 
#   define USE_ITKPIX_DEBUG_BUFFER 0
#endif

#ifndef ITKPIX_DEBUG_BUFFERSIZE
#   define ITKPIX_DEBUG_BUFFERSIZE 30
#endif

class Itkpixv2DataProcessor : public FeDataProcessor
{
public:
    Itkpixv2DataProcessor();
    ~Itkpixv2DataProcessor() override;

    void connect(FrontEndCfg *feCfg, ClipBoard<RawDataContainer> *input, ClipBoard<EventDataBase> *out) override
    {
        m_feCfg = dynamic_cast<Itkpixv2Cfg*>(feCfg);
        m_input = input;
        m_out = out;
    }
    void connect(ClipBoard<FeedbackProcessingInfo> *arg_proc_status) override {statusFb = arg_proc_status;}

    void init() override;
    void run() override;
    void join() override;
    void process() override;
    json getLog() override;

    uint32_t *_data; // Pointer to one data block
    RawDataPtr _dataPtrCpy; // Copy of shared pointer to data object _data points to
    uint32_t *_data_t; // Internal state var
    RawDataPtr _dataPtrCpy_t; // Copy of shared pointer to data object _data_t points to

    int _wordIdx;          // Index of the word under processing
    unsigned _bitIdx;	   // Index of the first bit in datablock which is not processed yet. It starts from 0. The first half thus ends at 31, and the 2nd starts at 32
    int _rawDataIdx;       // Index of the raw data within each raw data container. Note it can be negative (means going back to previous container)

    std::unique_ptr<RawDataContainer> _curInV; // Current raw data container
    uint32_t _data_pre[2];                    // Last 64 bit of data from previous raw data container
    std::unique_ptr<FrontEndData> _curOut; // Output data container
    int _events;                           // Output number of events    

    unsigned _chipTagBitFlipCnt; // Number of tags with value 216-219 (chip tag bit flip detected)
    unsigned _chipTagErrorCnt; // Number of tags with value 220-223 (chip tag unreadable)
    unsigned _unfinishedStreamErrorCnt; // Number of "expect unfinished stream while ES=1" (without check EOS on)
    unsigned _unfinishedStreamEOSErrorCnt; // Number of "expect unfinished stream while ES=1" (with check EOS on)
    unsigned _corruptStreamErrorCnt; // Number of ES=0, but CCOL=0 instances (implies corrupted stream)
    unsigned _splitEventsCnt; // Number of times we add a hit to a new event with the same tag as a previous event

    void setCompressedHitmap(bool flag) { _isCompressedHitmap = flag; }
    void setDropToT(bool flag){_dropToT = flag;}

private:
    std::unique_ptr<std::thread> thread_ptr;
    ClipBoard<RawDataContainer> *m_input;
    ClipBoard<EventDataBase> *m_out;
    ClipBoard<FeedbackProcessingInfo> *statusFb = nullptr;
    Itkpixv2Cfg *m_feCfg;

    uint16_t _tag;
    uint16_t _l1id;
    uint16_t _bcid;
    unsigned long _wordCount;
    unsigned long _hits;

    bool _isCompressedHitmap; // Flag for toggle hitmap type, true for compressed, false for raw
    bool _dropToT;
    bool _enChipId;
    bool _enBcid; // Flag for BCID read enable
    bool _enL1id; // Flag for Level-1 read enable
    bool _readBcL1; // OR 
    unsigned _chipIdShift;
    unsigned _chipId;
    unsigned long _streamMask;

    std::vector<uint32_t> _debugBuffer;
    unsigned _debugIdx; // position in debug buffer

    // Inline functions frequently used
    inline bool retrieve(uint64_t &variable, const unsigned length, const bool checkEOS = false, const bool skipNSCheck = false);	// Retrieve bit string with length
    inline void rollBack(const unsigned length);									// Roll back bit index
    inline uint8_t getBitPair(uint16_t &lowestLayer, uint8_t depth, uint8_t shift); // Get decoded bit pair used in hit map
    inline bool getNextDataBlock();
    inline bool getNextDataBlockImpl();
    inline void getPreviousDataBlock();
    inline void process_core();
    inline void sendFeedback(unsigned tag, unsigned bcid);
    void dumpDebugBuffer();

    // Data stream components
    uint64_t _ccol;
    uint64_t _qrow[55]; // One counter for each core column. Use 54 as total number of core columns to be compatible with CMS chip geometry. Note core column index starts from 1.
    uint64_t _islast_isneighbor;
    uint64_t _hitmap;
    uint64_t _ToT;

    // Data processor status
    enum STATUS
    {
        INIT=0,  // Initial run
        BCIDL1=1, // Reading BCID and/or L1ID
        CCOL=2,  // Reading core column
        CCC=3,   // Core column check
        ILIN=4,  // Reading islast/isneighbor bits
        QROW=5,  // Reading quarter row
        HMAP1=6, // Reading hit map step 1
        HMAP2=7, // Reading hit map step 2
        TOT=8    // Reading ToT
    };

    STATUS _status;
};
#endif
