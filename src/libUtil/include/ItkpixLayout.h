/*
* Author: Ondra Kovanda, ondrej.kovanda at cern.ch
* Date: 04/2024
* Description: ITkPix* chip layout template. This aims for a contiguously stored
*              array of arbitrary type, representing the individual pixels in
*              ITkPix* chips with a 'physically' motivated (col, row) access
*/

#ifndef ITKPIXLAYOUT_H
#define ITKPIXLAYOUT_H

#include <array>
#include <cstdint>

template<class T> class ItkpixLayout{

    public:

        ItkpixLayout(){};

        T& operator()(const uint16_t col, const uint16_t row){

            //Columns are stored after one another
            return pixels[ col * 384 + row ];

        }

        T operator()(const uint16_t col, const uint16_t row) const {

            //Columns are stored after one another
            return pixels[ col * 384 + row ];

        }

        T& operator()(const uint32_t pos){
            
            //In case we already have the position at hand
            return pixels[pos];
        
        }

        T operator()(const uint32_t pos) const {
            
            //In case we already have the position at hand
            return pixels[pos];
        
        }

        void reset(){

            pixels = {};

        }

    private:

        //All chips will allways have 400*384 pixels
        std::array<T, 153600> pixels = {};

};

#endif