#include "Fei4EventData.h"

#include <fstream>
#include <iostream>

void Fei4Event::toFileBinary(std::fstream &handle) {
    handle.write((char*)&l1id, sizeof(uint16_t));
    handle.write((char*)&bcid, sizeof(uint16_t));
    handle.write((char*)&nHits, sizeof(uint16_t));
    for (unsigned i=0; i<nHits; i++) {
        handle.write((char*)&hits[i], sizeof(Fei4Hit));
    }
}

void Fei4Event::fromFileBinary(std::fstream &handle) {
    uint16_t t_hits = 0;
    handle.read((char*)&l1id, sizeof(uint16_t));
    handle.read((char*)&bcid, sizeof(uint16_t));
    handle.read((char*)&t_hits, sizeof(uint16_t));
    for (unsigned i=0; i<t_hits; i++) {
        struct Fei4Hit hit;
        handle.read((char*)&hit, sizeof(Fei4Hit));
        this->addHit(hit.row, hit.col, hit.tot);
    }
}
void Fei4Data::toFile(std::string filename) {
    //std::cout << __PRETTY_FUNCTION__ << " " << filename << std::endl;
    std::fstream file(filename, std::fstream::out | std::fstream::app);
    
    file << events.size() << std::endl;
    for (std::list<Fei4Event>::iterator eit = events.begin(); eit != events.end(); ++eit) {
        file << (*eit).l1id << " " << (*eit).bcid << " " << (*eit).nHits << std::endl;
        for (std::vector<Fei4Hit>::iterator it = (*eit).hits.begin(); it != (*eit).hits.end(); ++it) {
            file << (*it).col << " " << (*it).row << " " << (*it).tot << std::endl;
        }
    }
    file.close();
}
