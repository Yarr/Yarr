#include <iostream>
#include <fstream>

#include <TStyle.h>
#include <TCanvas.h>
#include <TH2F.h>
#include <TPaveStats.h>
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

		std::string xaxistitle, yaxistitle;
		std::string line;	

		int xbins, ybins;
		double xlow; 
		double xhigh;
		int underflow, overflow;
		int rowno, colno;

		std::getline(infile, type);
		std::getline(infile, name);
		std::getline(infile, line); //skip line
		std::getline(infile, line);
		std::getline(infile, line);
		std::getline(infile, line);
		std::getline(infile, line); 

		rowno = 192;
		colno = 400;
		xaxistitle = "Occupancy (%)";
		yaxistitle = "Number of Pixels";
		xbins = 6;
		xlow = -0.5;
		xhigh = 5.5;	

		infile >> underflow >> overflow;

		std::cout << "Histogram type: " << type << std::endl;
		std::cout << "Histogram name: " << name << std::endl;
		std::cout << "X axis title: " << xaxistitle << std::endl;
		std::cout << "Y axis title: " << yaxistitle << std::endl;

		if (!infile) {
			std::cout << "Something wrong with file ..." << std::endl;
			return -1;
		}

		TCanvas *c = new TCanvas("c", "c", 800, 600);
		TH1 *h = NULL;
		h = (TH1*) new TH1F(name.c_str(), "", xbins, xlow, xhigh);
		h->SetTitle("Occupancy % Range");
		h->GetXaxis()->SetTitle(xaxistitle.c_str());
		h->GetYaxis()->SetTitle(yaxistitle.c_str());

		int zero=0, less98=0, less100=0, hundred=0, less102=0, more102=0;
		for (int i=0; i<rowno; i++) {
			for (int j=0; j<(colno); j++) {

				if(i==192) {
					
				}
				double tmp;
				infile >> tmp;
				std::cout << i*j << " " << tmp << std::endl;
				if (tmp == 0.0){
					zero++;
					h->SetBinContent(0,zero);
				} else if (tmp < 98.0) {
					less98++;
					h->SetBinContent(1,less98);
				} else if (tmp < 100.0) {
					less100++;
					h->SetBinContent(2,less100);
				} else if (tmp == 100.0) {
					hundred++;
					h->SetBinContent(3,hundred);
				} else if (tmp < 102.0) {
					less102++;
					h->SetBinContent(4,less102);
				} else if (tmp >= 102.0) {
					more102++;
					h->SetBinContent(5,more102);
				}
			}
		}
		const char *LabelName[6] = {"0%", "0-98%", "98-100%", "100%", "100-102%", ">102%"};
		for (int i=1; i<=6; i++) h->GetXaxis()->SetBinLabel(i,LabelName[i-1]);
		h->GetXaxis()->LabelsOption("h");
		h->SetFillColor(kBlue);
		h->SetLineColor(kBlue);
		h->SetStats(0);
		h->GetXaxis()->SetTitleSize(0.05);
		h->GetXaxis()->SetTitleOffset(0.8);
		h->GetYaxis()->SetTitleSize(0.05);
		h->GetYaxis()->SetTitleOffset(0.8);
		//gStyle->SetOptFit(1);
		//h->Fit("gaus", "", "", 800, 1200);
		//h->Draw();
		//gStyle->SetStatY(0.8);
		//gStyle->SetStatX(0.5);
		h->Draw();
		//      }

		filename.replace(filename.find(".dat"), 4, "_occroot.pdf"); 

		c->Print(filename.c_str());
		delete h;
		delete c;
}

return 0;
}  
