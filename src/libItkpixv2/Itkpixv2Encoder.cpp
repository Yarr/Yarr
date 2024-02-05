#include "Itkpixv2Encoder.h"
#include <bitset>
#include <string>

//Constructor sets up the geometry for all future loops

Itkpixv2Encoder::Itkpixv2Encoder(uint nCol, uint nRow, uint nColInCCol, uint nRowInQRow): m_nCol(nCol), m_nRow(nRow), m_nColInCCol(nColInCCol), m_nRowInQRow(nRowInQRow){
    m_nCCol = nCol/m_nColInCCol;
    m_nQRow = nRow/m_nRowInQRow;
}

void Itkpixv2Encoder::addBits64(uint64_t value, uint8_t length){
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
        uint32_t word1 = m_currBlock >> 32;
        uint32_t word2 = m_currBlock & 0xFFFFFFFF;
        m_words.push_back(word1);
        m_words.push_back(word2);
        m_currBlock = 0;
        m_currBit   = 0;

        //What are we left with?
        uint8_t leftoverBits = length - remainingBits;

        //Now we can add the remainder
        addBits64(value, leftoverBits);
        
    }
}

void Itkpixv2Encoder::encodeEvent(){
    //This produces the bits for one event. Loop over CCols, keep track of position within current block.
    

}

void Itkpixv2Encoder::test(){

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