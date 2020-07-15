#ifndef RD53BDATAPROCESSOR_H
#define RD53BDATAPROCESSOR_H

#include <vector>
#include <array>
#include <map>
#include <thread>

#include "DataProcessor.h"
#include "ClipBoard.h"
#include "RawData.h"
#include "Fei4EventData.h"
#include "Rd53b.h"

#define BINARYTREE_DEPTH 4
#define BLOCKSIZE 64
#define HALFBLOCKSIZE 32

class Rd53bDataProcessor : public DataProcessor
{
public:
	Rd53bDataProcessor();
	~Rd53bDataProcessor();

	void connect(ClipBoard<RawDataContainer> *input, std::map<unsigned, ClipBoard<EventDataBase>> *outMap) override final
	{
		m_input = input;
		m_outMap = outMap;
	}

	void init() override final;
	void run() override final;
	void join() override final;
	void process() override final;

	const uint32_t *_data; // Pointer to one data block
	unsigned _blockIdx;	   // Index of the data block
	unsigned _bitIdx;	   // Index of the first bit in datablock which is not processed yet. It starts from 0. The first half thus ends at 31, and the 2nd starts at 32
	std::unique_ptr<RawData> _curIn;
        void setCompressedHitmap(bool flag){_isCompressedHitmap = flag;}
private:
	std::vector<std::unique_ptr<std::thread>> thread_ptrs;
	ClipBoard<RawDataContainer> *m_input;
	std::map<unsigned, ClipBoard<EventDataBase>> *m_outMap;
	std::vector<unsigned> activeChannels;

	std::map<unsigned, unsigned> tag;
	std::map<unsigned, unsigned> l1id;
	std::map<unsigned, unsigned> bcid;
	std::map<unsigned, unsigned> wordCount;
	std::map<unsigned, int> hits;

        bool _isCompressedHitmap; // Flag for toggle hitmap type, true for compressed, false for raw
	// Inline functions frequently used
	inline uint64_t retrieve(const unsigned length, const bool checkEOS = false);	// Retrieve bit string with length
	inline void rollBack(const unsigned length);									// Roll back bit index
	inline uint8_t getBitPair(uint16_t &lowestLayer, uint8_t depth, uint8_t shift); // Get decoded bit pair used in hit map
	inline void process_core();
};
#endif
