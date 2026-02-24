/*
* Author: Ondra Kovanda, ondrej.kovanda at cern.ch
* Date: 02/2024
* Description: Generation of hits for ITkPix* emulation
*/
#include "HitMapGenerator.h"

HitMapGenerator::HitMapGenerator(const uint nCol, const uint nRow, const uint nColInCCol, const uint nRowInQRow, const uint seed)
  : m_seed(seed),
    m_nColInCCol(nColInCCol), m_nRowInQRow(nRowInQRow)
{
    //initialize the pdfs for assigning a hit and for
    //the ToT value of that hit
    m_hitProb = std::uniform_real_distribution<float>(0., 1.);
    m_totProb = std::uniform_int_distribution<uint16_t>(1, 15); 

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
                uint16_t tot = m_totProb(generator);
                m_hitMap(pixCol, pixRow) = tot;//m_totProb(generator);
                
                //and save it into the truth output, respecting the
                //decoder numbering conventions
                m_truthEvt.addHit(pixRow + 1, pixCol + 1, m_hitMap(pixCol, pixRow) - 1);
            }
        }
    }
}


void HitMapGenerator::randomHitMap(float occupancy){
    //Generate uniformly random hit map for dev/testing purposes
    //initialize the hit map with all zeros
    m_hitMap   = ItkpixLayout<uint16_t>();
    m_truthEvt = FrontEndEvent(0, 0, m_nGenerated);
    m_occupancy = occupancy;
    
    //Loop over the hit map and fill random tot values.
    //In order to preserve the order of hits as it emerges
    //from the decoder & ensure easy comparison, the generation
    //has to be per-QCore, and keep reverse order of the hits
    //compared to the encoding recipe in each QCore. This happens
    //in the randomQCore(...) function
    for (uint CCol = 0; CCol < m_nCCol; CCol++){
        for (uint QRow = 0; QRow < m_nQRow; QRow++){
            randomQCore(CCol, QRow);  
        }
        
    }
    
    m_nGenerated++;
}