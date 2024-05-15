/*
* Author: Ondra Kovanda, ondrej.kovanda at cern.ch
* Date: 05/2024
* Description: Controller for ITkPixV2 emulator
*/

#ifndef ITKPIXV2EMU_H
#define ITKPIXV2EMU_H

#include "ItkpixLayout.h"

class Itkpixv2Emu {
    public:
        
        Itkpixv2Emu();

        void executeLoop();

        void outputLoop();

    private:
        ItkpixLayout<uint16_t> ToT_map; //just a dummy
};

#endif ITKPIXV2EMU_H