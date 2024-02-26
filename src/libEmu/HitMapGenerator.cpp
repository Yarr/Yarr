/*
* Author: Ondra Kovanda, ondrej.kovanda at cern.ch
* Date: 02/2024
* Description: Generation of hits for ITkPix* emulation
*/
#include "HitMapGenerator.h"

HitMapGenerator::HitMapGenerator(const uint nCol, const uint nRow, const uint nColInCCol, const uint nRowInQRow, const uint seed): m_nCol(nCol), m_nRow(nRow), m_nColInCCol(nColInCCol), m_nRowInQRow(nRowInQRow), m_seed(seed){
    //initialize the pdfs
    m_hitProb = std::uniform_real_distribution<float>(0., 1.);
    m_totProb = std::uniform_int_distribution<uint>(1, 15); 

    m_nCCol = nCol/m_nColInCCol;
    m_nQRow = nRow/m_nRowInQRow;

    generator.seed(m_seed);

}

void HitMapGenerator::setSeed(const uint seed){
    m_seed = seed;
    generator.seed(m_seed);
}

void HitMapGenerator::randomQCore(const uint CCol, const uint QRow){
    //These are the coordinates of the top left corner of the QCore
    const int m_col = CCol * m_nColInCCol;
    const int m_row = QRow * m_nRowInQRow;

    //Loop through pixels in that QCore, generate a random hit for each
    //and store the truth accordingly. Loop "backwards" to get the
    //truth hits in the same order as the default Yarr decoder
    for (int pixRow = m_row + m_nRowInQRow -1; pixRow >= m_row; pixRow--){
        for (int pixCol = m_col + m_nColInCCol -1; pixCol >= m_col; pixCol--){
            //does the pixel have a hit?
            float hitprob = m_hitProb(generator);
            //std::cout << hitprob << " vs occupancy " << m_occupancy << "\n";
            if (hitprob < m_occupancy){
                //Then give it a tot!
                m_hitMap[pixCol][pixRow] = m_totProb(generator);
                //and save it into the truth output, respecting the
                //decoder numbering conventions
                m_truthEvt.addHit(pixRow + 1, pixCol + 1, m_hitMap[pixCol][pixRow] - 1);
                std::cout << "Generating " << pixCol << " " << pixRow << " " << m_hitMap[pixCol][pixRow] << "\n";
            }
        }
    }
}


void HitMapGenerator::randomHitMap(float occupancy){
    //Generate uniformly random hit map for dev/testing purposes
    //initialize the hit map with all zeros
    m_hitMap   = std::vector<std::vector<uint16_t>>(m_nCol, std::vector<uint16_t>(m_nRow, 0));
    m_truthEvt = FrontEndEvent(0, 0, m_nGenerated);
    m_occupancy = occupancy;
    //Loop over the hit map and fill random tot values.
    //In order to preserve the order of hits as it emerges
    //from the decoder & ensure easy comparison, the generation
    //has to be per-QCore, and keep reverse order of the hits
    //compared to the encoding recipe in each QCore. This happens
    //in the randomQCore(...) function
    //m_hitMap[0][0] = 3;
    //m_truthEvt.addHit(1, 1, 2);
    //m_hitMap[1][0] = 3;
    //m_truthEvt.addHit(1, 2, 2);
    //m_hitMap[10][6] = 3;
    //m_truthEvt.addHit(7, 11, 2);
    //m_hitMap[15][67] = 3;
    //m_truthEvt.addHit(68, 16, 2);

    
    for (uint CCol = 0; CCol < m_nCCol; CCol++){
        for (uint QRow = 0; QRow < m_nQRow; QRow++){
            randomQCore(CCol, QRow);
            
        }
        
    }
    
    m_nGenerated++;
}