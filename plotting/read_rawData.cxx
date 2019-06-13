#include <iostream>
#include <fstream>
#include <vector>

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
    int pixel_cluster_id[nhits_max];
    t1->Branch("nhits", &nhits, "nhits/I");
    t1->Branch("hit_bcid", &bcid, "hit_bcid/I");
    t1->Branch("hit_l1id", &l1id, "hit_l1id/I");
    t1->Branch("hit_col", &col, "hit_col[nhits]/I");
    t1->Branch("hit_row", &row, "hit_row[nhits]/I");
    t1->Branch("hit_tot", &tot, "hit_tot[nhits]/I");
    t1->Branch("pixel_cluster_id", &pixel_cluster_id, "pixel_cluster_id[nhits]/I");

    int nclusters = -1;
    int cluster_ncol[nhits_max];
    int cluster_nrow[nhits_max];
    int cluster_size[nhits_max];
    int cluster_tot[nhits_max];
    int cluster_id[nhits_max];
    t1->Branch("nclusters", &nclusters, "nclusters/I");
    t1->Branch("cluster_tot", &cluster_tot, "cluster_tot[nclusters]/I");
    t1->Branch("cluster_id", &cluster_id, "cluster_id[nclusters]/I");
    t1->Branch("cluster_size", &cluster_size, "cluster_size[nclusters]/I");
    t1->Branch("cluster_ncol", &cluster_ncol, "cluster_ncol[nclusters]/I");
    t1->Branch("cluster_nrow", &cluster_nrow, "cluster_nrow[nclusters]/I");

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
        unsigned i =0;
        for (auto &hit : event.hits) {
            col[i] = hit.col;
            row[i] = hit.row;
            tot[i] = hit.tot;
            pixel_cluster_id[i] = -1;
            i++;
        }
            //std::cout << "Now: " << file.tellg() << std::endl;

	    //Let's now run a CCA for clustering.
	    
	    int k = 0;
	    for (int i=0; i<nhits; i++) {
	      bool found_neighbor = false;
	      for (int j=0; j<nhits; j++) {
		if (i==j) continue;
		if (abs(col[i]-col[j]) <= 2 && abs(row[i]-row[j]) <=2){ //would be better to use 1 and then make exceptions for masked pixels.
		  if (pixel_cluster_id[j] >= 0 && pixel_cluster_id[i] < 0){
		    pixel_cluster_id[i] = pixel_cluster_id[j];
		    found_neighbor = true;
		    break;
		  }
		}
	      }
	      if (!found_neighbor){
		pixel_cluster_id[i] = k;
		k++;
	      } 
	    }
	    //Now, let's quickly merge clusters that are themselves close enough.
	    bool need_to_merge = false;
	    int merge_counter = 0;
	    for (int i=0; i<nhits; i++) {
	      for (int j=0; j<nhits; j++) {
		if (i==j) continue;
		if (abs(col[i]-col[j]) <= 2 && abs(row[i]-row[j]) <=2){
		  if (pixel_cluster_id[j] != pixel_cluster_id[i]){
		    need_to_merge = true;
		    break;
		  }
		}
	      }
	    }
	    while (need_to_merge){
	      need_to_merge = false;
	      merge_counter++;
	      for (int i=0; i<nhits; i++) {
		for (int j=0; j<nhits; j++) {
		  if (i==j) continue;
		  if (abs(col[i]-col[j]) <= 2 && abs(row[i]-row[j]) <=2){
		    if (pixel_cluster_id[j] != pixel_cluster_id[i]){
		      need_to_merge = true;
		      if (pixel_cluster_id[i] < pixel_cluster_id[j]) pixel_cluster_id[j] = pixel_cluster_id[i];
		      else pixel_cluster_id[i] = pixel_cluster_id[j];
		    }
		  }
		}
	      }
	    }
	    std::vector<int> cluster_ids;
	    for (int i=0; i<nhits; i++) {
	      bool found_alread = false;
	      for (unsigned int j=0; j<cluster_ids.size(); j++){
		if (pixel_cluster_id[i] == cluster_ids[j]) found_alread = true;
	      }
	      if (!found_alread) cluster_ids.push_back(pixel_cluster_id[i]);
	    }	    
	    nclusters = cluster_ids.size();
	    
	    for (unsigned j=0; j<cluster_ids.size(); j++){
	      cluster_tot[j]=0;
	      cluster_size[j]=0;
	      cluster_ncol[j]=0;
	      cluster_nrow[j]=0;
	      cluster_id[j] = cluster_ids[j];
	      std::vector<int> col_vals;
	      std::vector<int> row_vals;
	      for (int i=0; i<nhits; i++) {
		if (pixel_cluster_id[i] == cluster_ids[j]){
		  cluster_tot[j]+=tot[i];
		  cluster_size[j]+=1;
		  bool found_col = false;
		  for (unsigned int k=0; k<col_vals.size(); k++){
		    if (col[i]==col_vals[k]){
		      found_col = true;
		      break;
		    } 
		  }
		  if (!found_col){
		    col_vals.push_back(col[i]);
		    cluster_ncol[j]+=1;
		  }
		  bool found_row = false;
                  for (unsigned int k=0; k<row_vals.size(); k++){
		    if (row[i]==row_vals[k]){
		      found_row= true;
		      break;
                    }
		  }
                  if (!found_row){
                    row_vals.push_back(row[i]);
		    cluster_nrow[j]+=1;
                  }
		}
	      }
	    }

	    t1->Fill();

	    //if (nhits > 3) exit(1);
	    
	}
	
        file.close();
    }
    t1->Write();
    rootfile->Close();

    return 0;
}
