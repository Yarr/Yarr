#include <iostream>
#include <fstream>
#include <array>
#include <memory>

#include "Fei4EventData.h"
#include "Histo1d.h"
#include "Histo2d.h"

class HistoContainer
{
    public:

        std::vector<Histo1d> histos1d;
        std::vector<Histo2d> histos2d;

        void createHisto(const std::string &arg_name, unsigned arg_bins, double arg_xlow, double arg_xhigh)
        {
            histos1d.push_back(Histo1d(arg_name, arg_bins, arg_xlow, arg_xhigh));
        }
        void createHisto(const std::string &arg_name, unsigned arg_xbins, double arg_xlow, double arg_xhigh,
                unsigned arg_ybins, double arg_ylow, double arg_yhigh)
        {
            histos2d.push_back(Histo2d(arg_name, arg_xbins, arg_xlow, arg_xhigh, arg_ybins, arg_ylow, arg_yhigh));
        }

        Histo1d * getHisto1D(const std::string &arg_name)
        {
            int len = histos1d.size();
            for (int i = 0; i < len; i++)
            {
                std::string name = histos1d[i].getName();
                if ( name == arg_name){
                    return &histos1d[i];
                }
            }
            std::cout << "Couldn't find histogram " << arg_name << std::endl;
            return nullptr;
        }

        Histo2d * getHisto2D(const std::string &arg_name)
        {
            int len = histos2d.size();
            for (int i = 0; i < len; i++)
            {
                std::string name = histos2d[i].getName();
                if ( name == arg_name){
                    //return  *histo;
                    return &histos2d[i];
                }
            }
            std::cout << "Couldn't find histogram " << arg_name << std::endl;
            return nullptr;
        }
};

struct VariableContainer
{
    int count = 0;
    int nonZero_cnt = 0;
    int old_bcid = 0;
    int max_bcid = 0;

    std::string outputDir = "";
};

