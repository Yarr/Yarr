#include <iostream>
#include <fstream>
#include <vector>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <TStyle.h>
#include <TPaletteAxis.h>
#include <TF1.h>
#include <TGaxis.h>
#include <TMarker.h>

#include <plotWithRoot.h>
#include <RD53Style.h>
#include "json.hpp"

using json=nlohmann::basic_json<std::map, std::vector, std::string, bool, std::int32_t, std::uint32_t, float>;

int main(int argc, char *argv[]) { //./plotWithRoot_json path/to/directory file_ext
	//Example file extensions: png, pdf, C, root	

	SetRD53Style();
	gStyle->SetTickLength(0.02);
	gStyle->SetTextFont();

	if (argc < 3) {
		std::cout << "No directory and/or image plot extension given! \nExample: ./plotWithRoot_json path/to/directory/ pdf" << std::endl;
		return -1;
	}

	std::string ext = argv[2];

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
		if (strstr(file_path,"before_") != NULL || strstr(file_path,"after_") != NULL) continue; //skip if file is a pdf

		else if ( strstr(file_path, ".json") != NULL && strstr(file_path, "rd53a") !=NULL ) { //if filename contains string declared in argument.

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

				int lin_bins, diff_bins, range_bins;
				double xlow, xhigh, range_low, range_high, lin_low; 
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
				lin_bins = 16;
				diff_bins = 31;
				range_bins = 6;
				xlow = -15.5;
				xhigh = 15.5;
				lin_low = -0.5;	
				range_low = 0;
				range_high = 6;

				chipnum = "Chip SN: ";
				chipnumber = cfg["RD53A"]["Parameter"]["Name"];
				chipnum.append(chipnumber);
				std::string rd53 = "RD53A Internal";
	
				if (!cfgfile) {
					std::cout << "Something wrong with file ..." << std::endl;
					return -1;
				}
		
				//Create Histograms				
				TH1 *h_Lin = NULL;
				h_Lin = (TH1*) new TH1F("h_Lin","", lin_bins, lin_low, xhigh);

				TH1 *h_Diff = NULL;
				h_Diff = (TH1*) new TH1F("h_Diff","", diff_bins, xlow, xhigh);

				TH1 *h_range_Lin = NULL;
				h_range_Lin = (TH1*) new TH1F("h_range_Lin","", range_bins, 0, range_high);

				TH1 *h_range_Diff = NULL;
				h_range_Diff = (TH1*) new TH1F("h_range_Diff","", range_bins, range_low, range_high);
				
				TH2F *h_TDAC = NULL;
				h_TDAC = new TH2F("h_TDAC", "", colno, 0, colno, rowno, 0, rowno); 
		
				TH2F *h_En = NULL;
				h_En = new TH2F("h_En", "", colno, 0, colno, rowno, 0, rowno); 

				TH2F *h_HitOr = NULL;
				h_HitOr = new TH2F("h_HitOr", "", colno, 0, colno, rowno, 0, rowno); 
					
				TH1* fe_hist[2] = {h_Lin, h_Diff};
				TH1* range_hist[2] = {h_range_Lin, h_range_Diff};
				TH2* h_2D[3] = {h_TDAC, h_En, h_HitOr};
				std::vector <double> pix_values;

				int en_counter=0, hit_counter=0;

				//Fill Threshold plots	
				for (int i=0; i<rowno; i++) {
					for (int j=0; j<colno; j++) {
						int tmp_TDAC, tmp_En, tmp_HitOr;
						tmp_TDAC = cfg["RD53A"]["PixelConfig"][j]["TDAC"][i];
						tmp_En = cfg["RD53A"]["PixelConfig"][j]["Enable"][i];
						tmp_HitOr = cfg["RD53A"]["PixelConfig"][j]["Hitbus"][i];
						
						pix_values.push_back(tmp_TDAC);
						if (whichFE(j) != 0) fe_hist[whichFE(j)-1]->Fill(tmp_TDAC);
						h_TDAC->SetBinContent(j+1,i+1,tmp_TDAC);
						
						h_En->SetBinContent(j+1,i+1,tmp_En);
						h_HitOr->SetBinContent(j+1,i+1,tmp_HitOr);
				
						if (tmp_En == 0) en_counter++;
						if (tmp_HitOr == 0) hit_counter++;		
					}
				}

				for (int i=0; i<2; i++) {
					style_TH1(fe_hist[i], xaxistitle.c_str(), yaxistitle.c_str());
					style_TH1(range_hist[i], xrangetitle.c_str(), yaxistitle.c_str());
					for (int j=1; j<=range_bins; j++) range_hist[i]->GetXaxis()->SetBinLabel(j, LabelName[j-1]);
					range_hist[i]->GetXaxis()->LabelsOption("h");
				}
	
				//Linear FE TDAC Plot
				h_Lin->SetFillColor(kSpring+4);
				h_Lin->SetLineColor(kSpring+4);
				h_Lin->SetStats(0);
				TCanvas *c_Lin = new TCanvas("c_Lin", "c_Lin", 800, 600);
				style_TH1canvas(c_Lin);
				h_Lin->GetXaxis()->SetLabelSize(0.035);
				h_Lin->Draw();
				TLatex *tname= new TLatex();
				tname->SetNDC();
				tname->SetTextAlign(22);
				tname->SetTextFont(73);
				tname->SetTextSizePixels(30);
				tname->DrawLatex(0.28,0.96, rd53.c_str());
				tname->DrawLatex(0.8, 0.96, chipnum.c_str());
				TLegend *lin_legend = new TLegend(0.7,0.82,0.87,0.91);
				lin_legend->SetHeader("Analog FEs", "C");
				lin_legend->AddEntry(h_Lin, "Linear", "f");
				lin_legend->SetBorderSize(0);
				lin_legend->Draw();	
				mean_hLin = h_Lin->GetMean();
				rms_hLin = h_Lin->GetRMS();

				//Write Mean and RMS
				TLatex *mean_rms = new TLatex;
				mean_rms->SetNDC();
				mean_rms->SetTextAlign(13);
				mean_rms->SetTextFont(63);
				mean_rms->SetTextSizePixels(24);
				sprintf(mean_Lin, "Mean = %.1f", h_Lin->GetMean());
				mean_rms->DrawLatex(0.18,0.91, mean_Lin);
				sprintf(rms_Lin, "RMS = %.1f", h_Lin->GetRMS());
				mean_rms->DrawLatex(0.18,0.87, rms_Lin);

				h_Lin->SetMaximum((h_Lin->GetMaximum())*1.25);
				c_Lin->Update();				
				
				std::string lin_ext = "_LINTdac.";	
				filename1 = filename.append(lin_ext.append(ext)); 
				c_Lin->Print(filename1.c_str());

				//Diff FE TDAC Plot
				h_Diff->SetFillColor(kAzure+5);
				h_Diff->SetLineColor(kAzure+5);
				h_Diff->SetStats(0);
				TCanvas *c_Diff = new TCanvas("c_Diff", "c_Diff", 800, 600);
				style_TH1canvas(c_Diff);
				h_Diff->GetXaxis()->SetLabelSize(0.035);
				h_Diff->Draw();
				tname->DrawLatex(0.28,0.96, rd53.c_str());
				tname->DrawLatex(0.8, 0.96, chipnum.c_str());
				TLegend *diff_legend = new TLegend(0.7,0.82,0.87,0.91);
				diff_legend->SetHeader("Analog FEs", "C");
				diff_legend->AddEntry(h_Diff, "Differential", "f");
				diff_legend->SetBorderSize(0);
				diff_legend->Draw();

				mean_hDiff = h_Diff->GetMean();
				rms_hDiff = h_Diff->GetRMS();
				
				sprintf(mean_Diff, "Mean = %.1f", h_Diff->GetMean());
				mean_rms->DrawLatex(0.18,0.91, mean_Diff);
				sprintf(rms_Diff, "RMS = %.1f", h_Diff->GetRMS());
				mean_rms->DrawLatex(0.18,0.87, rms_Diff);

				h_Diff->SetMaximum((h_Diff->GetMaximum())*1.25);
				c_Diff->Update();				
				
				std::string diff_ext = "_DIFFTdac.";
				filename2 = filename.replace(filename.find(lin_ext), 14, diff_ext.append(ext)); 
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
							int bin_num = whichSigma(tmp, mean_h[whichFE(j)-1], rms_h[whichFE(j)-1], 1, range_bins);
							range_hist[whichFE(j)-1]->AddBinContent(bin_num);
							if (tmp <= 0) {
								if(whichFE(j)==1) zero_Lin++;
								if(whichFE(j)==2) zero_Diff++;
							}				
						}
					}
				}
		
				std::cout << "\n \n \n Number of Zeros are	 " << "	" << zero_Lin << "	" << zero_Diff << std::endl;

				//Lin Range TDAC Plot
				h_range_Lin->SetFillColor(kSpring+4);
				h_range_Lin->SetLineColor(kSpring+4);
				h_range_Lin->SetStats(0);
				TCanvas *c_range_Lin = new TCanvas("c_range_Lin", "c_range_Lin", 800, 600);
				style_TH1canvas(c_range_Lin);
				h_range_Lin->Draw();
				h_range_Lin->SetMarkerSize(1.8);
				h_range_Lin->SetMarkerColor(1);
				h_range_Lin->Draw("TEXT0 SAME");
				tname->DrawLatex(0.28,0.96, rd53.c_str());
				tname->DrawLatex(0.8, 0.96, chipnum.c_str());
				TLatex *zeros = new TLatex();
				zeros->SetNDC();
				zeros->SetTextAlign(13);
				zeros->SetTextFont(63);
				zeros->SetTextSizePixels(20);
				sprintf(zeros_Lin, "Untuned Pixels = %i", zero_Lin);
				zeros->DrawLatex(0.18,0.91, zeros_Lin);
				TLegend *lin_range_legend = new TLegend(0.7,0.82,0.87,0.91);
				lin_range_legend->SetHeader("Analog FEs", "C");
				lin_range_legend->AddEntry(h_range_Lin, "Linear", "f");
				lin_range_legend->SetBorderSize(0);
				lin_range_legend->Draw();		
				h_range_Lin->SetMaximum((h_range_Lin->GetMaximum())*1.25);
				c_range_Lin->Update();			
				
				std::string linr_ext = "_LINRTdac.";	
				filename3 = filename.replace(filename.find(diff_ext), 15, linr_ext.append(ext));
				c_range_Lin->Print(filename3.c_str());

				//Diff Range TDAC Plot
				h_range_Diff->SetFillColor(kAzure+5);
				h_range_Diff->SetLineColor(kAzure+5);
				h_range_Diff->SetStats(0);
				TCanvas *c_range_Diff = new TCanvas("c_range_Diff", "c_range_Diff", 800, 600);
				style_TH1canvas(c_range_Diff);
				h_range_Diff->Draw();
				h_range_Diff->SetMarkerSize(1.8);
				h_range_Diff->SetMarkerColor(1);
				h_range_Diff->Draw("TEXT0 SAME");
				tname->DrawLatex(0.28,0.96, rd53.c_str());
				tname->DrawLatex(0.8, 0.96, chipnum.c_str());
				sprintf(zeros_Diff, "Untuned Pixels = %i", zero_Diff);
				zeros->DrawLatex(0.18,0.91, zeros_Diff);
				TLegend *diff_range_legend = new TLegend(0.7,0.82,0.87,0.91);
				diff_range_legend->SetHeader("Analog FEs", "C");
				diff_range_legend->AddEntry(h_range_Diff, "Differential", "f");
				diff_range_legend->SetBorderSize(0);
				diff_range_legend->Draw();		
				h_range_Diff->SetMaximum((h_range_Diff->GetMaximum())*1.25);
				c_range_Diff->Update();			
				std::string diffr_ext = "_DIFFRTdac.";	
				filename4 = filename.replace(filename.find(linr_ext), 16,diffr_ext.append(ext));
				c_range_Diff->Print(filename4.c_str());
	
				std::string c_name[3] = {"c_TDAC", "c_En", "c_HitOr"};	
				TCanvas* c_2D[3];
				std::string plot_ext[4] = {"_DIFFRTdac.", "_PLOTTdac.", "_PLOTEnab.", "_PLOTHitOr."};					
				for(int i=0; i<4; i++) plot_ext[i].append(ext);				

				std::string zaxistitle[3] = {"TDAC Setting", "Enable", "Hitbus"};

				//Map of Pixels for TDAC, EnableMask, and HitOr
				for (int i=0; i<3; i++) {
					style_TH2(h_2D[i], "Column", "Row", zaxistitle[i].c_str()); 
					c_2D[i] = new TCanvas(c_name[i].c_str(), c_name[i].c_str(), 800, 600);
					style_TH2canvas(c_2D[i]);
					//h_2D[i]->GetZaxis()->SetTitleOffset(1.2);
					//c_2D[i]->SetRightMargin(0.12);	
					h_2D[i]->Draw("colz");
					h_2D[i]->Draw("colz X+ Y+ SAME");
					h_2D[i]->GetXaxis()->SetLabelSize(0.04);
					h_2D[i]->GetYaxis()->SetLabelSize(0.04);
					tname->DrawLatex(0.23,0.96,rd53.c_str());
					tname->DrawLatex(0.72, 0.96, chipnum.c_str());
					gStyle->SetOptStat(0);
					c_2D[i]->RedrawAxis();
					c_2D[i]->Update();
					h_2D[i]->GetZaxis()->SetLabelSize(0.04);
				
					//For the Hitbus and EnableMask, the only values are 0 and 1, so scale is changed accordingly. If <= 25% of pixels have value= 0, draw a red circle around them.
					if (i>0) {
						h_2D[i]->GetZaxis()->SetRangeUser(0,1.05);
						if ((i==1 && en_counter <= rowno*colno*0.25) || (i==2 && hit_counter <= rowno*colno*0.25)) {
							for (int j=0; j<colno; j++) {
								for (int k=0; k<rowno; k++) {
									if (h_2D[i]->GetBinContent(j+1,k+1) == 0) {
										TMarker *m = new TMarker(j+1,k+1,0);
										m->SetMarkerSize(0.5);
										m->SetMarkerColor(2);
										m->SetMarkerStyle(kCircle);
										m->Draw();
									}

								}
							}
						}
					}
					filename5=filename.replace(filename.find(plot_ext[i].c_str()), 15, plot_ext[i+1].c_str());				
					c_2D[i]->Print(filename5.c_str());
				}

				delete h_Lin;
				delete c_Lin;
				delete h_Diff;
				delete c_Diff;
				delete h_range_Lin;
				delete c_range_Lin;
				delete h_range_Diff;
				delete c_range_Diff;
				for (int i=0; i<3; i++) {
					delete c_2D[i];
					delete h_2D[i];
				}
		}
	}

	return 0;
} 
