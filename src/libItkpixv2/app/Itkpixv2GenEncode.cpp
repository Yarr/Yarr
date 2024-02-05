#include "Itkpixv2Encoder.h"
#include <iostream>
#include <memory>

int main(){
    std::unique_ptr<Itkpixv2Encoder> encoder(new Itkpixv2Encoder());
    encoder->test();
}