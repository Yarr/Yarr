//plots a Vth1 heat map with a file created by DiffFE_Vth1Map_multifile
//(from src)  runs with ./scripts/plotWithRoot_DiffFE_Vth1.cxx (filename)

#include <iostream>
#include <fstream>
#include <TStyle.h>
#include <TCanvas.h>
#include <TH2F.h>
#include <TPaveStats.h>
#include <plotWithRoot_DiffFE.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cout << "No file given!" << argc << std::endl;
        return -1;
    }

    for (int n=1; n<argc; n++) {
        std::cout << "Opening file: " << argv[n] << std::endl;
        std::string filename = argv[n];
        std::fstream infile(argv[n], std::ios::in);

        std::string type;
        std::string name;

        std::string xaxistitle, yaxistitle, zaxistitle;

        int xbins, ybins;
        double xlow, ylow;
        double xhigh, yhigh;
        int underflow, overflow;

        std::getline(infile, type);
        std::getline(infile, name);
        std::getline(infile, xaxistitle);
        std::getline(infile, yaxistitle);
        std::getline(infile, zaxistitle);
        infile >> xbins >> xlow >> xhigh;
	std::cout << " xbins " << xbins << " xlow" << xlow << " xhigh" << xhigh << std::endl;

        type = type.substr(0,7); // EOL char kept by getline()

        if (type == "Histo2d") {
            std::cout << " ... detecting 2d histogram" << std::endl;
            infile >> ybins >> ylow >> yhigh;
	std::cout << " ybins " << ybins << " ylow" << ylow << " yhigh" << yhigh << std::endl;
        } else {
            ybins = 1;
        }

        infile >> underflow >> overflow;

        std::cout << "Histogram type: " << type << std::endl;
        std::cout << "Histogram name: " << name << std::endl;
        std::cout << "X axis title: " << xaxistitle << std::endl;
        std::cout << "Y axis title: " << yaxistitle << std::endl;
        std::cout << "Z axis title: " << zaxistitle << std::endl;

        if (!infile) {
            std::cout << "Something wrong with file ..." << std::endl;
            return -1;
        }

        TCanvas *c = new TCanvas("c", "c", 800, 600);
        TH1 *h = NULL;
        if (type == "Histo2d") {
            h = (TH1*) new TH2F(name.c_str(), name.c_str(), 136, 264, xhigh, ybins, ylow, yhigh);
            h->SetStats(0);
            h->GetXaxis()->SetTitle(xaxistitle.c_str());
            h->GetYaxis()->SetTitle(yaxistitle.c_str());
            h->GetZaxis()->SetTitle(zaxistitle.c_str());
            c->SetRightMargin(0.2);
            for (int i=0; i<ybins; i++) {
                for (int j=0; j<136; j++) {
                    double tmp;
                         infile >> tmp;	
                    std::cout << i << " " << j << " " << tmp << std::endl;
                    h->SetBinContent(j+1, i+1, tmp*(goodDiff(i, j) == 1)); //applies good pixel mask
                }
            }
            
	  h->GetXaxis()->SetRangeUser(264,401);  //sets X axis range (264 for Diff FE only)
	   h->GetYaxis()->SetRangeUser(ylow,193);  //sets Y axis range
	h->GetZaxis()->SetRangeUser(70,150);	
            h->Draw("colz");
        } 
        if (type == "Histo1d") {
            h = (TH1*) new TH1F(name.c_str(), "", xbins, xlow, xhigh);
            h->GetXaxis()->SetTitle(xaxistitle.c_str());
            h->GetYaxis()->SetTitle(yaxistitle.c_str());
		
            for (int j=0; j<xbins; j++) {
                double tmp;
                infile >> tmp;
                std::cout << j << " " << tmp << std::endl;
                h->SetBinContent(j+1,tmp);
            }
            h->SetFillColor(kBlue);
            h->SetLineColor(kBlue);
            h->SetStats(0);
            h->GetXaxis()->SetTitleSize(0.05);
            h->GetXaxis()->SetTitleOffset(0.8);
            h->GetYaxis()->SetTitleSize(0.05);
            h->GetYaxis()->SetTitleOffset(0.8);
            h->Draw();
        }

        filename.replace(filename.find(".txt"), 4, "_root.pdf"); 

        c->Print(filename.c_str());
        delete h;
        delete c;
    }

    return 0;
}
