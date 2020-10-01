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
        FrontEndHit hit;
        handle.read((char*)&hit, sizeof(FrontEndHit));
        this->addHit(hit);
    } // ii
}

void FrontEndEvent::doClustering() {

    // No hits = no cluster
    if (nHits == 0)
        return ;
    // Create "copy" of hits
    std::vector<FrontEndHit*> unclustered;
    for (auto &&hit : hits) {
        unclustered.push_back(&hit);
    }
    
    // Create first cluster and add first hit
    clusters.emplace_back(FrontEndCluster());
    clusters.back().addHit(unclustered.front());
    unclustered.erase(unclustered.begin());

    int gap = 1;

    // Loop over vector of unclustered hits until empty
    while (!unclustered.empty()) {
        // Loop over hits in cluster, increases as we go
        for (unsigned ii=0; ii < clusters.back().nHits; ii++) {
            auto tHit = *clusters.back().hits.at(ii);
            // Loop over unclustered hits
            for (auto jj = unclustered.begin() ; jj != unclustered.end(); ++jj) {
                if ((abs((int)(tHit.col) - (int)(*jj)->col) <= (1+gap))
                 && (abs((int)(tHit.row) - (int)(*jj)->row) <= (1+gap))) {
                    // If not more than 1 pixel gap, add to cluster
                    clusters.back().addHit(*jj);
                    unclustered.erase(jj--);
                }
            }
        }
        // Still hits to be clustered, create new cluster
        if (!unclustered.empty()) {
            clusters.emplace_back(FrontEndCluster()); 
            clusters.back().addHit(unclustered.front());
            unclustered.erase(unclustered.begin());
        }
    }
    // All clustered
}

void FrontEndData::toFile(std::string filename) {
    std::fstream file(filename, std::fstream::out | std::fstream::app);
    file << events.size() << std::endl;
    for (auto event : events) {
        file << event.l1id << " " << event.bcid << " " << event.nHits << std::endl;
        for (auto hit : event.hits) {
            file << hit.col << " " << hit.row << " " << hit.tot << std::endl;
        }
    } // event
    file.close();
}
