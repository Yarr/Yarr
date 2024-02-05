#ifndef ITKPIXV2ENCODER_H
#define ITHPIXV2ENCODER_H

#include <vector>
#include <iostream>
#include <random>

class Itkpixv2Encoder{
    public:
        Itkpixv2Encoder(uint nCol = 400, uint nRow = 384, uint nColInCCol = 8, uint nRowInQRow = 2);
        std::vector<uint32_t> getWords();
        void randomHitMap(float occupancy = 1e-3, int seed = 0);
        void addBits64(uint64_t value, uint8_t length);
        
        void encodeEvent();

        void test();

    
    private:
        // Output
        std::vector<uint32_t> m_words;
        uint m_nEventsPerStream, m_currCCol, m_currQRow, m_lastQRow;

        // Encoding machinery
        uint64_t m_currBlock;
        uint8_t m_currBit;

        // Chip geometry
        uint m_nCol, m_nRow, m_nCCol, m_nQRow, m_nColInCCol, m_nRowInQRow;

        // Testing
        std::vector<std::vector<uint16_t>> m_hitMap;
        std::mt19937 generator;


};


#endif