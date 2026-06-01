#include "EventData.h"

// std/stl
#include <iostream>
#include <fstream>

void FrontEndEvent::toFileBinary(std::fstream &handle) const {
    handle.write((char*)&tag, sizeof(uint32_t));
    handle.write((char*)&l1id, sizeof(uint16_t));
    handle.write((char*)&bcid, sizeof(uint16_t));
    handle.write((char*)&nHits, sizeof(uint16_t));
    for (auto hit : hits) {
        handle.write((char*)&hit, sizeof(FrontEndHit));
    } // h
}

void FrontEndEvent::fromFileBinary(std::fstream &handle) {
    uint16_t t_hits = 0;
    handle.read((char*)&tag, sizeof(uint32_t));
    handle.read((char*)&l1id, sizeof(uint16_t));
    handle.read((char*)&bcid, sizeof(uint16_t));
    handle.read((char*)&t_hits, sizeof(uint16_t));
    for (unsigned ii = 0; ii < t_hits; ii++) {
        FrontEndHit hit = {};
        handle.read((char*)&hit, sizeof(FrontEndHit));
        this->addHit(hit);
    } // ii
}

void FrontEndData::toFile(std::string filename) {
    std::fstream file(filename, std::fstream::out | std::fstream::app);
    file << events.size() << std::endl;
    for (auto &event : events) {
        file << event.l1id << " " << event.bcid << " " << event.nHits << std::endl;
        for (auto &hit : event.hits) {
            file << hit.col << " " << hit.row << " " << hit.tot << std::endl;
        }
    } // event
    file.close();
}
