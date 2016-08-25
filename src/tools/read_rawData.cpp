#include <iostream>
#include <fstream>

#include "Fei4EventData.h"
#include "Histo1d.h"
#include "Histo2d.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Provide input file!" << std::endl;
        return -1;
    }

    Histo1d hitsPerEvent("hitsPerEvent", 31, -0.5, 30.5, typeid(void));
    hitsPerEvent.setXaxisTitle("# of Hits");
    hitsPerEvent.setYaxisTitle("# of Events");
    
    Histo1d hitsPerCluster("hitsPerCluster", 31, -0.5, 30.5, typeid(void));
    hitsPerCluster.setXaxisTitle("# of Hits");
    hitsPerCluster.setYaxisTitle("# of Events");
    
    Histo2d *eventScreen = NULL;
    //Histo2d eventScreen("eventScreen", 64, 0.5, 64.5, 64, 0.5, 64.5, typeid(void));
    
    Histo1d clusterColLength("clusterColLength", 31, -0.5, 30.5, typeid(void));
    clusterColLength.setXaxisTitle("Cluster Column Length");
    clusterColLength.setYaxisTitle("# of Clusters");

    Histo1d clusterRowWidth("clusterRowWidth", 31, -0.5, 30.5, typeid(void));
    clusterRowWidth.setXaxisTitle("Cluster Row Width");
    clusterRowWidth.setYaxisTitle("# of Clusters");

    Histo2d clusterWidthLengthCorr("clusterWidthLengthCorr", 11, -0.5, 10.5, 11, -0.5, 10.5, typeid(void));
    clusterWidthLengthCorr.setXaxisTitle("Cluster Col Length");
    clusterWidthLengthCorr.setYaxisTitle("Cluster Row Width");

    Histo1d clustersPerEvent("clustersPerEvent", 11, -0.5, 10.5, typeid(void));
    clustersPerEvent.setXaxisTitle("# of Clusters");
    clustersPerEvent.setYaxisTitle("# of Events");

    Histo1d bcid("bcid", 10000001, -0.5, 10000000.5, typeid(void));
    Histo1d bcidDiff("bcidDiff", 501, -0.5, 500.5, typeid(void));

    for (int i=1; i<argc; i++) {
        std::cout << "Opening file: " << argv[i] << std::endl;
        std::fstream file(argv[i], std::fstream::in | std::fstream::binary);

        file.seekg(0, std::ios_base::end);
        int size = file.tellg();
        file.seekg(0, std::ios_base::beg);
        std::cout << "Size: " << size/1024.0/1024.0 << " MB" << std::endl;        

        int count = 0;
        int nonZero_cnt = 0;
        int plotIt = 0;
        int old_bcid = 0;
        int other_old_bcid = 0;
        unsigned long timestamp = 0;
        while (file) {
            int now = file.tellg();
            //std::cout << "\r" << (double)now/(double)size*100 << "%" << std::flush;
            Fei4Event event;
            event.fromFileBinary(file);
            if (!file)
                break;

            hitsPerEvent.fill(event.nHits);
           
            if (count==0)
                other_old_bcid = event.bcid;

            std::cout << timestamp << std::endl;
            if (event.bcid - other_old_bcid < 0) {
                timestamp+= (34935 + (int)event.bcid - other_old_bcid);
            } else {
                timestamp+= (event.bcid - other_old_bcid);
            }
            bcid.fill(timestamp, event.nHits);

            if (timestamp > 10000000)
                break;

            if ((int)event.bcid - old_bcid < 0) // wrap around, just reset
                old_bcid = event.bcid;

            if ((int)event.bcid -old_bcid > 5) { 
                bcidDiff.fill((int)event.bcid-old_bcid);
                old_bcid = event.bcid;
            }
            if (event.nHits > 0) {
                event.doClustering();
                clustersPerEvent.fill(event.clusters.size());
            }

            if(event.nHits > 1) {
                for (unsigned i=0; i<event.clusters.size(); i++) {
                    hitsPerCluster.fill(event.clusters[i].nHits);
                    clusterColLength.fill(event.clusters[i].getColLength());
                    clusterRowWidth.fill(event.clusters[i].getRowWidth());
                    clusterWidthLengthCorr.fill(event.clusters[i].getColLength(), event.clusters[i].getRowWidth());

                }
                if (event.clusters.size() > 1) {
                    eventScreen = new Histo2d((std::to_string(nonZero_cnt) + "-eventScreen"), 64, 0.5, 64.5, 64, 0.5, 64.5, typeid(void));
                    for (unsigned i=0; i<event.nHits; i++) {
                        eventScreen->fill(event.hits[i].col, event.hits[i].row, event.hits[i].tot);
                    }
                    //eventScreen->plot(std::to_string(plotIt));
                    plotIt++;
                    delete eventScreen;
                }


                /*
                std::cout << "Number of clusters in event: " << event.clusters.size() << std::endl;
                std::cout << "Number of hit in first cluster in event: " << event.clusters[0].hits.size() << std::endl;
                if (event.clusters[0].hits.size() > 6) {
                    if (eventScreen != NULL) {
                        std::cout << "Plotting #" << nonZero_cnt << std::endl;
                        eventScreen->plot(std::to_string(nonZero_cnt));
                        delete eventScreen;
                        eventScreen = NULL;
                        plotIt++;
                    }
                    eventScreen = new Histo2d((std::to_string(nonZero_cnt) + "-eventScreen"), 64, 0.5, 64.5, 64, 0.5, 64.5, typeid(void));
                    for (unsigned i=0; i<event.nHits; i++) {
                        //std::cout << event.hits[i].col << " " << event.hits[i].row << " " << event.hits[i].tot << std::endl;
                        eventScreen->fill(event.hits[i].col, event.hits[i].row, event.hits[i].tot);
                    }
                }            
                //std::cout << "~~ Event #" << count << " ~~" << std::endl;
                //std::cout << event.l1id << " " << event.bcid << " " << event.nHits << std::endl;
                */
                nonZero_cnt++;
            }
            count ++;
        }
        std::cout << std::endl;
        file.close();
    }

    bcid.plot("offline");
    bcidDiff.plot("offline");
    hitsPerEvent.plot("offline");
    hitsPerCluster.plot("offline");
    clusterColLength.plot("offline");
    clusterRowWidth.plot("offline");
    clusterWidthLengthCorr.plot("offline");
    clustersPerEvent.plot("offline");
    
    std::cout << "Number of events: " << clustersPerEvent.getEntries() << std::endl;
    std::cout << "Number of clusters: " << hitsPerEvent.getEntries() << std::endl;

    return 0;
}
