#include "Itkpixv2Encoder.h"
#include "Itkpixv2QCoreEncodingLUT.h"
#include <bitset>
#include <string>

//Constructor sets up the geometry for all future loops

Itkpixv2Encoder::Itkpixv2Encoder(uint nCol, uint nRow, uint nColInCCol, uint nRowInQRow): m_nCol(nCol), m_nRow(nRow), m_nColInCCol(nColInCCol), m_nRowInQRow(nRowInQRow){
    m_nCCol = nCol/m_nColInCCol;
    m_nQRow = nRow/m_nRowInQRow;
}

void Itkpixv2Encoder::addBits64(const uint64_t value, const uint8_t length){
    //This adds 'length' lowest bits to the current block. If the current
    //block gets filled, push it to the output and start a new block with
    //the rest of the bits that didn't make it. Need to keep track of the
    //remaining space in the current word. Also, we only have 63 bits for
    //the added data, as the last bit is EoS.

    //Case 1: there's enough space for the entire information to be added
    if (length < (63 - m_currBit)){
        
        //The position at which the new bits should be inserted into the block
        //is (length of the block) - (currently last bit) - (length)
        m_currBlock |= (value << (64 - m_currBit - length));
        m_currBit += length;
        return;
    }

    //Case 2: there's not enough space, so we put what we can, push the block
    //to the output, and the rest to a new block
    else {

        //How much space do we have?
        uint8_t remainingBits = 63 - m_currBit;

        //Add that many bits, keeping the last one zero. We'll add the EoS
        //bit at the end of stream processing
        m_currBlock |= ((value >> (length - remainingBits - 1)) & (0xFFFFFFFFFFFFFFFF - 1));

        //Push the current block to the output and reset it and the current
        //bit counter. The block needs to be split in 32-bit halves before the output
        pushWords32();

        //What are we left with?
        uint8_t leftoverBits = length - remainingBits;

        //Now we can add the remainder
        addBits64(value, leftoverBits);
        
    }
}

void Itkpixv2Encoder::pushWords32(){
    //whenever the current block is ready for output,
    //split it into two 32-bit words and push them to
    //the output container. Reset the current bloc/bit
    uint32_t word1 = m_currBlock >> 32;
    uint32_t word2 = m_currBlock & 0xFFFFFFFF;
    m_words.push_back(word1);
    m_words.push_back(word2);
    m_currBlock = 0;
    m_currBit   = 0;
}

void Itkpixv2Encoder::encodeQCore(const uint nCCol, const uint nQRow){
    //produce hit map and ToTs
    //First, get the top-left pixel in the QCore
    uint m_col = nCCol * m_nColInCCol;
    uint m_row = nQRow * m_nRowInQRow;

    //now loop, store ToTs, and build index of the
    //compressed hit map in the LUT
    uint16_t lutIndex = 0x0000;
    std::vector<uint16_t> tots;
    tots.reserve(16);
    int pix = 0;
    for (uint pixRow = m_row; pixRow < m_row + m_nRowInQRow; pixRow++){
        for (uint pixCol = m_col; pixCol < m_col + m_nColInCCol; pixCol++){
            if (m_hitMap[pixCol][pixRow]){
                lutIndex |= 0x1 << pix;
                tots.push_back(m_hitMap[pixCol][pixRow] - 1);
            }
            pix++;
        }
    }

    //now add the binary-tree encoded & compressed map
    //from the LUT to the stream
    std::bitset<16> bsLUTIndex(lutIndex);
    std::cout << "The LUT index is " << bsLUTIndex.to_string() << "\n";
    addBits64(Itkpixv2Encoding::Itkpixv2QCoreEncodingLUT_Tree[lutIndex], Itkpixv2Encoding::Itkpixv2QCoreEncodingLUT_Length[lutIndex]);

    //and add the four-bit ToT information for each hit
    for (auto& tot : tots){
        std::bitset<4> bstot(tot);
        std::cout << "Adding ToT " << bstot << "\n";
        addBits64(tot, 4);
    }
}

bool Itkpixv2Encoder::hitInQCore(const uint CCol, const uint QRow){

    uint m_col = CCol * m_nColInCCol;
    uint m_row = QRow * m_nRowInQRow;

    for (uint pixRow = m_row; pixRow < m_row + m_nRowInQRow; pixRow++){
        for (uint pixCol = m_col; pixCol < m_col + m_nColInCCol; pixCol++){
            if (m_hitMap[pixCol][pixRow]) return true;
        }
    }

    return false;
}

void Itkpixv2Encoder::scanHitMap(){
    //Fill in a helper map of hit QCores and a vector of last qrow in each ccol
    m_hitQCores = std::vector<std::vector<bool>>(m_nCCol, std::vector<bool>(m_nQRow, false));
    m_lastQRow  = std::vector<uint>(m_nCCol, 0);

    for (uint CCol = 0; CCol < m_nCCol; CCol++){
        for (uint QRow = 0; QRow < m_nQRow; QRow++){
            //if there's a hit in the qcore, flag the helper map
            m_hitQCores[CCol][QRow] = hitInQCore(CCol, QRow);
            
            //and keep track of the last qrow in each CCol, so that we can
            //easily set the isLast bit
            if (m_hitQCores[CCol][QRow]) m_lastQRow[CCol]++;
        }
    }

}

