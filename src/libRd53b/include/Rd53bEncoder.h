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

class Rd53bEncoder : public ItkpixEncoder {
    
    public:
        
        void startStream();
        
        void addOrphans();
        
        void addToStream(const HitMap& hitMap, bool last = false);
};

#endif
