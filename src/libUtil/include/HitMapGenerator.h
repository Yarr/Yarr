/*
* Author: Ondra Kovanda, ondrej.kovanda at cern.ch
* Date: 02/2024
* Description: Generation of hits for ITkPix* emulation
*/

#ifndef HITMAPGENERATOR_H
#define HITMAPGENERATOR_H

#include <vector>
#include <random>
#include <iostream>
#include "EventData.h"
#include "ItkpixLayout.h"


class HitMapGenerator{

    typedef ItkpixLayout<uint16_t> HitMap;


    public:
        HitMapGenerator(const uint nCol = 400, const uint nRow = 384, const uint nColInCCol = 8, const uint nRowInQrow = 2, const uint seed = 0);

        void setSeed(const uint seed = 0);

        void randomHitMap(const float occupancy = 1e-3);

        void randomQCore(const uint CCol, const uint QRow);

        HitMap& outHits(){return m_hitMap;}

        FrontEndEvent& outTruth(){return m_truthEvt;}

    private:
        //generation machinery
        std::mt19937 generator;
        std::uniform_real_distribution<float> m_hitProb;
        std::uniform_int_distribution<uint16_t>   m_totProb;


        //geometry and config
        uint m_seed, m_nCol, m_nRow, m_nCCol, m_nQRow, m_nColInCCol, m_nRowInQRow;
        float m_occupancy;

        //output
        HitMap m_hitMap;
        FrontEndEvent m_truthEvt;
        uint m_nGenerated;
};

#endif