void Itkpixv2Encoder::encodeEvent(){
    //This produces the bits for one event.
    //First, scan the map and produce helpers
    scanHitMap();

    for (uint CCol = 0; CCol < m_nCCol; CCol++){
        //if there are no hits in this CCol, continue
        if (m_lastQRow[CCol] == 0) continue;
        
        int previousQRow = -666;
        for (uint QRow = 0; QRow < m_nQRow; QRow++){
            //if there's no hit in this row, continue
            if (!m_hitQCores[CCol][QRow]) continue;

            //add the 6-bit (CCol + 1) address
            //debug
            std::bitset<6> bsCCol(CCol+1);
            std::cout << "Adding " << bsCCol.to_string() << " for CCol address\n";
            //debug
            addBits64(CCol + 1, 6);    
            
            //add the isLast bit
            //debug
            std::cout << "Adding " << (QRow == m_lastQRow[CCol] ? 1 : 0) << " for isLast\n";
            //debug

            QRow == m_lastQRow[CCol] ? addBits64(0x1, 1) : addBits64(0x0, 1);

            //add the isNeighbor bit. If false, add the QRow address as well.
            //debug
            std::cout << "Adding " << (QRow == previousQRow + 1 ? 1 : 0) << " for isNeighbor\n";
            //debug

            if (QRow == previousQRow + 1){
                addBits64(0x1, 1);
            }
            else {
                std::bitset<8> bsQRow(QRow+1);
                std::cout << "Adding " << bsQRow.to_string() << " for QRow " << QRow + 1 << "\n";
                addBits64(0x0, 1);
                addBits64(QRow + 1, 8);
            };

            //add the map and ToT
            encodeQCore(CCol, QRow);

            //update the previous QRow
            previousQRow = QRow;
        }
    }    

}

void Itkpixv2Encoder::intTag(const uint16_t nEvt){
    //this adds 11 bits of interal tagging between events.
    //does the tag always need to start with 111?
    uint16_t tag = nEvt | (0xf << 8);
    addBits64(tag, 11);
}

void Itkpixv2Encoder::endStream(){
    m_currBlock |= 0x1;
    pushWords32();
}

void Itkpixv2Encoder::test(){

    int nTot = 1;
    int nEventsPerStream = 1;

    for (int i = 0; i < nTot; i++){
        //randomHitMap(0.01, i);
        m_hitMap = std::vector<std::vector<uint16_t>>(m_nCol, std::vector<uint16_t>(m_nRow, 0));
        m_hitMap[0][0] = 5;
        encodeEvent();
        if (nEventsPerStream != 1) intTag(i);
        if ((nTot == 1) || i != 0 && (i % nEventsPerStream == 0)) endStream();
    }
    
    /*
    m_hitMap = std::vector<std::vector<uint16_t>>(m_nCol, std::vector<uint16_t>(m_nRow, 0));
    m_hitMap[2][0] = 5;
    m_hitMap[7][1] = 9;
    
    m_hitMap[5][2] = 3;
    m_hitMap[10][50] = 5;
    m_hitMap[37][71] = 9;
    
    m_hitMap[65][92] = 3;
    m_hitMap[28][0] = 5;
    m_hitMap[74][15] = 9;
    
    m_hitMap[56][22] = 3;
    */
    

    std::cout << "First QCore is:\n";

    uint m_col = 0;
    uint m_row = 0;

    for (uint pixRow = m_row; pixRow < m_row + m_nRowInQRow; pixRow++){
        for (uint pixCol = m_col; pixCol < m_col + m_nColInCCol; pixCol++){
            std::cout << m_hitMap[pixCol][pixRow];
        }
        std::cout << "\n";
    }

    std::cout << "First few words are:\n";
    std::cout << m_words.size() << "\n";
    for (uint w = 0; w < 5; w++){
        std::bitset<32> bw(m_words[w]);
        std::cout << bw.to_string() << "\n";
    }

    std::cout << "Last few words are:\n";
    std::cout << m_words.size() << "\n";
    for (uint w = m_words.size() - 1; w > m_words.size() - 5; w--){
        std::bitset<32> bw(m_words[w]);
        std::cout << bw.to_string() << "\n";
    }

/*
    for (uint i = 0; i < 10; i++){
        std::string s = "1010101010";
        std::bitset<10> bs(s);
        uint64_t toAdd = bs.to_ullong();
        std::bitset<64> bsAdd(toAdd);
        std::cout << "Adding\n";
        std::cout << bsAdd.to_string() << "\n";
        std::cout << "to\n";
        std::bitset<64> bsCurr(m_currBlock);
        std::cout << bsCurr.to_string() << "\n";
        addBits64(toAdd, 10);
        std::cout << "result\n";
        std::bitset<64> bsOut(m_currBlock);
        std::cout << bsOut.to_string() << "\n";
        std::cout << "Output size " << m_words.size() << "\n";
    }
*/
}



void Itkpixv2Encoder::randomHitMap(float occupancy, int seed){
    //Generate random hit map for dev/testing purposes. Otherwise the actual generation
    //will happen in another tool/class.

    //Set the generator seed, create the neccessary pdfs

    generator.seed(seed);
    std::uniform_real_distribution<float> hitProb(0., 1.);
    std::uniform_int_distribution<uint> totProb(1, 15);


    //initialize the hit map with all zeros

    m_hitMap = std::vector<std::vector<uint16_t>>(m_nCol, std::vector<uint16_t>(m_nRow, 0));

    //Loop over the hit map and fill random tot values

    for (uint col = 0; col < m_nCol; col++){
        for (uint row = 0; row < m_nRow; row++){

            //Did the pixel get a hit?
            if (hitProb(generator) < occupancy){
                //Then give it a tot!
                m_hitMap[col][row] = totProb(generator);
            }
        }
    }
}