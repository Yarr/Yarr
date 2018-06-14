#include <iostream>
#include <fstream>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <plotWithRoot.h>


int main(int argc, char *argv[]) { //./plotWithRoot_Occupancydir Directory_name
	if (argc < 2) {
		std::cout << "No directory given!" << std::endl;
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

		if ( strstr( file_path, "OccupancyMap") != NULL) { //if filename contains string declared in argument.
			if (strstr(file_path, ".dat") != NULL) {

				chipnum = "Chip SN: " + file_name.substr(0, file_name.find(delimiter)); //get chip # from file name

				std::cout << "Opening file: " << filepath.c_str() << std::endl;
				std::string filename = filepath.c_str();
				std::fstream infile(filepath.c_str(), std::ios::in);

				std::string type;
				std::string name;

				std::string xaxistitle, yaxistitle;
				std::string line;	

				std::string filename1, filename2, filename3, filename4;

				int xbins;
				double xlow; 
				double xhigh;
				int underflow, overflow;
				int rowno, colno;

				std::getline(infile, type);
				std::getline(infile, name);
				std::getline(infile, line); //skip "Column"
				std::getline(infile, line); //skip "Rows"
				std::getline(infile, line); //skip "Hits"
				std::getline(infile, line); //skip x range
				std::getline(infile, line); //skip y range

				rowno = 192;
				colno = 400;
				xaxistitle = "Occupancy [%]";
				yaxistitle = "Number of Pixels";
				xbins = 6;
				xlow = 0.5;
				xhigh = 6.5;	

				infile >> underflow >> overflow;

				std::cout << "Histogram type: " << type << std::endl;
				std::cout << "Histogram name: " << name << std::endl;
				std::cout << "X axis title: " << xaxistitle << std::endl;
				std::cout << "Y axis title: " << yaxistitle << std::endl;

				if (!infile) {
					std::cout << "Something wrong with file ..." << std::endl;
					return -1;
				}


				TH1 *h_Syn = NULL;
				h_Syn = (TH1*) new TH1F("h_Syn", "", xbins, xlow, xhigh);

				TH1 *h_Lin = NULL;
				h_Lin = (TH1*) new TH1F("h_Lin","", xbins, xlow, xhigh);

				TH1 *h_Diff = NULL;
				h_Diff = (TH1*) new TH1F("h_Diff","", xbins, xlow, xhigh);

				THStack *hs = new THStack("hs","");

				TH1* fe_hist[3] = {h_Syn, h_Lin, h_Diff};
				const char *LabelName[6] = {"0%", " 0-98%", " 98-100%", "100%", "100-102%", " >102%"};

				for (int i=0; i<rowno; i++) {
					for (int j=0; j<colno; j++) {

						double tmp;
						infile >> tmp;
						//std::cout << i*j << " " << tmp << std::endl;

						double inj_value = 100;
						int bin_num = whichBin(inj_value, tmp);
						fe_hist[whichFE(j)]->AddBinContent(bin_num);
					}
				}

				for(int i=0; i<3; i++){
					style_TH1(fe_hist[i], xaxistitle.c_str(), yaxistitle.c_str());
					for (int j=1; j<=xbins; j++) fe_hist[i]->GetXaxis()->SetBinLabel(j,LabelName[j-1]);
					fe_hist[i]->GetXaxis()->LabelsOption("h");
					fe_hist[i]->GetXaxis()->SetLabelSize(0.065);
					fe_hist[i]->GetYaxis()->SetLabelSize(0.045);

				}

				//Synchronous FE Plot	
				h_Syn->SetFillColor(kRed);
				h_Syn->SetLineColor(kRed);
				TCanvas *c_Syn = new TCanvas("c_Syn", "c_Syn", 800, 600);
				style_TH1canvas(c_Syn);
				h_Syn->Draw();
				h_Syn->SetMarkerSize(1.8);
				h_Syn->Draw("TEXT0 SAME");
				TLatex *tname= new TLatex();
				latex_Chip(tname);
				tname->DrawLatex(0.21,0.93,"RD53A");
				tname->DrawLatex(0.8, 0.93, chipnum.c_str());
				TLegend *syn_legend = new TLegend(0.7,0.77,0.88,0.88);
				syn_legend->SetHeader("Analog FEs", "C");
				syn_legend->AddEntry(h_Syn, "Synchronous", "f");
				syn_legend->SetBorderSize(0);
				syn_legend->Draw();		
				filename1 = filename.replace(filename.find(".dat"), 12, "_SYNroot.pdf"); 
				c_Syn->Print(filename1.c_str());

				//Linear FE Plot
				h_Lin->SetFillColor(kGreen+2);
				h_Lin->SetLineColor(kGreen+2);
				TCanvas *c_Lin = new TCanvas("c_Lin", "c_Lin", 800, 600);
				style_TH1canvas(c_Lin);
				h_Lin->Draw();
				h_Lin->SetMarkerSize(1.8);
				h_Lin->Draw("TEXT0 SAME");
				tname->DrawLatex(0.21,0.93,"RD53A");
				tname->DrawLatex(0.8, 0.93, chipnum.c_str());
				TLegend *lin_legend = new TLegend(0.7,0.77,0.86,0.88);
				lin_legend->SetHeader("Analog FEs", "C");
				lin_legend->AddEntry(h_Lin, "Linear", "f");
				lin_legend->SetBorderSize(0);
				lin_legend->Draw();		
				filename2 = filename.replace(filename.find("_SYNroot.pdf"), 12, "_LINroot.pdf"); 
				c_Lin->Print(filename2.c_str());

				//Diff FE Plot
				h_Diff->SetFillColor(kBlue);
				h_Diff->SetLineColor(kBlue);
				TCanvas *c_Diff = new TCanvas("c_Diff", "c_Diff", 800, 600);
				style_TH1canvas(c_Diff);
				h_Diff->Draw();
				h_Diff->SetMarkerSize(1.8);
				h_Diff->Draw("TEXT0 SAME");
				tname->DrawLatex(0.21,0.93,"RD53A");
				tname->DrawLatex(0.8, 0.93, chipnum.c_str());
				TLegend *diff_legend = new TLegend(0.7,0.77,0.87,0.88);
				diff_legend->SetHeader("Analog FEs", "C");
				diff_legend->AddEntry(h_Diff, "Differential", "f");
				diff_legend->SetBorderSize(0);
				diff_legend->Draw();		
				filename3 = filename.replace(filename.find("_LINroot.pdf"), 13, "_DIFFroot.pdf"); 
				c_Diff->Print(filename3.c_str());

				//Stack Plot for all 3 FEs
				hs->Add(h_Syn);
				hs->Add(h_Lin);
				hs->Add(h_Diff);
				TCanvas *c_Stack = new TCanvas("c_Stack", "c_Stack", 800, 600);
				c_Stack->SetLeftMargin(0.15);
				c_Stack->SetRightMargin(0.05);
				c_Stack->SetBottomMargin(0.1225);
				hs->Draw(); 
				//hs->Draw("TEXt0 SAME"); //Works, but the text layers on top of each other if the values are too low
				//Setting the title for THStack plots has to be after Draw(), and needs canvas->Modified() after
				style_THStack(hs, xaxistitle.c_str(), yaxistitle.c_str());	
				c_Stack->Modified();
				gStyle->SetOptStat(0);
				TLegend *stack_legend = new TLegend(0.7,0.65,0.88,0.88);
				stack_legend->SetHeader("Analog FEs", "C");
				stack_legend->AddEntry(h_Syn, "Synchronous", "f");
				stack_legend->AddEntry(h_Lin, "Linear", "f");
				stack_legend->AddEntry(h_Diff, "Differential", "f");
				stack_legend->SetBorderSize(0);
				stack_legend->Draw();
				for (int i=1; i<=6; i++) hs->GetXaxis()->SetBinLabel(i,LabelName[i-1]);
				hs->GetXaxis()->LabelsOption("h");	
				hs->GetXaxis()->SetLabelSize(0.05);
				hs->GetYaxis()->SetLabelSize(0.03);
				tname->DrawLatex(0.21,0.93,"RD53A");
				tname->DrawLatex(0.8, 0.93, chipnum.c_str());

				filename4 = filename.replace(filename.find("_DIFFroot.pdf"), 14, "_STACKroot.pdf");
				c_Stack->Print(filename4.c_str());

				delete h_Syn;
				delete c_Syn;
				delete h_Lin;
				delete c_Lin;
				delete h_Diff;
				delete c_Diff;
				delete hs;
				delete c_Stack;
			}	
		}
	}

	return 0;
} 
