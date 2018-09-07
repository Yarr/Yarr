#include <iostream>
#include <fstream>
#include <array>

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

    Histo1d hitsPerTrigger("clustersPerEvent", 11, -0.5, 10.5, typeid(void));
    hitsPerTrigger.setXaxisTitle("# of Clusters");
    hitsPerTrigger.setYaxisTitle("# of Events");

    Histo1d bcid("bcid", 32768, -0.5, 32767.5, typeid(void));
    bcid.setXaxisTitle("BCID");
    bcid.setYaxisTitle("Number of Trigger");

    Histo1d bcidDiff("bcidDiff", 32768, -0.5, 32767.5, typeid(void));
    bcidDiff.setXaxisTitle("Delta BCID");
    bcidDiff.setYaxisTitle("Number of Trigger");

    Histo1d l1id("l1id", 32, -0.5, 31.5, typeid(void));
    l1id.setXaxisTitle("L1Id");
    l1id.setYaxisTitle("Number of Trigger");

    Histo2d occupancy("occupancy", 400, 0.5, 400.5, 192, 0.5, 192.5, typeid(void));
    occupancy.setXaxisTitle("Column");
    occupancy.setYaxisTitle("Row");
    occupancy.setZaxisTitle("Hits");

    const std::array<unsigned, 16> l1ToTag = {{1,1,2,2,2,2,3,3,3,3,4,4,4,4,0,0}};

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
        int max_bcid = 0;
        int trigger = 0;
        int old_l1id = -1;

        Fei4Event *next_event = new Fei4Event;
        Fei4Event *prev_event = new Fei4Event;

        next_event->fromFileBinary(file);
        while (file) {
            //int now = file.tellg();
            //std::cout << "\r" << (double)now/(double)size*100 << "%\t\t" << std::flush;

            Fei4Event *event = new Fei4Event(*next_event);
            delete prev_event;
            prev_event = next_event;
            next_event = new Fei4Event;

            next_event->fromFileBinary(file);

            if (l1ToTag[prev_event->l1id%16] != prev_event->tag)
                continue;

            if (!file) {
                break;
            }


            if (l1ToTag[prev_event->l1id%16] != prev_event->tag) {
                std::cout << "############" << std::endl;
                std::cout << "Tag: " << prev_event->tag << std::endl;
                std::cout << "L1Id: " << prev_event->l1id << std::endl;
                std::cout << "BCId: " << prev_event->bcid << std::endl;
                std::cout << "Hits: " << prev_event->nHits << std::endl;
            }

            int l1_count = 1;
            while ((((int)next_event->bcid+32768) - (int)prev_event->bcid)%32768 == 1) {
                event->addEvent(*next_event);
                l1_count++;
                if (l1_count > 16 && false) {
                    std::cout << "############" << std::endl;
                    std::cout << "Tag: " << next_event->tag << std::endl;
                    std::cout << "L1Id: " << next_event->l1id << std::endl;
                    std::cout << "BCId: " << next_event->bcid << std::endl;
                    std::cout << "Hits: " << next_event->nHits << std::endl;

                }
                if (l1ToTag[prev_event->l1id%16] != prev_event->tag) {
                    std::cout << "############" << std::endl;
                    std::cout << "Tag: " << next_event->tag << std::endl;
                    std::cout << "L1Id: " << next_event->l1id << std::endl;
                    std::cout << "BCId: " << next_event->bcid << std::endl;
                    std::cout << "Hits: " << next_event->nHits << std::endl;
                }

                delete prev_event;
                prev_event = next_event;
                next_event = new Fei4Event;
                next_event->fromFileBinary(file);
                if (!file)
                    break;

            }

            if (l1_count != 16)
                std::cout << "L1 count: " << l1_count << " at event " << count << " L1ID(" << event->l1id <<") BCID(" << event->bcid << ") TAG(" << event->tag << ") HITS(" << event->nHits << ")" << std::endl;

            if (event->l1id == 0 && event->bcid == 0 && event->tag == 0) {
                for (auto hit : event->hits) {
                    std::cout << "Col(" << hit.col << ") Row(" << hit.row << ") ToT(" << hit.tot << ")" << std::endl;
                }
            }

            hitsPerEvent.fill(event->nHits);
            l1id.fill(event->l1id);


            if (max_bcid < event->bcid)
                max_bcid = event->bcid;

            if ((event->l1id - old_l1id) > 0 || event->l1id < old_l1id) {
                trigger++;
            }
            old_l1id = event->l1id;

            //std::cout << "Tag: " << event->tag << std::endl;
            if (count==0)
                other_old_bcid = event->bcid;
            (void)other_old_bcid;

            bcid.fill(event->bcid, event->nHits);

            if ((int)event->bcid - old_bcid < 0 && ((int)event->bcid-old_bcid+32768) > 16) {// wrap around, just reset
                bcidDiff.fill((int)event->bcid-old_bcid+32768);
                old_bcid = event->bcid;
            } else if ((int)event->bcid - old_bcid > 16) { 
                bcidDiff.fill((int)event->bcid-old_bcid);
                old_bcid = event->bcid;
            }

            if (event->nHits > 0) {
                event->doClustering();
                clustersPerEvent.fill(event->clusters.size());
                nonZero_cnt++;
                for (auto hit : event->hits) {
                    occupancy.fill(hit.col, hit.row);
                }
            }

            for (auto cluster : event->clusters) {
                hitsPerCluster.fill(cluster.nHits);
                if (cluster.nHits > 1) {
                    clusterColLength.fill(cluster.getColLength());
                    clusterRowWidth.fill(cluster.getRowWidth());
                    clusterWidthLengthCorr.fill(cluster.getColLength(), cluster.getRowWidth());
                }

            }
            if (event->clusters.size() > 0 && plotIt < 100) {
                if (eventScreen == NULL) {
                    eventScreen = new Histo2d((std::to_string(nonZero_cnt) + "-eventScreen"), 400, 0.5, 400.5, 192, 0.5, 192.5, typeid(void));
                    eventScreen->setXaxisTitle("Column");
                    eventScreen->setYaxisTitle("Row");
                    eventScreen->setZaxisTitle("ToT");
                }
                int cluster_cnt = 1;
                for (auto cluster: event->clusters) {
                    for (auto hit : cluster.hits) {
                        //std::cout << hit->col << " " << hit->row << std::endl;
                        eventScreen->fill(hit->col, hit->row, hit->tot);
                    }
                    cluster_cnt++;
                }
                if (plotIt%10 == 9) {
                    eventScreen->plot(std::to_string(plotIt), "offline/");
                    delete eventScreen;
                    eventScreen = new Histo2d((std::to_string(nonZero_cnt) + "-eventScreen"), 400, 0.5, 400.5, 192, 0.5, 192.5, typeid(void));
                    eventScreen->setXaxisTitle("Column");
                    eventScreen->setYaxisTitle("Row");
                    eventScreen->setZaxisTitle("ToT");
                }
                plotIt++;
            }

            count ++;
            delete event;
        }
        std::cout << std::endl;
        file.close();
        std::cout << "Max BCID: " << max_bcid << std::endl;
        std::cout << "Numer of trigger: " << trigger << std::endl;
    }

    int sum = 0;
    for (unsigned i=0; i<400*192; i++) {
        sum += occupancy.getBin(i);
    }
    double mean=(double)sum/(400.0*192.0);
    std::cout << "Occupancy mean = " << mean << std::endl;
    if (mean < 3.0) 
        mean = 3;
    for (unsigned i=0; i<400*192; i++) {
        if (occupancy.getBin(i) > (mean*5)) {
            std::cout << "Flagged bin " << i << " nosiy " << occupancy.getBin(i) << std::endl;
            occupancy.setBin(i, 0);
        }
    }



    bcid.plot("offline", "offline/");
    l1id.plot("offline", "offline/");
    bcidDiff.plot("offline", "offline/");
    hitsPerEvent.plot("offline", "offline/");
    hitsPerCluster.plot("offline", "offline/");
    clusterColLength.plot("offline", "offline/");
    clusterRowWidth.plot("offline", "offline/");
    clusterWidthLengthCorr.plot("offline", "offline/");
    clustersPerEvent.plot("offline", "offline/");
    occupancy.plot("offline", "offline/");

    std::cout << "Cluster Column Length mean: " << clusterColLength.getMean() << " +- " << clusterColLength.getStdDev() << std::endl;
    std::cout << "Cluster Row Width mean:     " << clusterRowWidth.getMean() << " +- " << clusterRowWidth.getStdDev() << std::endl;
    std::cout << "BCID entries: " << bcid.getEntries() << std::endl;
    std::cout << "BCIDdiff entries: " << bcidDiff.getEntries() << std::endl;
    std::cout << "Number of clusters: " << clustersPerEvent.getEntries() << std::endl;
    std::cout << "Number of events: " << hitsPerEvent.getEntries() << std::endl;

    return 0;
}
