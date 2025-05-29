#include "ClusterData.h"

std::vector<FrontEndCluster> doClustering(const FrontEndEvent &ev)
{
    // No hits = no cluster
    if (ev.nHits == 0)
      return {};
    // Create "copy" of hits
    std::vector<const FrontEndHit*> unclustered;
    for (auto &&hit : ev.hits) {
        unclustered.push_back(&hit);
    }

    // Create first cluster and add first hit
    std::vector<FrontEndCluster> clusters;

    clusters.emplace_back(FrontEndCluster());
    clusters.back().addHit(unclustered.front());
    unclustered.erase(unclustered.begin());

    int gap = 1;

    // Loop over vector of unclustered hits until empty
    while (!unclustered.empty()) {
        // Loop over hits in cluster, increases as we go
        for (unsigned ii=0; ii < clusters.back().nHits; ii++) {
            auto tHit = *clusters.back().hits[ii];
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
    return clusters;
}
