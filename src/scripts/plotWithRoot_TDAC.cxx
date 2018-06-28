#include <iostream>
#include <fstream>
#include <vector>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <TStyle.h>
#include <TF1.h>
#include <TGaxis.h>
#include <plotWithRoot.h>
#include "json.hpp"

using json=nlohmann::basic_json<std::map, std::vector, std::string, bool, std::int32_t, std::uint32_t, float>;

int main(int argc, char *argv[]) { //./plotWithRoot_TDAC Directory_name
	if (argc < 2) {
		std::cout << "No directory given!" << std::endl;
		return -1;
	}

	std::string dir, filepath, file_name, chipnum;
	DIR *dp;
	struct dirent *dirp;
	struct stat filestat;

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
		if (strstr(file_path,".pdf")) continue;

		if ( strstr( file_path, ".json") != NULL) { //if filename contains string declared in argument.

				std::cout << "Opening file: " << filepath.c_str() << std::endl;
				std::string filename = filepath.c_str();
				std::fstream cfgfile(filepath.c_str(), std::ios::in);
				json cfg;
				cfgfile >> cfg;

				std::string type;
				std::string name;

				std::string xaxistitle, yaxistitle, xrangetitle;
				std::string chipnumber;	

				std::string filename1, filename2, filename3, filename4, filename5;


				int xbins, range_bins;
				double xlow, xhigh, range_low, range_high, xyzero; 
				int rowno, colno;
				char mean_Lin[100]={}, mean_Diff[100]={};
				char rms_Lin[100]={}, rms_Diff[100]={};
				double mean_hLin, mean_hDiff;
				double rms_hLin, rms_hDiff;
				char zeros_Lin[100]={}, zeros_Diff[100]={};
				const char *LabelName[6] = {"1","2","3","4","5",">5"};	
			
				rowno = 192;
				colno = 400;
				xaxistitle = "TDAC";
				yaxistitle = "Number of pixels";
				xrangetitle = "Deviation from the Mean [RMS] ";
				xbins = 31;
				range_bins = 6;
				xlow = -15.5;
				xhigh = 15.5;
				xyzero = 0;
				range_low = 0;
				range_high = 6;

				std::cout << "Histogram type: " << type << std::endl;
				std::cout << "Histogram name: " << name << std::endl;
				std::cout << "X axis title: " << xaxistitle << std::endl;
				std::cout << "Y axis title: " << yaxistitle << std::endl;
				
				chipnum = "Chip SN: ";
				chipnumber = cfg["RD53A"]["Parameter"]["Name"];
				chipnum.append(chipnumber);

				if (!cfgfile) {
					std::cout << "Something wrong with file ..." << std::endl;
					return -1;
				}
			
				TH1 *h_Lin = NULL;
				h_Lin = (TH1*) new TH1F("h_Lin","", xbins, xyzero, xhigh);

				TH1 *h_Diff = NULL;
				h_Diff = (TH1*) new TH1F("h_Diff","", xbins, xlow, xhigh);

				TH1 *h_range_Lin = NULL;
				h_range_Lin = (TH1*) new TH1F("h_range_Lin","", range_bins, xyzero, range_high);

				TH1 *h_range_Diff = NULL;
				h_range_Diff = (TH1*) new TH1F("h_range_Diff","", range_bins, range_low, range_high);
				
				TH2F *h_plot = NULL;
				h_plot = new TH2F("h_plot", "", colno, xyzero, colno, rowno, xyzero, rowno); 
			
					
				TH1* fe_hist[2] = {h_Lin, h_Diff};
				TH1* range_hist[2] = {h_range_Lin, h_range_Diff};
				std::vector <double> pix_values;

				//Fill Threshold plots	
				for (int i=0; i<rowno; i++) {
					for (int j=0; j<colno; j++) {
						int tmp;
						tmp = cfg["RD53A"]["PixelConfig"][j]["TDAC"][i];
						//std::cout << i*j << " " << tmp << std::endl;
						pix_values.push_back(tmp);
						if (whichFE(j) != 0) fe_hist[whichFE(j)-1]->Fill(tmp);
						h_plot->SetBinContent(j+1,i+1,tmp);
							
					}
				}

				for (int i=0; i<2; i++) {
					style_TH1(fe_hist[i], xaxistitle.c_str(), yaxistitle.c_str());
					style_TH1(range_hist[i], xrangetitle.c_str(), yaxistitle.c_str());
					for (int j=1; j<=range_bins; j++) range_hist[i]->GetXaxis()->SetBinLabel(j, LabelName[j-1]);
					range_hist[i]->GetXaxis()->LabelsOption("h");
					range_hist[i]->GetXaxis()->SetLabelSize(0.065);
					range_hist[i]->GetYaxis()->SetLabelSize(0.045);
				}
	
				//Linear FE Plot
				h_Lin->SetFillColor(kSpring+4);
				h_Lin->SetLineColor(kSpring+4);
				h_Lin->SetStats(0);
				TCanvas *c_Lin = new TCanvas("c_Lin", "c_Lin", 800, 600);
				style_TH1canvas(c_Lin);
				h_Lin->Draw();
				TLatex *tname= new TLatex();
				tname->SetNDC();
				tname->SetTextAlign(22);
				tname->SetTextFont(73);
				tname->SetTextSizePixels(30);
				tname->DrawLatex(0.21,0.93,"RD53A");
				tname->DrawLatex(0.8, 0.93, chipnum.c_str());
				TLegend *lin_legend = new TLegend(0.75,0.77,0.93,0.88);
				lin_legend->SetHeader("Analog FEs", "C");
				lin_legend->AddEntry(h_Lin, "Linear", "f");
				lin_legend->SetBorderSize(0);
				lin_legend->Draw();	
				mean_hLin = h_Lin->GetMean();
				rms_hLin = h_Lin->GetRMS();

				TLatex *mean_rms = new TLatex;
				mean_rms->SetNDC();
				mean_rms->SetTextAlign(13);
				mean_rms->SetTextFont(63);
				mean_rms->SetTextSizePixels(24);
				sprintf(mean_Lin, "Mean = %.1f #pm %.1f", h_Lin->GetMean(), h_Lin->GetMeanError());
				mean_rms->DrawLatex(0.18,0.88, mean_Lin);
				sprintf(rms_Lin, "RMS = %.1f #pm %.1f", h_Lin->GetRMS(), h_Lin->GetRMSError());
				mean_rms->DrawLatex(0.18,0.84, rms_Lin);

				//h_Lin->GetXaxis()->SetRangeUser(mean_hLin - 3*rms_hLin, mean_hLin + 3*rms_hLin);
				h_Lin->SetMaximum((h_Lin->GetMaximum())*1.25);
				c_Lin->Update();				
	
				filename1 = filename.append("_LINTdac.pdf"); 
				c_Lin->Print(filename1.c_str());

				//Diff FE Plot
				h_Diff->SetFillColor(kAzure+5);
				h_Diff->SetLineColor(kAzure+5);
				h_Diff->SetStats(0);
				TCanvas *c_Diff = new TCanvas("c_Diff", "c_Diff", 800, 600);
				style_TH1canvas(c_Diff);
				h_Diff->Draw();
				tname->DrawLatex(0.21,0.93,"RD53A");
				tname->DrawLatex(0.8, 0.93, chipnum.c_str());
				TLegend *diff_legend = new TLegend(0.75,0.77,0.93,0.88);
				diff_legend->SetHeader("Analog FEs", "C");
				diff_legend->AddEntry(h_Diff, "Differential", "f");
				diff_legend->SetBorderSize(0);
				diff_legend->Draw();

				mean_hDiff = h_Diff->GetMean();
				rms_hDiff = h_Diff->GetRMS();
				
				sprintf(mean_Diff, "Mean = %.1f #pm %.1f", h_Diff->GetMean(), h_Diff->GetMeanError());
				mean_rms->DrawLatex(0.18,0.88, mean_Diff);
				sprintf(rms_Diff, "RMS = %.1f #pm %.1f", h_Diff->GetRMS(), h_Diff->GetRMSError());
				mean_rms->DrawLatex(0.18,0.84, rms_Diff);

				h_Diff->SetMaximum((h_Diff->GetMaximum())*1.25);
				//h_Diff->GetXaxis()->SetRangeUser(mean_hDiff - 3*rms_hDiff, mean_hDiff + 3*rms_hDiff);
				c_Diff->Update();				

				filename2 = filename.replace(filename.find("_LINTdac.pdf"), 13, "_DIFFTdac.pdf"); 
				c_Diff->Print(filename2.c_str());

				double mean_h[2] = {mean_hLin, mean_hDiff};
				double rms_h[2] = {rms_hLin, rms_hDiff};

				int zero_Lin=0, zero_Diff=0, n=0;
				//Fill range histograms
				for (int i=0; i<rowno; i++) {
					for (int j=0; j<colno; j++) {
						double *tmp_p = &pix_values[n];
						n++;
						double tmp = *tmp_p;
						if (whichFE(j) != 0) {		
							int bin_num = whichSigma(tmp, mean_h[whichFE(j)-1], rms_h[whichFE(j)-1]);
							range_hist[whichFE(j)-1]->AddBinContent(bin_num);
							if (tmp == 0) {
								if(whichFE(j)==1) zero_Lin++;
								if(whichFE(j)==2) zero_Diff++;
							}				
						}
					}
				}
		
				std::cout << "\n \n \n Number of Zeros are	 " << "	" << zero_Lin << "	" << zero_Diff << std::endl;

				//Lin Range Plot
				h_range_Lin->SetFillColor(kSpring+4);
				h_range_Lin->SetLineColor(kSpring+4);
				h_range_Lin->SetStats(0);
				TCanvas *c_range_Lin = new TCanvas("c_range_Lin", "c_range_Lin", 800, 600);
				style_TH1canvas(c_range_Lin);
				h_range_Lin->Draw();
				h_range_Lin->SetMarkerSize(1.8);
				h_range_Lin->Draw("TEXT0 SAME");
				tname->DrawLatex(0.21,0.93,"RD53A");
				tname->DrawLatex(0.8, 0.93, chipnum.c_str());
				TLatex *zeros = new TLatex();
				zeros->SetNDC();
				zeros->SetTextAlign(13);
				zeros->SetTextFont(63);
				zeros->SetTextSizePixels(20);
				sprintf(zeros_Lin, "Untuned Pixels = %.0i", zero_Lin);
				zeros->DrawLatex(0.18,0.88, zeros_Lin);
				TLegend *lin_range_legend = new TLegend(0.7,0.78,0.88,0.89);
				lin_range_legend->SetHeader("Analog FEs", "C");
				lin_range_legend->AddEntry(h_range_Lin, "Linear", "f");
				lin_range_legend->SetBorderSize(0);
				lin_range_legend->Draw();		
				h_range_Lin->SetMaximum((h_range_Lin->GetMaximum())*1.25);
				c_range_Lin->Update();				
				filename3 = filename.replace(filename.find("_DIFFTdac.pdf"), 14, "_LINRTdac.pdf");
				c_range_Lin->Print(filename3.c_str());

				//Diff Range Plot
				h_range_Diff->SetFillColor(kAzure+5);
				h_range_Diff->SetLineColor(kAzure+5);
				h_range_Diff->SetStats(0);
				TCanvas *c_range_Diff = new TCanvas("c_range_Diff", "c_range_Diff", 800, 600);
				style_TH1canvas(c_range_Diff);
				h_range_Diff->Draw();
				h_range_Diff->SetMarkerSize(1.8);
				h_range_Diff->Draw("TEXT0 SAME");
				tname->DrawLatex(0.21,0.93,"RD53A");
				tname->DrawLatex(0.8, 0.93, chipnum.c_str());
				sprintf(zeros_Diff, "Untuned Pixels = %.0i", zero_Diff);
				zeros->DrawLatex(0.18,0.88, zeros_Diff);
				TLegend *diff_range_legend = new TLegend(0.7,0.78,0.88,0.89);
				diff_range_legend->SetHeader("Analog FEs", "C");
				diff_range_legend->AddEntry(h_range_Diff, "Differential", "f");
				diff_range_legend->SetBorderSize(0);
				diff_range_legend->Draw();		
				h_range_Diff->SetMaximum((h_range_Diff->GetMaximum())*1.25);
				c_range_Diff->Update();				
				filename4 = filename.replace(filename.find("_LINRTdac.pdf"), 15, "_DIFFRTdac.pdf");
				c_range_Diff->Print(filename4.c_str());
	
				
				//Map of Pixels
				style_TH2(h_plot, "Column", "Row"); 
				TCanvas *c_plot = new TCanvas("c_plot", "c_plot", 800, 600);
				style_TH2canvas(c_plot);
				h_plot->Draw("colz");
				h_plot->Draw("colz X+ Y+ SAME");
				tname->DrawLatex(0.21,0.93,"RD53A");
				tname->DrawLatex(0.8, 0.93, chipnum.c_str());
				gStyle->SetOptStat(0);
				filename5=filename.replace(filename.find("_DIFFRTdac.pdf"), 14, "PLOTTdac.pdf");				
				c_plot->RedrawAxis();
				c_plot->Print(filename5.c_str());
				

				delete h_Lin;
				delete c_Lin;
				delete h_Diff;
				delete c_Diff;
				delete h_range_Lin;
				delete c_range_Lin;
				delete h_range_Diff;
				delete c_range_Diff;
				delete h_plot;
				delete c_plot;

		}
	}

	return 0;
} 
