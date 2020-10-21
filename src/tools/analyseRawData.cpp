#include <iostream>
#include <fstream>
#include <array>
#include <memory>

#include "Fei4EventData.h"
#include "Histo1d.h"
#include "Histo2d.h"




void usage(char* argv[])
{
    std::cout << "Usage: " << argv[0] << " [options] inputFile1 inputFile2 ..." << std::endl;
    std::cout << "    List of options:" << std::endl;
    std::cout << "        -o outpu_dir (default: ./offline)" << std::endl;
    exit(1);
}

bool fileExists(const std::string& filename)
{
    std::ifstream ifile(filename);
    return ifile.good();
}

int main(int argc, char* argv[]) 
{
    if (argc < 2) {
        usage(argv);
        return -1;
    }

    std::string outputDir = "";

    // Parse CL
    int c;
    while ((c = getopt (argc, argv, "o:")) != -1)
    {
        switch (c)
        {
        case 'o':
            outputDir = std::string(optarg) + std::string("/");
            break;
        }
    }

    if (outputDir == "") {
        outputDir = "offline/";
    }

    std::string mkdirCmd = "mkdir -p " + outputDir;
    if (system(mkdirCmd.c_str()) < 0){
        std::cout << "#ERROR# Failed to create " << outputDir << " directory, not able to save plots!" << std::endl;
    }

    std::string inputFile;
    std::vector<std::string> inputFiles;
    if (optind < argc)
    {
        while (optind < argc)
        {
            inputFile = argv[optind++];

            if (! fileExists(inputFile)){
                std::cout << "#ERROR# Input file \"" << inputFile << "\" does not exist or cannot be opened" << std::endl;
                return -1;
            }

            inputFiles.push_back(inputFile);
        }
    }

    if (inputFiles.size() == 0)
    {
        usage(argv);
    }

    // Create histos
    Histo1d hitsPerEvent("hitsPerEvent", 31, -0.5, 30.5);
    hitsPerEvent.setXaxisTitle("# of Hits");
    hitsPerEvent.setYaxisTitle("# of Events");

    Histo1d hitsPerCluster("hitsPerCluster", 31, -0.5, 30.5);
    hitsPerCluster.setXaxisTitle("# of Hits");
    hitsPerCluster.setYaxisTitle("# of Events");

    Histo1d clusterColLength("clusterColLength", 31, -0.5, 30.5);
    clusterColLength.setXaxisTitle("Cluster Column Length");
    clusterColLength.setYaxisTitle("# of Clusters");

    Histo1d clusterRowWidth("clusterRowWidth", 31, -0.5, 30.5);
    clusterRowWidth.setXaxisTitle("Cluster Row Width");
    clusterRowWidth.setYaxisTitle("# of Clusters");

    Histo2d *eventScreen = NULL;

    Histo2d clusterWidthLengthCorr("clusterWidthLengthCorr", 11, -0.5, 10.5, 11, -0.5, 10.5);
    clusterWidthLengthCorr.setXaxisTitle("Cluster Col Length");
    clusterWidthLengthCorr.setYaxisTitle("Cluster Row Width");

    Histo1d clustersPerEvent("clustersPerEvent", 11, -0.5, 10.5);
    clustersPerEvent.setXaxisTitle("# of Clusters");
    clustersPerEvent.setYaxisTitle("# of Events");

    Histo1d hitsPerTrigger("hitsPerTrigger", 11, -0.5, 10.5);
    hitsPerTrigger.setXaxisTitle("# of Clusters");
    hitsPerTrigger.setYaxisTitle("# of Events");

    Histo1d bcid("bcid", 32768, -0.5, 32767.5);
    bcid.setXaxisTitle("BCID");
    bcid.setYaxisTitle("Number of Trigger");

    Histo1d bcidDiff("bcidDiff", 32768, -0.5, 32767.5);
    bcidDiff.setXaxisTitle("Delta BCID");
    bcidDiff.setYaxisTitle("Number of Trigger");

    Histo1d l1id("l1id", 32, -0.5, 31.5);
    l1id.setXaxisTitle("L1Id");
    l1id.setYaxisTitle("Number of Trigger");

    Histo2d occupancy("occupancy", 400, 0.5, 400.5, 192, 0.5, 192.5);
    occupancy.setXaxisTitle("Column");
    occupancy.setYaxisTitle("Row");
    occupancy.setZaxisTitle("Hits");


    // Only valid tag to l1id association
    //const std::array<unsigned, 16> l1ToTag = {{0,0,1,1,1,1,2,2,2,2,3,3,3,3,0,0}};
    //    const std::array<unsigned, 32> l1ToTag = {{0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,
    //                                               4,4,5,5,5,5,6,6,6,6,7,7,7,7,0,0}};


    // Loop over input files
    int skipped = 0;
    for (int i=0; i<inputFiles.size(); i++) {

        std::cout << "Opening file: " << inputFiles[i] << std::endl;
        std::fstream file(inputFiles[i], std::fstream::in | std::fstream::binary);

        // seekg doesn't give the right value for files over 2 GB

        //file.seekg(0, std::ios_base::end);
        //int size = file.tellg();
        //file.seekg(0, std::ios_base::beg);
        //std::cout << "Size of " << inputFiles[i] << " is: " << size/1024.0/1024.0 << " MB" << std::endl;        

        std::shared_ptr<Fei4Event> multiEvent(nullptr);
        std::shared_ptr<Fei4Event> event(nullptr);

        long int iterCounter = 0;
        int trigger     = 0;
        int old_offset  = 0;
        int n_truncated = 0;
        int l1_offset   = 0;
        int l1_count    = 0;
        int old_l1id    = 0;
        int max_bcid    = 0;
        int error       = 0;
        int nonZero_cnt = 0;
        int plotIt      = 0;
        int old_bcid    = 0;
        int weird       = 0;

        while (file) {

            iterCounter ++;

            event = std::make_shared<Fei4Event>();
            event->fromFileBinary(file);
            
            // Skip if not valid event
            int mod_l1id = (event->l1id-l1_offset+32)%32;
            if (event->tag==666) {
                skipped++;
                if (l1_count != 16 && l1_count != 0){
                  if(event->l1id != 666 and event->l1id != 31){
                    l1_offset = event->l1id+1;
                  }
                  if(event->bcid == 666){
                    l1_offset = old_l1id+1;
                  }
                  l1_count = 0;
                }
                continue;
            }

            if(l1_offset - old_offset != 0){
              n_truncated++;
            }

            old_offset = l1_offset;
            old_l1id = event->l1id;

            if (mod_l1id == 0 or mod_l1id == 16) {
              trigger++;
            }

            if (multiEvent == NULL) {
                multiEvent = event;
            } else {
                multiEvent->addEvent(*event);
            }
            
            if (l1_count - mod_l1id != 0) {
                error++;
            }

            if (event->bcid - old_bcid != 1 && not (l1_count == 0 or l1_count == 16)) {
                error++;
            }

            // Valid event
            l1_count++;
            // First event should have l1id 0
            if (l1_count == 1 && mod_l1id != 0) {
                weird++;
                //exit(-1);
            }

            for (auto hit : event->hits) {
                l1id.fill(mod_l1id);
                (void) hit;
            }
            
            if (iterCounter % 200000 == 0){
                std::cout << "\r Loaded events: " << trigger << "                    " << std::flush;
            }

            // Fill histos and start new multi-event container after 16 events
            if (mod_l1id == 31 || !file) {

                if(multiEvent != 0){
                    hitsPerEvent.fill(multiEvent->nHits);
                }else{
                    multiEvent = NULL;
                    l1_count = 0;
                    continue;
                }

                if (max_bcid < multiEvent->bcid){
                    max_bcid = multiEvent->bcid;
                }

                bcid.fill(multiEvent->bcid, multiEvent->nHits);

                if ((int)multiEvent->bcid - old_bcid < 0 && ((int)multiEvent->bcid-old_bcid+32768) > 16) {// wrap around, just reset
                    bcidDiff.fill((int)multiEvent->bcid-old_bcid+32768);
                    old_bcid = multiEvent->bcid;
                } else if ((int)multiEvent->bcid - old_bcid > 16) { 
                    bcidDiff.fill((int)multiEvent->bcid-old_bcid);
                    old_bcid = multiEvent->bcid;
                }

                if (multiEvent->nHits > 0) {
                    multiEvent->doClustering();
                    clustersPerEvent.fill(multiEvent->clusters.size());
                    nonZero_cnt++;
                    for (auto hit : multiEvent->hits) {
                        occupancy.fill(hit.col, hit.row);
                    }
                }

                for (auto cluster : multiEvent->clusters) {
                    hitsPerCluster.fill(cluster.nHits);
                    if (cluster.nHits > 1) {
                        clusterColLength.fill(cluster.getColLength());
                        clusterRowWidth.fill(cluster.getRowWidth());
                        clusterWidthLengthCorr.fill(cluster.getColLength(), cluster.getRowWidth());
                    }
                }

                if (multiEvent->clusters.size() > 0 && plotIt < 100) {
                    if (eventScreen == NULL) {
                        eventScreen = new Histo2d((std::to_string(nonZero_cnt) + "-eventScreen"), 400, 0.5, 400.5, 192, 0.5, 192.5);
                        eventScreen->setXaxisTitle("Column");
                        eventScreen->setYaxisTitle("Row");
                        eventScreen->setZaxisTitle("ToT");
                    }
                    for (auto cluster: multiEvent->clusters) {
                        for (auto hit : cluster.hits) {
                            eventScreen->fill(hit->col, hit->row, hit->tot);
                        }
                    }
                    if (plotIt%10 == 9) {
                        eventScreen->plot(std::to_string(plotIt), outputDir);
                        delete eventScreen;
                        eventScreen = new Histo2d((std::to_string(nonZero_cnt) + "-eventScreen"), 400, 0.5, 400.5, 192, 0.5, 192.5);
                        eventScreen->setXaxisTitle("Column");
                        eventScreen->setYaxisTitle("Row");
                        eventScreen->setZaxisTitle("ToT");
                    }
                    plotIt++;
                }

                multiEvent = NULL;
                l1_count = 0;
            }
        }
        std::cout << "\rLoaded triggers: " << trigger << "                    " << std::flush;


        file.close();
        std::cout << std::endl;

        std::cout << "Number of errors: " << error << std::endl;\
        std::cout << "Number of first events without the right l1id: " << weird << std::endl;\
        std::cout << "Max BCID: " << max_bcid << std::endl;
        std::cout << "Number of trigger: " << trigger << std::endl;
        std::cout << "Number of truncated events: " << n_truncated << std::endl;
    }
    

    // Save occupancy data
    occupancy.toFile(outputDir + "offline");

    int sum = 0;
    for (unsigned i=0; i<400*192; i++) {
        sum += occupancy.getBin(i);
    }
    double mean=(double)sum/(400.0*192.0);
    std::cout << "Occupancy mean = " << mean << std::endl;
    if (mean < 3.0) 
        mean = 3;
    int noisy = 0;
    for (unsigned i=0; i<400*192; i++) {
        if (occupancy.getBin(i) > (mean*5)) {
            //std::cout << "Flagged bin " << i << " noisy " << occupancy.getBin(i) << std::endl;
            occupancy.setBin(i, 0);
            noisy ++;
        }
    }

    bcid.plot("offline", outputDir);
    l1id.plot("offline", outputDir);
    bcidDiff.plot("offline", outputDir);
    hitsPerEvent.plot("offline", outputDir);
    hitsPerCluster.plot("offline", outputDir);
    clusterColLength.plot("offline", outputDir);
    clusterRowWidth.plot("offline", outputDir);
    clusterWidthLengthCorr.plot("offline", outputDir);
    clustersPerEvent.plot("offline", outputDir);
    occupancy.plot("offline", outputDir);

    std::cout << "Cluster Column Length mean: " << clusterColLength.getMean() << " +- " << clusterColLength.getStdDev() << std::endl;
    std::cout << "Cluster Row Width mean:     " << clusterRowWidth.getMean() << " +- " << clusterRowWidth.getStdDev() << std::endl;
    std::cout << "BCID entries: " << bcid.getEntries() << std::endl;
    std::cout << "BCIDdiff entries: " << bcidDiff.getEntries() << std::endl;
    std::cout << "Number of clusters: " << clustersPerEvent.getEntries() << std::endl;
    std::cout << "Number of events: " << hitsPerEvent.getEntries() << std::endl;
    std::cout << "Number of skipped events: " << skipped << std::endl;
    std::cout << "Number Bins flagged noisy (occupancy map): " << noisy << std::endl;

    return 0;
}
