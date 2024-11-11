/*
* Author: Ondra Kovanda, ondrej.kovanda at cern.ch
* Date: 02/2024
* Description: RD53b encoding
*/

#include "Rd53bEncoder.h"

void Rd53bEncoder::startStream(){
    //RD53B functionality, can be replaced by separate by separate
    //instance of low-level class
    m_currBlock |= (0x1ULL << 63);
}

void Rd53bEncoder::addOrphans(){
    //where is currently the bit counter in the current block?
    if (63 - m_currBit >= 6){
        //There are enough orphans in the current word to end the stream
        pushWords32();
    }
    else {
        //There's not enough orphans in the current word, so we need
        //to add a whole new word full of 0s
        pushWords32();
        pushWords32();
    }
}

void Rd53bEncoder::addToStream(const HitMap& hitMap, bool last){
    //This is a high-level interface function that can take care of
    //adding an event into the current stream, this can be called
    //easily from the outside, and automatically tags/ends streams
    //and events based on internal vars only
    
    //If this is the first event, start a new stream. Otherwise, add an
    //internal tag
    if (m_currEvent == 0){
        startStream();
        streamTag(m_currStream);
        m_currStream++;
    }
    else {
        intTag(m_currEvent);
    }
    
    //Then add the actual encoded event information
    // setHitMap(hitMap);
    encodeEvent(hitMap);
    m_currEvent++;

    //If this is the last event in the stream or if we explicitly
    //want to end the stream (i. e. total number of generated events
    //is not a multiple of nEventsPerStream), end the stream
    if (m_currEvent == m_nEventsPerStream || last){
        addOrphans();
        m_currEvent = 0;
    }

}