/*
* Author: Ondra Kovanda, ondrej.kovanda at cern.ch
* Date: 04/2024
* Description: ITkPix* chip layout template. This aims for a contiguously stored
*              array of arbitrary type, representing the individual pixels in
*              ITkPix* chips with a 'physically' motivated (col, row) access
*/

#include <array>
#include <cstdint>

template<class T> class ItkpixLayout{

    public:

        ItkpixLayout();

        T& ItkpixLayout::operator()(const uint16_t col, const uint16_t row){

            //Columns are stored after one another
            return pixels[ col * 384 + row ];

        }

        T ItkpixLayout::operator()(const uint16_t col, const uint16_t row) const {

            //Columns are stored after one another
            return pixels[ col * 384 + row ];

        }

    private:

        //All chips will allways have 400*384 pixels
        std::array<T, 153600> pixels;

};