void processMultiEvent(std::list<std::shared_ptr<Fei4Event>> eventList, HistoContainer &histos, VariableContainer *varContainer){

    std::cout << std::endl;

    // Get the histos now so accessing them is faster inside the loop
    Histo1d *bcidDiff = histos.getHisto1D("bcidDiff");
    Histo1d *clustersPerEvent = histos.getHisto1D("clustersPerEvent");
    Histo1d *hitsPerEvent = histos.getHisto1D("hitsPerEvent");
    Histo1d *bcid = histos.getHisto1D("bcid");
    Histo1d *clusterColLength = histos.getHisto1D("clusterColLength");
    Histo1d *hitsPerCluster = histos.getHisto1D("hitsPerCluster");
    Histo1d *clusterRowWidth = histos.getHisto1D("clusterRowWidth");
    Histo2d *eventScreen = NULL;
    Histo2d *occupancy = histos.getHisto2D("occupancy");
    Histo2d *clusterWidthLengthCorr = histos.getHisto2D("clusterWidthLengthCorr");


    // Variables used only inside this function
    int len         = eventList.size();
    int iterCounter = 0;
    int count       = 0;
    int nonZero_cnt = 0;
    int plotIt      = 0;
    int old_bcid    = varContainer->old_bcid;
    int other_old_bcid = 0;


    // Loop over event List
    for (std::shared_ptr<Fei4Event> event: eventList) {

        // print progress every 1000 iterations
        if (iterCounter % 1000 == 0){
            std::cout << "\r Filling histograms with loaded events... " << ((float)iterCounter + 1)/len * 100 << " %                    " << std::flush;
        }
        iterCounter ++;

        if(event != 0){
            hitsPerEvent->fill(event->nHits);
        }else{
            continue;
        }

        if (varContainer->max_bcid < event->bcid){
            varContainer->max_bcid = event->bcid;
        }

        bcid->fill(event->bcid, event->nHits);

        if ((int)event->bcid - old_bcid < 0 && ((int)event->bcid-old_bcid+32768) > 16) {// wrap around, just reset
            bcidDiff->fill((int)event->bcid-old_bcid+32768);
            old_bcid = event->bcid;
        } else if ((int)event->bcid - old_bcid > 16) { 
            bcidDiff->fill((int)event->bcid-old_bcid);
            old_bcid = event->bcid;
        }

        if (event->nHits > 0) {
            event->doClustering();
            clustersPerEvent->fill(event->clusters.size());
            nonZero_cnt++;
            for (auto hit : event->hits) {
                occupancy->fill(hit.col, hit.row);
            }
        }

        for (auto cluster : event->clusters) {
            hitsPerCluster->fill(cluster.nHits);
            if (cluster.nHits > 1) {
                clusterColLength->fill(cluster.getColLength());
                clusterRowWidth->fill(cluster.getRowWidth());
                clusterWidthLengthCorr->fill(cluster.getColLength(), cluster.getRowWidth());
            }
        }

        if (event->clusters.size() > 0 && plotIt < 100) {
            if (eventScreen == NULL) {
                eventScreen = new Histo2d((std::to_string(nonZero_cnt) + "-eventScreen"), 400, 0.5, 400.5, 192, 0.5, 192.5);
                eventScreen->setXaxisTitle("Column");
                eventScreen->setYaxisTitle("Row");
                eventScreen->setZaxisTitle("ToT");
            }
            for (auto cluster: event->clusters) {
                for (auto hit : cluster.hits) {
                    eventScreen->fill(hit->col, hit->row, hit->tot);
                }
            }
            if (plotIt%10 == 9) {
                eventScreen->plot(std::to_string(plotIt), varContainer->outputDir);
                delete eventScreen;
                eventScreen = new Histo2d((std::to_string(nonZero_cnt) + "-eventScreen"), 400, 0.5, 400.5, 192, 0.5, 192.5);
                eventScreen->setXaxisTitle("Column");
                eventScreen->setYaxisTitle("Row");
                eventScreen->setZaxisTitle("ToT");
            }
            plotIt++;
        }
        count ++;
    }
    std::cout << std::endl;
}

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

    // object containing all the variables used also outside main()
    VariableContainer varContainer; 

    // Parse CL
    int c;
    while ((c = getopt (argc, argv, "o:")) != -1)
    {
        switch (c)
        {
        case 'o':
            varContainer.outputDir = std::string(optarg) + std::string("/");
            break;
        }
    }

    if (varContainer.outputDir == "") {
        varContainer.outputDir = "offline/";
    }

    std::string mkdirCmd = "mkdir -p " + varContainer.outputDir;
    if (system(mkdirCmd.c_str()) < 0){
        std::cout << "#ERROR# Failed to create " << varContainer.outputDir << " directory, not able to save plots!" << std::endl;
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

    // Container for the histos to be used outside main
    HistoContainer histos;

    histos.createHisto("hitsPerEvent", 31, -0.5, 30.5);
    histos.getHisto1D("hitsPerEvent")->setXaxisTitle("# of Hits");
    histos.getHisto1D("hitsPerEvent")->setYaxisTitle("# of Events");

    histos.createHisto("hitsPerCluster", 31, -0.5, 30.5);
    histos.getHisto1D("hitsPerCluster")->setXaxisTitle("# of Hits");
    histos.getHisto1D("hitsPerCluster")->setYaxisTitle("# of Events");

    histos.createHisto("clusterColLength", 31, -0.5, 30.5);
    histos.getHisto1D("clusterColLength")->setXaxisTitle("Cluster Column Length");
    histos.getHisto1D("clusterColLength")->setYaxisTitle("# of Clusters");

    histos.createHisto("clusterRowWidth", 31, -0.5, 30.5);
    histos.getHisto1D("clusterRowWidth")->setXaxisTitle("Cluster Row Width");
    histos.getHisto1D("clusterRowWidth")->setYaxisTitle("# of Clusters");

    histos.createHisto("clusterWidthLengthCorr", 11, -0.5, 10.5, 11, -0.5, 10.5);
    histos.getHisto2D("clusterWidthLengthCorr")->setXaxisTitle("Cluster Col Length");
    histos.getHisto2D("clusterWidthLengthCorr")->setYaxisTitle("Cluster Row Width");

    histos.createHisto("clustersPerEvent", 11, -0.5, 10.5);
    histos.getHisto1D("clustersPerEvent")->setXaxisTitle("# of Clusters");
    histos.getHisto1D("clustersPerEvent")->setYaxisTitle("# of Events");

    histos.createHisto("clustersPerEvent", 11, -0.5, 10.5);
    histos.getHisto1D("clustersPerEvent")->setXaxisTitle("# of Clusters");
    histos.getHisto1D("clustersPerEvent")->setYaxisTitle("# of Events");

    histos.createHisto("bcid", 32768, -0.5, 32767.5);
    histos.getHisto1D("bcid")->setXaxisTitle("BCID");
    histos.getHisto1D("bcid")->setYaxisTitle("Number of Trigger");

    histos.createHisto("bcidDiff", 32768, -0.5, 32767.5);
    histos.getHisto1D("bcidDiff")->setXaxisTitle("Delta BCID");
    histos.getHisto1D("bcidDiff")->setYaxisTitle("Number of Trigger");

    histos.createHisto("l1id", 32, -0.5, 31.5);
    histos.getHisto1D("l1id")->setXaxisTitle("L1Id");
    histos.getHisto1D("l1id")->setYaxisTitle("Number of Trigger");

    histos.createHisto("occupancy", 400, 0.5, 400.5, 192, 0.5, 192.5);
    histos.getHisto2D("occupancy")->setXaxisTitle("Column");
    histos.getHisto2D("occupancy")->setYaxisTitle("Row");
    histos.getHisto2D("occupancy")->setZaxisTitle("Hits");

    // Only valid tag to l1id association
    //const std::array<unsigned, 16> l1ToTag = {{0,0,1,1,1,1,2,2,2,2,3,3,3,3,0,0}};
    //    const std::array<unsigned, 32> l1ToTag = {{0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,
    //                                               4,4,5,5,5,5,6,6,6,6,7,7,7,7,0,0}};

    // Loop over input files
    int skipped = 0;
    for (int i=0; i<inputFiles.size(); i++) {

        std::cout << "Opening file: " << inputFiles[i] << std::endl;
        std::fstream file(inputFiles[i], std::fstream::in | std::fstream::binary);

        file.seekg(0, std::ios_base::end);
        int size = file.tellg();
        file.seekg(0, std::ios_base::beg);
        std::cout << "Size of " << inputFiles[i] << " is: " << size/1024.0/1024.0 << " MB" << std::endl;        

        auto multiEvent = std::make_shared<Fei4Event>();
        std::list<std::shared_ptr<Fei4Event>> eventList;
        int error = 0;

        long int iterCounter = 0;
        int packageCounter = 1;
        long int eventsPerckage = 1e7;
        std::shared_ptr<Fei4Event> event(nullptr);
        int trigger = 0;
        int old_offset = 0;
        int n_truncated = 0;
        int l1_offset = 0;
        int l1_count = 0;
        int old_l1id = 0;
        

        while (file) {

            iterCounter ++;
            if (iterCounter % 100000 == 0){
                std::cout << "\r Loaded events: " << trigger << "                    " << std::flush;
            }

            event = std::make_shared<Fei4Event>();
            event->fromFileBinary(file);
            
            // Skip if not valid event
            int mod_l1id = (event->l1id-l1_offset+32)%32;
            if (event->tag==666) {
                skipped++;
                eventList.push_back(multiEvent);
                multiEvent = NULL;
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

            // Skip if EOF
            if (!file) {
                processMultiEvent(eventList, histos, &varContainer);
                break;
            }

            if(l1_offset - old_offset != 0){
              n_truncated++;
            }
            old_offset = l1_offset;
            old_l1id = event->l1id;
            if (mod_l1id == 0 or mod_l1id == 16) {
              trigger++;
              //std::cout << " #### Event " << trigger << " #### " << std::endl;
            }

            if (multiEvent == NULL) {
                multiEvent = event;
            } else {
                multiEvent->addEvent(*event);
            }
            
            if (l1_count - mod_l1id != 0) {
                error++;
            }

            if (event->bcid - varContainer.old_bcid != 1 && not (l1_count == 0 or l1_count == 16)) {
                error++;
            }
            varContainer.old_bcid = event->bcid; 

            // Valid event
            l1_count++;
            // First event should have l1id 0
            if (l1_count == 1 && mod_l1id != 0) {
                //std::cout << "... wierd first event does not have the right l1id" << std::endl;
                //exit(-1);
            }

            for (auto hit : event->hits) {
                histos.getHisto1D("l1id")->fill(mod_l1id);
                (void) hit;
            }
            
            // Start new multi-event container after 16 events
            if (mod_l1id == 31) {
                eventList.push_back(multiEvent);
                multiEvent = NULL;
                l1_count = 0;
            }

            // Once eventList is filled to some extent, use it to fill the
            // histos, empty it afterwards and repeat the process. This prevents
            // memory problems when the input file is too big.
            if (trigger > eventsPerckage*packageCounter)
            {
                iterCounter = 0;
                packageCounter ++;
                processMultiEvent(eventList, histos, &varContainer);
                eventList.clear();
            }
        }
        file.close();
        std::cout << std::endl;

        std::cout << "Number of errors: " << error << std::endl;\
        std::cout << "Max BCID: " << varContainer.max_bcid << std::endl;
        std::cout << "Numer of trigger: " << trigger << std::endl;
        std::cout << "Number of truncated events: " << n_truncated << std::endl;
    }
    
    // Save occupancy data
    histos.getHisto2D("occupancy")->toFile(varContainer.outputDir + "offline_");

    int sum = 0;
    for (unsigned i=0; i<400*192; i++) {
        sum += histos.getHisto2D("occupancy")->getBin(i);
    }
    double mean=(double)sum/(400.0*192.0);
    std::cout << "Occupancy mean = " << mean << std::endl;
    if (mean < 3.0) 
        mean = 3;
    for (unsigned i=0; i<400*192; i++) {
        if (histos.getHisto2D("occupancy")->getBin(i) > (mean*5)) {
            std::cout << "Flagged bin " << i << " noisy " << histos.getHisto2D("occupancy")->getBin(i) << std::endl;
            histos.getHisto2D("occupancy")->setBin(i, 0);
        }
    }

    histos.getHisto1D("bcid")->plot("offline", varContainer.outputDir);
    histos.getHisto1D("l1id")->plot("offline", varContainer.outputDir);
    histos.getHisto1D("bcidDiff")->plot("offline", varContainer.outputDir);
    histos.getHisto1D("hitsPerEvent")->plot("offline", varContainer.outputDir);
    histos.getHisto1D("hitsPerCluster")->plot("offline", varContainer.outputDir);
    histos.getHisto1D("clusterColLength")->plot("offline", varContainer.outputDir);
    histos.getHisto1D("clusterRowWidth")->plot("offline", varContainer.outputDir);
    histos.getHisto2D("clusterWidthLengthCorr")->plot("offline", varContainer.outputDir);
    histos.getHisto1D("clustersPerEvent")->plot("offline", varContainer.outputDir);
    histos.getHisto2D("occupancy")->plot("offline", varContainer.outputDir);

    std::cout << "Cluster Column Length mean: " << histos.getHisto1D("clusterColLength")->getMean() << " +- " << histos.getHisto1D("clusterColLength")->getStdDev() << std::endl;
    std::cout << "Cluster Row Width mean:     " << histos.getHisto1D("clusterRowWidth")->getMean() << " +- " << histos.getHisto1D("clusterRowWidth")->getStdDev() << std::endl;
    std::cout << "BCID entries: " << histos.getHisto1D("bcid")->getEntries() << std::endl;
    std::cout << "BCIDdiff entries: " << histos.getHisto1D("bcidDiff")->getEntries() << std::endl;
    std::cout << "Number of clusters: " << histos.getHisto1D("clustersPerEvent")->getEntries() << std::endl;
    std::cout << "Number of events: " << histos.getHisto1D("hitsPerEvent")->getEntries() << std::endl;
    std::cout << "Number of skipped events: " << skipped << std::endl;

    return 0;
}
