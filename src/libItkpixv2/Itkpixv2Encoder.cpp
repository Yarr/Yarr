/*
* Author: Ondra Kovanda, ondrej.kovanda at cern.ch
* Date: 02/2024
* Description: ITkPix* encoding
*/

#include "Itkpixv2Encoder.h"

void Itkpixv2Encoder::endStream(){
    m_currBlock |= (0x1ULL << 63);
    pushWords32();
}

void Itkpixv2Encoder::addToStream(const HitMap& hitMap, bool last){
    //This is a high-level interface function that can take care of
    //adding an event into the current stream, this can be called
    //easily from the outside, and automatically tags/ends streams
    //and events based on internal vars only
    
    //If this is the first event, start a new stream. Otherwise, add an
    //internal tag
    if (m_currEvent == 0){
        streamTag(m_currStream);
        m_currStream++;
    }
    else {
        intTag(m_currEvent);
    }
    
    //Then add the actual encoded event information
    setHitMap(hitMap);
    encodeEvent();
    m_currEvent++;

    //If this is the last event in the stream or if we explicitly
    //want to end the stream (i. e. total number of generated events
    //is not a multiple of nEventsPerStream), end the stream
    if (m_currEvent == m_nEventsPerStream || last){
        endStream();
        m_currEvent = 0;
    }

}