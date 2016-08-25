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

void Fei4Event::doClustering() {
    if (nHits == 0)
        return ;
    
    // Create "copy" of hits
    std::vector<Fei4Hit*> unclustered;
    for (unsigned i=0; i<hits.size(); i++)
        unclustered.push_back(&hits[i]);

    // Create first cluster
    clusters.push_back(Fei4Cluster());
    clusters.back().addHit(unclustered[0]);
    unclustered.erase(unclustered.begin() + 0);

    // Loop over vector until empty
    while (!unclustered.empty()) {
        // Loop over hits in cluster
        for (unsigned i=0; i<clusters.back().nHits; i++) {
            Fei4Hit tHit = *clusters.back().hits[i];
            
            // Loop over unclustered hits
            for (unsigned j=0; j<unclustered.size(); j++) {
                if ((abs((int)tHit.col - unclustered[j]->col) <= 2)
                 && (abs((int)tHit.row - unclustered[j]->row) <= 2)) {
                    // If not more than 1 pixel gap, add to cluster
                    clusters.back().addHit(unclustered[j]);
                    unclustered.erase(unclustered.begin()+j);
                    j--;
                }
            }
        }
        // Still hits to be clustered, create new cluster
        if (!unclustered.empty()) {
            clusters.push_back(Fei4Cluster());
            clusters.back().addHit(unclustered[0]);
            unclustered.erase(unclustered.begin() + 0);
        }
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
