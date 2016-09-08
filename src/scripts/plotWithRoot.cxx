#include <iostream>
#include <fstream>

#include <TCanvas.h>
#include <TH2F.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cout << "No file given!" << std::endl;
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

        infile >> type >> name;
        infile >> xaxistitle >> yaxistitle >> zaxistitle;
        infile >> xbins >> xlow >> xhigh;

        if (type == "Histo2d") {
            infile >> ybins >> ylow >> yhigh;
        }

        infile >> underflow >> overflow;

        if (!infile) {
            std::cout << "Something wrong with file ..." << std::endl;
            return -1;
        }

        std::cout << "Histogram type: " << type << std::endl;
        std::cout << "Histogram name: " << name << std::endl;
        std::cout << "X axis title: " << xaxistitle << std::endl;
        std::cout << "Y axis title: " << yaxistitle << std::endl;
        std::cout << "Z axis title: " << zaxistitle << std::endl;


        TCanvas *c = new TCanvas("c", "c", 800, 600);
        TH1 *h;
        if (type == "Histo2d") {
            h = (TH1*) new TH2F(name.c_str(), name.c_str(), xbins, xlow, xhigh, ybins, ylow, yhigh);
            h->SetStats(0);
            for (unsigned i=0; i<ybins; i++) {
                for (unsigned j=0; j<xbins; j++) {
                    double tmp;
                    infile >> tmp;
                    h->SetBinContent(j+1, i+1, tmp);
                }
            }
            h->SetMaximum(h->GetMean()*4);
            h->Draw("colz");
        }
        filename.replace(filename.find(".dat"), 4, "_root.pdf"); 

        c->Print(filename.c_str());
        delete h;
        delete c;
    }

    return 0;
}
