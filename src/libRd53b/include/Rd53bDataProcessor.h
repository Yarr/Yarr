#ifndef RD53BDATAPROCESSOR_H
#define RD53BDATAPROCESSOR_H

#include <vector>
#include <array>
#include <map>
#include <thread>

#include "DataProcessor.h"
#include "ClipBoard.h"
#include "RawData.h"
#include "EventData.h"
#include "Rd53b.h"

#define BINARYTREE_DEPTH 4
#define BLOCKSIZE 64
#define HALFBLOCKSIZE 32

class Rd53bDataProcessor : public DataProcessor
{
public:
    Rd53bDataProcessor();
    ~Rd53bDataProcessor() override;

    void connect(FrontEndCfg *feCfg, ClipBoard<RawDataContainer> *input, ClipBoard<EventDataBase> *out) override
    {
        m_feCfg = dynamic_cast<Rd53bCfg*>(feCfg);
        m_input = input;
        m_out = out;
    }
    void connect(ClipBoard<FeedbackProcessingInfo> *arg_proc_status) override {statusFb = arg_proc_status;}

    void init() override;
    void run() override;
    void join() override;
    void process() override;

    const uint32_t *_data; // Pointer to one data block
    int _wordIdx;          // Index of the word under processing
    unsigned _bitIdx;	   // Index of the first bit in datablock which is not processed yet. It starts from 0. The first half thus ends at 31, and the 2nd starts at 32
    int _rawDataIdx;       // Index of the raw data within each raw data container. Note it can be negative (means going back to previous container)

    std::unique_ptr<RawDataContainer> _curInV; // Current raw data container
    uint32_t _data_pre[2];                    // Last 64 bit of data from previous raw data container
    std::unique_ptr<FrontEndData> _curOut; // Output data container
    int _events;                           // Output number of events    

    void setCompressedHitmap(bool flag) { _isCompressedHitmap = flag; }
    void setDropToT(bool flag){_dropToT = flag;}

private:
    std::unique_ptr<std::thread> thread_ptr;
    ClipBoard<RawDataContainer> *m_input;
    ClipBoard<EventDataBase> *m_out;
    ClipBoard<FeedbackProcessingInfo> *statusFb = nullptr;
    Rd53bCfg *m_feCfg;

    unsigned _tag;
    unsigned _l1id;
    unsigned _bcid;
    unsigned _wordCount;
    unsigned _hits;

    bool _isCompressedHitmap; // Flag for toggle hitmap type, true for compressed, false for raw
    bool _dropToT;
    bool _enChipId;
    unsigned _chipIdShift;
    unsigned _chipId;
    unsigned long _streamMask;

    // Inline functions frequently used
    inline bool retrieve(uint64_t &variable, const unsigned length, const bool checkEOS = false, const bool skipNSCheck = false);	// Retrieve bit string with length
    inline void rollBack(const unsigned length);									// Roll back bit index
    inline uint8_t getBitPair(uint16_t &lowestLayer, uint8_t depth, uint8_t shift); // Get decoded bit pair used in hit map
    inline bool getNextDataBlock();
    inline void getPreviousDataBlock();
    inline void process_core();

    // Data stream components
    uint64_t _ccol;
    uint64_t _qrow[55]; // One counter for each core column. Use 54 as total number of core columns to be compatible with CMS chip geometry. Note core column index starts from 1.
    uint64_t _islast_isneighbor;
    uint64_t _hitmap;
    uint64_t _ToT;

    // Data processor status
    enum STATUS
    {
        INIT,  // Initial run
        CCOL,  // Reading core column
        CCC,   // Core column check
        ILIN,  // Reading islast/isneighbor bits
        QROW,  // Reading quarter row
        HMAP1, // Reading hit map step 1
        HMAP2, // Reading hit map step 2
        TOT    // Reading ToT
    };

    STATUS _status;
};
#endif
