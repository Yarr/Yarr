#include <iostream>
#include <fstream>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <TStyle.h>
#include <TCanvas.h>
#include <TH2F.h>
#include <TPaveStats.h>
#include <THStack.h>
#include <TLegend.h>
#include <TLatex.h>
#include <RD53Style.h>

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
				
				chipnum = file_name.substr(0, file_name.find(delimiter)); //get chip # from file name
				
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
				std::getline(infile, line); //skip line
				std::getline(infile, line);
				std::getline(infile, line);
				std::getline(infile, line);
				std::getline(infile, line); 

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
				h_Syn->GetXaxis()->SetTitle(xaxistitle.c_str());
				h_Syn->GetYaxis()->SetTitle(yaxistitle.c_str());

				TH1 *h_Lin = NULL;
				h_Lin = (TH1*) new TH1F("h_Lin","", xbins, xlow, xhigh);
				h_Lin->GetXaxis()->SetTitle(xaxistitle.c_str());
				h_Lin->GetYaxis()->SetTitle(yaxistitle.c_str());

				TH1 *h_Diff = NULL;
				h_Diff = (TH1*) new TH1F("h_Diff","", xbins, xlow, xhigh);
				h_Diff->GetXaxis()->SetTitle(xaxistitle.c_str());
				h_Diff->GetYaxis()->SetTitle(yaxistitle.c_str());

				int zero_S=0, less98_S=0, less100_S=0, hundred_S=0, less102_S=0, more102_S=0;
				int zero_L=0, less98_L=0, less100_L=0, hundred_L=0, less102_L=0, more102_L=0;
				int zero_D=0, less98_D=0, less100_D=0, hundred_D=0, less102_D=0, more102_D=0;

				for (int i=0; i<rowno; i++) {
					for (int j=0; j<colno; j++) {

						double tmp;
						infile >> tmp;
						//std::cout << i*j << " " << tmp << std::endl;

						if (tmp == 0 ){
							if (j<128) {
								zero_S++;
								h_Syn->SetBinContent(1, zero_S);
							} else if (j>=128 && j<264) {
								zero_L++;	
								h_Lin->SetBinContent(1, zero_L);
							} else if (j>=264) {
								zero_D++;
								h_Diff->SetBinContent(1, zero_D);
							}	
						} else if (tmp > 0  && tmp < 98.0) {
							if (j<128) {
								less98_S++;
								h_Syn->SetBinContent(2, less98_S);
							} else if (j>=128 && j<264) {
								less98_L++;	
								h_Lin->SetBinContent(2, less98_L);
							} else if (j>=264) {
								less98_D++;
								h_Diff->SetBinContent(2, less98_D);
							}	
						} else if (tmp >= 98.0 && tmp < 100) {
							if (j<128) {
								less100_S++;
								h_Syn->SetBinContent(3, less100_S);
							} else if (j>=128 && j<264) {
								less100_L++;	
								h_Lin->SetBinContent(3, less100_L);
							} else if (j>=264) {
								less100_D++;
								h_Diff->SetBinContent(3, less100_D);
							}	
						} else if (tmp == 100) {
							if (j<128) {
								hundred_S++;
								h_Syn->SetBinContent(4, hundred_S);
							} else if (j>=128 && j<264) {
								hundred_L++;	
								h_Lin->SetBinContent(4, hundred_L);
							} else if (j>=264) {
								hundred_D++;
								h_Diff->SetBinContent(4, hundred_D);
							}	
						} else if (tmp > 100 && tmp <= 102.0) {
							if (j<128) {
								less102_S++;
								h_Syn->SetBinContent(5, less102_S);
							} else if (j>=128 && j<264) {
								less102_L++;	
								h_Lin->SetBinContent(5, less102_L);
							} else if (j>=264) {
								less102_D++;
								h_Diff->SetBinContent(5, less102_D);
							}	
						} else if (tmp >= 102.0) {
							if (j<128) {
								more102_S++;
								h_Syn->SetBinContent(6, more102_S);
							} else if (j>=128 && j<264) {
								more102_L++;	
								h_Lin->SetBinContent(6, more102_L);
							} else if (j>=264) {
								more102_D++;
								h_Diff->SetBinContent(6, more102_D);
							}	
						}
					}
				}

				const char *LabelName[6] = {"0%", " 0-98%", " 98-100%", "100%", "100-102%", " >102%"};
				for (int i=1; i<=6; i++) h_Syn->GetXaxis()->SetBinLabel(i,LabelName[i-1]);	
				//Synchronous FE Plot	
				h_Syn->GetXaxis()->LabelsOption("h");
				h_Syn->GetXaxis()->SetLabelSize(0.065);
				h_Syn->GetYaxis()->SetLabelSize(0.045);
				h_Syn->SetFillColor(kRed);
				h_Syn->SetLineColor(kRed);
				h_Syn->SetStats(0);
				h_Syn->GetXaxis()->SetTitleSize(0.045);
				h_Syn->GetXaxis()->SetTitleOffset(1.25);
				h_Syn->GetYaxis()->SetTitleSize(0.05);
				h_Syn->GetYaxis()->SetTitleOffset(1.5);
				TCanvas *c_Syn = new TCanvas("c_Syn", "c_Syn", 800, 600);
				c_Syn->SetLeftMargin(0.15);
				c_Syn->SetRightMargin(0.05);
				c_Syn->SetBottomMargin(0.1225);
				h_Syn->Draw();
				h_Syn->SetMarkerSize(1.8);
				h_Syn->Draw("TEXT0 SAME");
				TLatex *tname= new TLatex();
				tname->SetNDC();
				tname->SetTextAlign(22);
				tname->SetTextFont(73);
				tname->SetTextSizePixels(30);
				tname->DrawLatex(0.21,0.93,"RD53A");
				tname->DrawLatex(0.88, 0.93, chipnum.c_str());
				TLegend *syn_legend = new TLegend(0.7,0.77,0.88,0.88);
				syn_legend->SetHeader("Analog FEs", "C");
				syn_legend->AddEntry(h_Syn, "Synchronous", "f");
				syn_legend->SetBorderSize(0);
				syn_legend->Draw();		
				filename1 = filename.replace(filename.find(".dat"), 12, "_SYNroot.pdf"); 
				c_Syn->Print(filename1.c_str());

				//Linear FE Plot
				for (int i=1; i<=6; i++) h_Lin->GetXaxis()->SetBinLabel(i,LabelName[i-1]);
				h_Lin->GetXaxis()->LabelsOption("h");
				h_Lin->GetXaxis()->SetLabelSize(0.065);
				h_Lin->GetYaxis()->SetLabelSize(0.045);
				h_Lin->SetFillColor(kGreen+2);
				h_Lin->SetLineColor(kGreen+2);
				h_Lin->SetStats(0);
				h_Lin->GetXaxis()->SetTitleSize(0.045);
				h_Lin->GetXaxis()->SetTitleOffset(1.25);
				h_Lin->GetYaxis()->SetTitleSize(0.05);
				h_Lin->GetYaxis()->SetTitleOffset(1.5);
				TCanvas *c_Lin = new TCanvas("c_Lin", "c_Lin", 800, 600);
				c_Lin->SetLeftMargin(0.15);
				c_Lin->SetRightMargin(0.05);
				c_Lin->SetBottomMargin(0.1225);
				h_Lin->Draw();
				h_Lin->SetMarkerSize(1.8);
				h_Lin->Draw("TEXT0 SAME");
				tname->DrawLatex(0.21,0.93,"RD53A");
				tname->DrawLatex(0.88, 0.93, chipnum.c_str());
				TLegend *lin_legend = new TLegend(0.7,0.77,0.86,0.88);
				lin_legend->SetHeader("Analog FEs", "C");
				lin_legend->AddEntry(h_Lin, "Linear", "f");
				lin_legend->SetBorderSize(0);
				lin_legend->Draw();		
				filename2 = filename.replace(filename.find("_SYNroot.pdf"), 12, "_LINroot.pdf"); 
				c_Lin->Print(filename2.c_str());

				//Diff FE Plot
				for (int i=1; i<=6; i++) h_Diff->GetXaxis()->SetBinLabel(i,LabelName[i-1]);
				h_Diff->GetXaxis()->LabelsOption("h");
				h_Diff->GetXaxis()->SetLabelSize(0.065);
				h_Diff->GetYaxis()->SetLabelSize(0.045);
				h_Diff->SetFillColor(kBlue);
				h_Diff->SetLineColor(kBlue);
				h_Diff->SetStats(0);
				h_Diff->GetXaxis()->SetTitleSize(0.045);
				h_Diff->GetXaxis()->SetTitleOffset(1.25);
				h_Diff->GetYaxis()->SetTitleSize(0.05);
				h_Diff->GetYaxis()->SetTitleOffset(1.5);
				TCanvas *c_Diff = new TCanvas("c_Diff", "c_Diff", 800, 600);
				c_Diff->SetLeftMargin(0.15);
				c_Diff->SetRightMargin(0.05);
				c_Diff->SetBottomMargin(0.1225);
				h_Diff->Draw();
				h_Diff->SetMarkerSize(1.8);
				h_Diff->Draw("TEXT0 SAME");
				tname->DrawLatex(0.21,0.93,"RD53A");
				tname->DrawLatex(0.88, 0.93, chipnum.c_str());
				TLegend *diff_legend = new TLegend(0.7,0.77,0.87,0.88);
				diff_legend->SetHeader("Analog FEs", "C");
				diff_legend->AddEntry(h_Diff, "Differential", "f");
				diff_legend->SetBorderSize(0);
				diff_legend->Draw();		
				filename3 = filename.replace(filename.find("_LINroot.pdf"), 13, "_DIFFroot.pdf"); 
				c_Diff->Print(filename3.c_str());

				//Stack Plot for all 3 FEs
				THStack *hs = new THStack("hs","");
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
				hs->GetXaxis()->SetTitle(xaxistitle.c_str());
				hs->GetYaxis()->SetTitle(yaxistitle.c_str());
				hs->GetXaxis()->SetLabelSize(0.065);
				hs->GetYaxis()->SetLabelSize(0.045);
				hs->GetXaxis()->SetTitleSize(0.045);
				hs->GetXaxis()->SetTitleOffset(1.25);
				hs->GetYaxis()->SetTitleSize(0.05);
				hs->GetYaxis()->SetTitleOffset(1.5);
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
				tname->DrawLatex(0.88, 0.93, chipnum.c_str());

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
