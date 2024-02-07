#include "Itkpixv2Encoder.h"
#include <iostream>
#include <memory>
#include <bitset>

int main(){
    std::unique_ptr<Itkpixv2Encoder> encoder(new Itkpixv2Encoder());
    encoder->test();

    std::vector<uint32_t> words = encoder->getWords();

    for (auto& w : words){
        std::bitset<32> bw(w);
        //std::cout << bw.to_string() << "\n";
    }
}