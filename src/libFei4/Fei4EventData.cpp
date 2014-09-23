#include "Fei4EventData.h"

#include <fstream>

void Fei4Data::toFile(std::string filename) {
    std::fstream file(filename, std::fstream::out | std::fstream::trunc);
    
    file << events.size() << std::endl;
    for (unsigned i=0; i<events.size(); i++) {
        file << events[i].l1id << " " << events[i].hits.size() << std::endl;
        for (unsigned j=0; j<events[i].hits.size(); j++) {
            file << events[i].hits[j].bcid << " " << events[i].hits[j].col << " " << events[i].hits[j].row << " " << events[i].hits[j].tot << std::endl;
        }
    }
}
