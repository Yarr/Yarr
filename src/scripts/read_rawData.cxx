#include <iostream>
#include <fstream>

#include "../libFei4/include/Fei4EventData.h"
#include "TTree.h"
#include "TFile.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Provide input file!" << std::endl;
        return -1;
    }

    std::cout << "setting up the ROOT file" << std::endl;
    TFile* rootfile = new TFile("rootfile.root","RECREATE");
    TTree* t1 = new TTree("HitTree", "Hit Tree");
    int nhits = -1;
    int bcid = -1;
    int l1id = -1;
    const Int_t nhits_max = 500;
    int col[nhits_max];
    int row[nhits_max];
    int tot[nhits_max];
    t1->Branch("nhits", &nhits, "nhits/I");
    t1->Branch("bcid", &bcid, "bcid/I");
    t1->Branch("l1id", &l1id, "l1id/I");
    t1->Branch("col", &col, "col[nhits]/I");
    t1->Branch("row", &row, "row[nhits]/I");
    t1->Branch("tot", &tot, "tot[nhits]/I");
    std::cout << "done setting up the ROOT file" << std::endl;

    for (int i=1; i<argc; i++) {
        std::cout << "Opening file: " << argv[i] << std::endl;
        std::fstream file(argv[i], std::fstream::in | std::fstream::binary);
        
        file.seekg(0, std::ios_base::end);
        int size = file.tellg();
        file.seekg(0, std::ios_base::beg);
        std::cout << "Size: " << size/1024.0/1024.0 << " MB" << std::endl;

        while (file) {
            Fei4Event event;
            event.fromFileBinary(file);
            if (!file)
                break;
	    nhits = event.nHits;
	    bcid = event.bcid;
	    l1id = event.l1id;
            for (unsigned i=0; i<event.nHits; i++) {
	      col[i] = event.hits[i].col;
	      row[i] = event.hits[i].row;
	      tot[i] = event.hits[i].tot;
            }
	    t1->Fill();
            //std::cout << "Now: " << file.tellg() << std::endl;
        }

        file.close();
    }
    t1->Write();
    rootfile->Close();

    return 0;
}
