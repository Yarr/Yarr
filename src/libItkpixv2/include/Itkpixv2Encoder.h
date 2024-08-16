/*
* Author: Ondra Kovanda, ondrej.kovanda at cern.ch
* Date: 02/2024
* Description: ITkPixv2 encoding
*/

#ifndef ITKPIXV2ENCODER_H
#define ITKPIXV2ENCODER_H

#include <vector>
#include <iostream>
#include <random>
#include "ItkpixEncoder.h"

class Itkpixv2Encoder : public ItkpixEncoder {
    
    public:
        
        void endStream();
        
        void addToStream(const HitMap& hitMap, bool last = false);

        void addToStream(const HitMap& hitMap, const uint8_t tag);
};

#endif
