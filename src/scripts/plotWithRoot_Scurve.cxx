#include <iostream>
#include <fstream>
#include <vector>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <TStyle.h>
#include <TF1.h>
#include <plotWithRoot.h>
#include <RD53Style.h>

int main(int argc, char *argv[]) { //./plotWithRoot_Scurve path/to/directory file_ext
	//Example file extensions: png, pdf, C, root
	
	SetRD53Style();
	gStyle->SetTickLength(0.02);
	gStyle->SetTextFont();

	if (argc < 3) {
		std::cout << "No directory and/or image plot extension given! \nExample: ./plotWithRoot_Scurve path/to/directory/ pdf" << std::endl;
		return -1;
	}

	std::string dir, filepath, file_name, chipnum;
	DIR *dp;
	struct dirent *dirp;
	struct stat filestat;

	std::string delimiter = "_";

	dp = opendir(argv[1]);	//open directory
	if (dp==NULL) {	//if directory doesn't exist
		std::cout << "Directory not found. " << std::endl;
		return -1;
	}

	dir = argv[1];

	while ((dirp = readdir(dp))) { //pointer to structure representing directory entry at current position in directory stream, and positions directory stream at the next entry. Returns a null pointer at the end of the directory stream.

		file_name = dirp->d_name;
		filepath = dir + "/" + dirp->d_name;
		const char *file_path = filepath.c_str();		

		if (stat(filepath.c_str(), &filestat)) continue; //skip if file is invalid
		if (S_ISDIR(filestat.st_mode)) continue; //skip if file is a directory

		if ( strstr( file_path, "sCurve.dat") != NULL) { //if filename contains string declared in argument.
				chipnum = "Chip SN: " + file_name.substr(0, file_name.find(delimiter)); //get chip # from file name

				std::cout << "Opening file: " << filepath.c_str() << std::endl;
				std::string filename = filepath.c_str();
				std::fstream infile(filepath.c_str(), std::ios::in);

				std::string type;
				std::string name;
				std::string xaxistitle, yaxistitle, zaxistitle;

				int xbins, ybins;
				double xlow, ylow;
				double xhigh, yhigh;
				int underflow, overflow;

				std::string filename1;

				std::getline(infile, type);
				std::getline(infile, name);
				std::getline(infile, xaxistitle);
				std::getline(infile, yaxistitle);
				std::getline(infile, zaxistitle);
				infile >> xbins >> xlow >> xhigh;
				infile >> ybins >> ylow >> yhigh;

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

				//Create plot
				TH2F *h_plot = NULL;
				h_plot = new TH2F("h_plot", "", xbins, xlow, xhigh, ybins, ylow, yhigh); 
				//Fill S-curve plot	
				for (int i=0; i<ybins; i++) {
					for (int j=0; j<xbins; j++) {
						double tmp;
						infile >> tmp;
						if (tmp != 0) h_plot->SetBinContent(j+1,i+1,tmp);	
					}
				}
		

				std::string rd53 = "RD53A Internal";
	
				//S-curve plot
				style_TH2(h_plot, xaxistitle.c_str(), yaxistitle.c_str(), zaxistitle.c_str()); 
				TCanvas *c_plot = new TCanvas("c_plot", "c_plot", 800, 600);
				style_TH2canvas(c_plot);
				gPad->SetLogz(1);
				h_plot->Draw("colz");
				h_plot->GetXaxis()->SetLabelSize(0.04);
				h_plot->GetYaxis()->SetLabelSize(0.04);
				
				//Write RD53A and Chip ID
				TLatex *tname= new TLatex();
				tname->SetNDC();
				tname->SetTextAlign(22);
				tname->SetTextFont(73);
				tname->SetTextSizePixels(30);
				tname->DrawLatex(0.23,0.96, rd53.c_str());
				tname->DrawLatex(0.72, 0.96, chipnum.c_str());
				
				gStyle->SetOptStat(0);
				c_plot->RedrawAxis();
				c_plot->Update();
				h_plot->GetZaxis()->SetLabelSize(0.04);
				std::string plot_ext = "_PLOT.";	
				filename1=filename.replace(filename.find(".dat"), 10, plot_ext.append(argv[2]));				
				c_plot->Print(filename1.c_str());

		}
	}

	return 0;
} 
