#ifndef YARR_CLUSTER_DATA_H
#define YARR_CLUSTER_DATA_H

#include "EventData.h"

class FrontEndCluster {
    public :
        FrontEndCluster() { nHits = 0; }
        ~FrontEndCluster() = default;

        void addHit(const FrontEndHit* hit) {
            hits.push_back(hit);
            nHits++;
        }
        unsigned getColLength() {
            int min = 99999;
            int max = -1;
            for (unsigned ii = 0; ii < hits.size(); ii++) {
                if ((int)hits[ii]->col > max)
                    max = hits[ii]->col;
                if ((int)hits[ii]->col < min)
                    min = hits[ii]->col;
            } // ii
            return (max-min+1);
        }

        unsigned getRowWidth() {
            int min = 99999;
            int max = -1;
            for (unsigned ii = 0; ii < hits.size(); ii++) {
                if ((int)hits[ii]->row > max)
                    max = hits[ii]->row;
                if ((int)hits[ii]->row < min)
                    min = hits[ii]->row;
            } // ii
            return (max-min+1);
        }

        unsigned nHits;
        std::vector<const FrontEndHit*> hits;

}; // class FrontEndCluster

std::vector<FrontEndCluster> doClustering(const FrontEndEvent &ev);

#endif
