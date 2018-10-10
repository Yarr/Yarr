#include <iostream>
#include <fstream>
#include <vector>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <TStyle.h>
#include <TF1.h>
#include <TMarker.h>

#include <plotWithRoot.h>
#include <RD53Style.h>

int main(int argc, char *argv[]) { //./plotWithRoot_Noise path/to/directory file_ext
	//Example file extensions: png, pdf, C, root	

	SetRD53Style();
	gStyle->SetTickLength(0.02);
	gStyle->SetTextFont();

	if (argc < 3) {
		std::cout << "No directory and/or image plot extension given! \nExample: ./plotWithRoot_Noise path/to/directory/ pdf" << std::endl;
		return -1;
	}

	std::string dir, filepath, file_name, chipnum;
	DIR *dp;
	struct dirent *dirp;
	struct stat filestat;

	std::string delimiter = "_";
	std::string ext = argv[2];

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

		if ( strstr(file_path, "NoiseOccupancy.dat") != NULL) { //if filename contains string declared in argument.

			chipnum = "Chip SN: " + file_name.substr(0, file_name.find(delimiter)); //get chip # from file name

			std::cout << "Opening file: " << filepath.c_str() << std::endl;
			std::string filename = filepath.c_str();
			std::fstream infile(filepath.c_str(), std::ios::in);

			std::string type;
			std::string name;

			std::string xaxistitle, yaxistitle, zaxistitle;
			std::string line;	

			std::string filename1;

			std::getline(infile, type);
			std::getline(infile, name);
			std::getline(infile, line); //skip "Column"
			std::getline(infile, line); //skip "Rows"
			std::getline(infile, line); //skip "Hits"
			std::getline(infile, line); //skip x range
			std::getline(infile, line); // skip y range

			int underflow, overflow;
			int rowno, colno;

			rowno = 192;
			colno = 400;
			xaxistitle = "Column";
			yaxistitle = "Row";
			zaxistitle = "Noise Occupancy (Hits/bc)";

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

			//Create histogram
			TH2F *h_plot = NULL;
			h_plot = new TH2F("h_plot", "", colno, 0, colno, rowno, 0, rowno); 

			int zero_counter = 0;
			//Fill Noise plot	
			for (int i=0; i<rowno; i++) {
				for (int j=0; j<colno; j++) {

					double tmp;
					infile >> tmp;
					h_plot->SetBinContent(j+1,i+1,tmp);	
					if (tmp == 0) zero_counter++;
				}
			}


			//For RD53A and Chip ID labels.
			TLatex *tname= new TLatex();
			tname->SetNDC();
			tname->SetTextAlign(22);
			tname->SetTextFont(73);
			tname->SetTextSizePixels(30);
			std::string rd53 = "RD53A Internal";

			//Map of Pixels
			style_TH2(h_plot, xaxistitle.c_str(), yaxistitle.c_str(), zaxistitle.c_str()); 
			TCanvas *c_plot = new TCanvas("c_plot", "c_plot", 800, 600);
			style_TH2canvas(c_plot);
			h_plot->Draw("colz");
			h_plot->GetXaxis()->SetLabelSize(0.04);
			h_plot->GetYaxis()->SetLabelSize(0.04);
			tname->DrawLatex(0.23,0.96, rd53.c_str());
			tname->DrawLatex(0.72, 0.96, chipnum.c_str());
			gStyle->SetOptStat(0);
			c_plot->RedrawAxis();
			c_plot->Update();
			h_plot->GetZaxis()->SetLabelSize(0.04);

			//If <= 25% of pixels have value= 0, draw a red circle around them.
			if ( zero_counter <= rowno*colno*0.25) {
				for (int j=0; j<colno; j++) {
					for (int k=0; k<rowno; k++) {
						if (h_plot->GetBinContent(j+1,k+1) == 0) {
							TMarker *m = new TMarker(j+1,k+1,0);
							m->SetMarkerSize(0.5);
							m->SetMarkerColor(2);
							m->SetMarkerStyle(kCircle);
							m->Draw();
						}
					}
				}
			}

			std::string plot_ext = "_PLOT.";
			filename1 = filename.replace(filename.find(".dat"), 10, plot_ext.append(ext));
			c_plot->Print(filename1.c_str());

			delete h_plot;
			delete c_plot;

		}
	}

	return 0;
} 
