#include <iostream>
#include <fstream>
#include <array>

#include <TStyle.h>
#include <TCanvas.h>
#include <TH2F.h>
#include <TPaveStats.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cout << "No file given!" << std::endl;
        return -1;
    }

    std::array<std::array<unsigned, 8>, 8> mask;
    mask[0] = {{0, 1, 1, 1, 0, 0, 0, 0}};
    mask[1] = {{0, 0, 1, 1, 1, 0, 0, 0}};
    mask[2] = {{0, 1, 0, 1, 1, 0, 0, 0}};
    mask[3] = {{0, 1, 0, 1, 1, 0, 0, 0}};
    mask[4] = {{0, 0, 1, 1, 0, 0, 0, 0}};
    mask[5] = {{0, 0, 1, 1, 0, 0, 0, 0}};
    mask[6] = {{0, 0, 1, 0, 0, 0, 0, 0}};
    mask[7] = {{0, 0, 1, 1, 0, 0, 0, 0}};

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

        type = type.substr(0,7); // EOL char kept by getline()

        if (type == "Histo2d") {
            std::cout << " ... detecting 2d histogram" << std::endl;
            infile >> ybins >> ylow >> yhigh;
        } else {
            ybins = 1;
            std::cout << "Not a 2D histogram!" << std::endl;
            return -1;
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

        gStyle->SetOptFit(1);
        TCanvas *c = new TCanvas("c", "c", 800, 600);
        TH2F *h = new TH2F(name.c_str(), name.c_str(), xbins, xlow, xhigh, ybins, ylow, yhigh);
        h->SetStats(0);
        h->GetXaxis()->SetTitle(xaxistitle.c_str());
        h->GetYaxis()->SetTitle(yaxistitle.c_str());
        h->GetZaxis()->SetTitle(zaxistitle.c_str());
        c->SetRightMargin(0.2);
        TH1F *dist_good = new TH1F("dist_good", "dist_good", 500, 5, 1505); 
        dist_good->SetTitle("Diff FE Good Pixels");
        dist_good->GetXaxis()->SetTitle("Threshold [e]");
        dist_good->GetYaxis()->SetTitle("# of Pixels");
        dist_good->SetFillColor(kBlue);
        TH1F *dist_bad = new TH1F("dist_bad", "dist_bad", 500, 5, 1505); 
        dist_bad->SetTitle("Diff FE Bad Pixels");
        dist_bad->GetXaxis()->SetTitle("Threshold [e]");
        dist_bad->GetYaxis()->SetTitle("# of Pixels");
        dist_bad->SetFillColor(kBlue);
        for (int i=0; i<ybins; i++) {
            for (int j=0; j<xbins; j++) {
                double tmp;
                infile >> tmp;
                std::cout << i << " " << j << " " << tmp << std::endl;
                h->SetBinContent(j+1, i+1, tmp);
                if (mask[i%8][j%8] == 1 && tmp > 0) {
                    dist_good->Fill(tmp);
                } else {
                    dist_bad->Fill(tmp);
                }
            }
        }
        dist_good->Fit("gaus");
        dist_good->Draw("");
        c->Print("dist_good.pdf");
        dist_bad->Fit("gaus");
        dist_bad->Draw("");
        c->Print("dist_bad.pdf");
        delete h;
        delete c;
    }

    return 0;
}
