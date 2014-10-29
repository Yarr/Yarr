#include "Fei4EventData.h"

#include <fstream>
#include <iostream>

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
