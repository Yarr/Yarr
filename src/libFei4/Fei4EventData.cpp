#include "Fei4EventData.h"

#include <fstream>
#include <iostream>
#include <list>

void Fei4Event::toFileBinary(std::fstream &handle) {
    handle.write((char*)&tag, sizeof(uint32_t));
    handle.write((char*)&l1id, sizeof(uint16_t));
    handle.write((char*)&bcid, sizeof(uint16_t));
    handle.write((char*)&nHits, sizeof(uint16_t));
    for (auto hit : hits) {
        handle.write((char*)&hit, sizeof(Fei4Hit));
    }
}

void Fei4Event::fromFileBinary(std::fstream &handle) {
    uint16_t t_hits = 0;
    handle.read((char*)&tag, sizeof(uint32_t));
    handle.read((char*)&l1id, sizeof(uint16_t));
    handle.read((char*)&bcid, sizeof(uint16_t));
    handle.read((char*)&t_hits, sizeof(uint16_t));
    for (unsigned i=0; i<t_hits; i++) {
        struct Fei4Hit hit;
        handle.read((char*)&hit, sizeof(Fei4Hit));
        this->addHit(hit.row, hit.col, hit.tot);
    }
}

void Fei4Event::doClustering() {
    // No hits = no cluster
    if (nHits == 0)
        return ;
    // Create "copy" of hits
    std::list<Fei4Hit*> unclustered;
    for (auto &&hit : hits) {
        unclustered.push_back(&hit);
    }
    
    // Create first cluster and add first hit
    clusters.push_back(Fei4Cluster());
    clusters.back().addHit(unclustered.front());
    unclustered.erase(unclustered.begin());

    int gap = 1;

    // Loop over vector of unclustered hits until empty
    while (!unclustered.empty()) {
        // Loop over hits in cluster, increases as we go
        for (unsigned i=0; i<clusters.back().nHits; i++) {
            Fei4Hit tHit = *clusters.back().hits[i];
            // Loop over unclustered hits
            for (auto j = unclustered.begin() ; j != unclustered.end(); ++j) {
                if ((abs((int)(tHit.col) - (int)(*j)->col) <= (1+gap))
                 && (abs((int)(tHit.row) - (int)(*j)->row) <= (1+gap))) {
                    // If not more than 1 pixel gap, add to cluster
                    clusters.back().addHit(*j);
                    unclustered.erase(j--);
                }
            }
        }
        // Still hits to be clustered, create new cluster
        if (!unclustered.empty()) {
            clusters.push_back(Fei4Cluster());
            clusters.back().addHit(unclustered.front());
            unclustered.erase(unclustered.begin());
        }
    }
    // All clustered
        
    
}

void Fei4Data::toFile(std::string filename) {
    //std::cout << __PRETTY_FUNCTION__ << " " << filename << std::endl;
    std::fstream file(filename, std::fstream::out | std::fstream::app);
    
    file << events.size() << std::endl;
    for (std::list<Fei4Event>::iterator eit = events.begin(); eit != events.end(); ++eit) {
        file << (*eit).l1id << " " << (*eit).bcid << " " << (*eit).nHits << std::endl;
        for (std::list<Fei4Hit>::iterator it = (*eit).hits.begin(); it != (*eit).hits.end(); ++it) {
            file << (*it).col << " " << (*it).row << " " << (*it).tot << std::endl;
        }
    }
    file.close();
}
