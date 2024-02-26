/*
* Author: Ondra Kovanda, ondrej.kovanda at cern.ch
* Date: 02/2024
* Description: ITkPix* encoding
*/

#ifndef ITKPIXV2ENCODER_H
#define ITHPIXV2ENCODER_H

#include <vector>
#include <iostream>
#include <random>


class Itkpixv2Encoder{
    typedef std::vector<std::vector<uint16_t>> HitMap;
    public:
        Itkpixv2Encoder(const uint nCol = 400, const uint nRow = 384, const uint nColInCCol = 8, const uint nRowInQRow = 2, const uint nEventsPerStream = 16, const bool plainHitMap = false, const bool dropToT = false);
        
        std::vector<uint32_t>& getWords(){return m_words;}
        
        void addBits64(const uint64_t value, const uint8_t length);

        void pushWords32();

        void encodeQCore(const uint nCCol, const uint nQRow);
        
        void encodeEvent();

        void streamTag(const uint8_t nStream);

        void intTag(const uint16_t nEvt);

        void endStream();

        void scanHitMap();

        bool hitInQCore(const uint CCol, const uint QRow);

        void setHitMap(const HitMap& hitMap){m_hitMap = hitMap;}

        void setEventsPerStream(const uint nEventsPerStream = 16){m_nEventsPerStream = nEventsPerStream;}

        void addToStream(const HitMap& hitMap);

    
    private:
        // Output
        std::vector<uint32_t> m_words;
        uint m_nEventsPerStream, m_currCCol, m_currQRow, m_currEvent;//, m_lastQRow;
        uint8_t m_currStream;

        // Encoding machinery
        uint64_t m_currBlock;
        uint8_t  m_currBit;
        std::vector<std::vector<bool>> m_hitQCores;
        std::vector<uint> m_lastQRow;

        // Chip geometry
        uint m_nCol, m_nRow, m_nCCol, m_nQRow, m_nColInCCol, m_nRowInQRow;

        //Globals - could be replace with compile-time conditioning instead of run-time if performance is critical
        bool m_plainHitMap, m_dropToT;

        // Input
        std::vector<std::vector<uint16_t>> m_hitMap;


};


#endif