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

int main(int argc, char *argv[]) { //./plotWithRoot_Occupancydir Directory_name
	SetRD53Style();
	gStyle->SetTickLength(0.02);
	gStyle->SetTextFont();

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

		if ( strstr( file_path, "NoiseMap") != NULL) { //if filename contains string declared in argument.
			if (strstr(file_path, ".dat") != NULL) {

				chipnum = "Chip SN: " + file_name.substr(0, file_name.find(delimiter)); //get chip # from file name

				std::cout << "Opening file: " << filepath.c_str() << std::endl;
				std::string filename = filepath.c_str();
				std::fstream infile(filepath.c_str(), std::ios::in);

				std::string type;
				std::string name;

				std::string xaxistitle, yaxistitle, xrangetitle;
				std::string line;	

				std::string filename1, filename2, filename3, filename4, filename5, filename6, filename7, filename8, filename9, filename10;

	
				std::getline(infile, type);
				std::getline(infile, name);
				std::getline(infile, line); //skip "Column"
				std::getline(infile, line); //skip "Rows"
				std::getline(infile, line); //skip "Hits"
				std::getline(infile, line); //skip x range
				std::getline(infile, line); // skip y range

				int xbins, range_bins;
				double xlow, xhigh, range_low, range_high; 
				int underflow, overflow;
				int rowno, colno;
				char mean_Syn[100]={}, mean_Lin[100]={}, mean_Diff[100]={};
				char rms_Syn[100]={}, rms_Lin[100]={}, rms_Diff[100]={};
				double mean_hSyn, mean_hLin, mean_hDiff;
				double rms_hSyn, rms_hLin, rms_hDiff;
				char zeros_Syn[100]={}, zeros_Lin[100]={}, zeros_Diff[100]={};
				const char *LabelName[6] = {"1","2","3","4","5",">5"};	
			
				rowno = 192;
				colno = 400;
				xaxistitle = "Noise [e]";
				yaxistitle = "Number of Pixels";
				xrangetitle = "Deviation from the Mean [RMS] ";
				xbins = 2000;	//5e per bin
				range_bins = 6;
				xlow = -0.5;
				xhigh = 10000.5;
				range_low = 0;
				range_high = 6;

				infile >> underflow >> overflow;

				std::cout << "Histogram type: " << type << std::endl;
				std::cout << "Histogram name: " << name << std::endl;
				std::cout << "X axis title: " << xaxistitle << std::endl;
				std::cout << "Y axis title: " << yaxistitle << std::endl;

				if (!infile) {
					std::cout << "Something wrong with file ..." << std::endl;
					return -1;
				}

				TH1 *h_all = NULL;
				h_all = (TH1*) new TH1F("h_all", "", xbins, xlow, xhigh);
				
				TH1 *h_Syn = NULL;
				h_Syn = (TH1*) new TH1F("h_Syn", "", xbins, xlow, xhigh);

				TH1 *h_Lin = NULL;
				h_Lin = (TH1*) new TH1F("h_Lin","", xbins, xlow, xhigh);

				TH1 *h_Diff = NULL;
				h_Diff = (TH1*) new TH1F("h_Diff","", xbins, xlow, xhigh);

				TH1 *h_range_Syn = NULL;
				h_range_Syn = (TH1*) new TH1F("h_range_Syn","", range_bins, range_low, range_high);

				TH1 *h_range_Lin = NULL;
				h_range_Lin = (TH1*) new TH1F("h_range_Lin","", range_bins, range_low, range_high);

				TH1 *h_range_Diff = NULL;
				h_range_Diff = (TH1*) new TH1F("h_range_Diff","", range_bins, range_low, range_high);


				TH2F *h_plot = NULL;
				h_plot = new TH2F("h_plot", "", colno, 0, colno, rowno, 0, rowno); 
					
				TH1* fe_hist[3] = {h_Syn, h_Lin, h_Diff};
				TH1* range_hist[3] = {h_range_Syn, h_range_Lin, h_range_Diff};
				std::vector <double> pix_values;

				//Fill Threshold plots	
				for (int i=0; i<rowno; i++) {
					for (int j=0; j<colno; j++) {

						double tmp;
						infile >> tmp;
						//std::cout << i*j << " " << tmp << std::endl;
						pix_values.push_back(tmp);
						if (tmp != 0) {	
							h_all->Fill(tmp);
							fe_hist[whichFE(j)]->Fill(tmp);
						}	
						h_plot->SetBinContent(j+1,i+1,tmp);	
					}
				}
			
				for (int i=0; i<3; i++) {
					style_TH1(fe_hist[i], xaxistitle.c_str(), yaxistitle.c_str());
					style_TH1(range_hist[i], xrangetitle.c_str(), yaxistitle.c_str());
					for (int j=1; j<=range_bins; j++) range_hist[i]->GetXaxis()->SetBinLabel(j, LabelName[j-1]);
					range_hist[i]->GetXaxis()->LabelsOption("h");
					fe_hist[i]->GetXaxis()->SetLabelSize(0.035);
				}

				//All histograms plot; just need for the fit.			
				style_TH1(h_all, xaxistitle.c_str(), yaxistitle.c_str());				
				h_all->SetFillColor(kOrange);
				h_all->SetLineColor(kOrange);
				h_all->SetStats(0);
				TCanvas *c_all = new TCanvas("c_all", "c_all", 800, 600);
				style_TH1canvas(c_all);
				h_all->Draw();
				TLatex *tname= new TLatex();
				tname->SetNDC();
				tname->SetTextAlign(22);
				tname->SetTextFont(73);
				tname->SetTextSizePixels(30);
				tname->DrawLatex(0.21,0.96,"RD53A");
				tname->DrawLatex(0.8, 0.96, chipnum.c_str());
				TLegend *all_legend = new TLegend(0.7,0.82,0.87,0.91);
				all_legend->SetHeader("Analog FEs", "C");
				all_legend->AddEntry(h_all, "All", "f");
				all_legend->SetBorderSize(0);
				all_legend->Draw();
				
				double mean_all, rms_all;
				mean_all = h_all->GetMean();
				rms_all = h_all->GetRMS();	

				h_all->SetMaximum((h_all->GetMaximum())*1.21);
				h_all->GetXaxis()->SetRangeUser((mean_all - 3*rms_all < 0) ? -0.5 : (mean_all- 3*rms_all)  , mean_all + 3*rms_all);
				c_all->Update();				

				filename1 = filename.replace(filename.find(".dat"), 8, "_ALL.pdf"); 
				c_all->Print(filename1.c_str());
	
				//Synchronous FE Plot
				h_Syn->SetFillColor(kOrange+6);
				h_Syn->SetLineColor(kOrange+6);
				h_Syn->SetStats(0);
				TCanvas *c_Syn = new TCanvas("c_Syn", "c_Syn", 800, 600);
				style_TH1canvas(c_Syn);
				h_Syn->Draw();
				tname->DrawLatex(0.21,0.96,"RD53A");
				tname->DrawLatex(0.8, 0.96, chipnum.c_str());
				TLegend *syn_legend = new TLegend(0.7,0.82,0.87,0.91);
				syn_legend->SetHeader("Analog FEs", "C");
				syn_legend->AddEntry(h_Syn, "Synchronous", "f");
				syn_legend->SetBorderSize(0);
				syn_legend->Draw();		
				
				mean_hSyn = h_Syn->GetMean();
				rms_hSyn = h_Syn->GetRMS();
	
				TLatex *mean_rms = new TLatex;
				mean_rms->SetNDC();
				mean_rms->SetTextAlign(13);
				mean_rms->SetTextFont(63);
				mean_rms->SetTextSizePixels(24);
				sprintf(mean_Syn, "Mean = %.1f #pm %.1f", h_Syn->GetMean(), h_Syn->GetMeanError());
				mean_rms->DrawLatex(0.18,0.91, mean_Syn);
				sprintf(rms_Syn, "RMS = %.1f #pm %.1f", h_Syn->GetRMS(), h_Syn->GetRMSError());
				mean_rms->DrawLatex(0.18,0.87, rms_Syn);

				h_Syn->SetMaximum((h_Syn->GetMaximum())*1.21);
				h_Syn->GetXaxis()->SetRangeUser((mean_hSyn - 3*rms_hSyn < 0) ? -0.5 : (mean_hSyn- 3*rms_hSyn)  , mean_hSyn + 3*rms_hSyn);
				c_Syn->Update();				
	
				filename2 = filename.replace(filename.find("_ALL.pdf"), 8, "_SYN.pdf"); 
				c_Syn->Print(filename2.c_str());


				//Linear FE Plot
				h_Lin->SetFillColor(kSpring+4);
				h_Lin->SetLineColor(kSpring+4);
				h_Lin->SetStats(0);
				TCanvas *c_Lin = new TCanvas("c_Lin", "c_Lin", 800, 600);
				style_TH1canvas(c_Lin);
				h_Lin->Draw();
				tname->DrawLatex(0.21,0.96,"RD53A");
				tname->DrawLatex(0.8, 0.96, chipnum.c_str());
				TLegend *lin_legend = new TLegend(0.7,0.82,0.87,0.91);
				lin_legend->SetHeader("Analog FEs", "C");
				lin_legend->AddEntry(h_Lin, "Linear", "f");
				lin_legend->SetBorderSize(0);
				lin_legend->Draw();	
				mean_hLin = h_Lin->GetMean();
				rms_hLin = h_Lin->GetRMS();

				sprintf(mean_Lin, "Mean = %.1f #pm %.1f", h_Lin->GetMean(), h_Lin->GetMeanError());
				mean_rms->DrawLatex(0.18,0.91, mean_Lin);
				sprintf(rms_Lin, "RMS = %.1f #pm %.1f", h_Lin->GetRMS(), h_Lin->GetRMSError());
				mean_rms->DrawLatex(0.18,0.87, rms_Lin);

				h_Lin->GetXaxis()->SetRangeUser((mean_hLin - 3*rms_hLin < 0) ? -0.5 : (mean_hLin- 3*rms_hLin)  , mean_hLin + 3*rms_hLin);
				h_Lin->SetMaximum((h_Lin->GetMaximum())*1.21);
				c_Lin->Update();				
	
				filename3 = filename.replace(filename.find("_SYN.pdf"), 8, "_LIN.pdf"); 
				c_Lin->Print(filename3.c_str());

				//Diff FE Plot
				h_Diff->SetFillColor(kAzure+5);
				h_Diff->SetLineColor(kAzure+5);
				h_Diff->SetStats(0);
				TCanvas *c_Diff = new TCanvas("c_Diff", "c_Diff", 800, 600);
				style_TH1canvas(c_Diff);
				h_Diff->Draw();
				tname->DrawLatex(0.21,0.96,"RD53A");
				tname->DrawLatex(0.8, 0.96, chipnum.c_str());
				TLegend *diff_legend = new TLegend(0.7,0.82,0.87,0.91);
				diff_legend->SetHeader("Analog FEs", "C");
				diff_legend->AddEntry(h_Diff, "Differential", "f");
				diff_legend->SetBorderSize(0);
				diff_legend->Draw();

				mean_hDiff = h_Diff->GetMean();
				rms_hDiff = h_Diff->GetRMS();
				
				sprintf(mean_Diff, "Mean = %.1f #pm %.1f", h_Diff->GetMean(), h_Diff->GetMeanError());
				mean_rms->DrawLatex(0.18,0.91, mean_Diff);
				sprintf(rms_Diff, "RMS = %.1f #pm %.1f", h_Diff->GetRMS(), h_Diff->GetRMSError());
				mean_rms->DrawLatex(0.18,0.87, rms_Diff);

				//h_Diff->GetYaxis()->SetRangeUser(0,5000);
				//std::cout << "Maximum	" << h_Diff->GetMaximum() << std::endl;
				h_Diff->SetMaximum((h_Diff->GetMaximum())*1.21);
				h_Diff->GetXaxis()->SetRangeUser((mean_hDiff - 3*rms_hDiff < 0) ? -0.5 : (mean_hDiff- 3*rms_hDiff)  , mean_hDiff + 3*rms_hDiff);
				c_Diff->Update();				

				filename4 = filename.replace(filename.find("_LIN.pdf"), 9, "_DIFF.pdf"); 
				c_Diff->Print(filename4.c_str());


				//Map of Pixels
				style_TH2(h_plot, "Column", "Row", xaxistitle.c_str()); 
				TCanvas *c_plot = new TCanvas("c_plot", "c_plot", 800, 600);
				style_TH2canvas(c_plot);
				h_plot->Draw("colz");
				//h_plot->Draw("colz X+ Y+ SAME");
				h_plot->GetXaxis()->SetLabelSize(0.04);
				h_plot->GetYaxis()->SetLabelSize(0.04);
				tname->DrawLatex(0.16,0.96,"RD53A");
				tname->DrawLatex(0.72, 0.96, chipnum.c_str());
				gStyle->SetOptStat(0);
				c_plot->RedrawAxis();
				c_plot->Update();
				h_plot->GetZaxis()->SetRangeUser((mean_all - 3*rms_all < 0) ? -0.5 : (mean_all- 3*rms_all)  , mean_all + 3*rms_all);	
				h_plot->GetZaxis()->SetLabelSize(0.04);
				filename5=filename.replace(filename.find("_DIFF.pdf"), 9, "_PLOT.pdf");				
				c_plot->Print(filename5.c_str());


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
				//Setting the title for THStack plots has to be after Draw(), and needs canvas->Modified() after
				style_THStack(hs, xaxistitle.c_str(), yaxistitle.c_str());
				hs->GetXaxis()->SetLabelSize(0.065);
				hs->GetYaxis()->SetLabelSize(0.045);
				gPad->SetLogy(0);
				c_Stack->Modified();
				gStyle->SetOptStat(0);
				TLegend *stack_legend = new TLegend(0.33,0.82,0.93,0.91);
				stack_legend->SetNColumns(3);
				stack_legend->SetHeader("Analog FEs", "C");
				stack_legend->AddEntry(h_Syn, "Synchronous", "f");
				stack_legend->AddEntry(h_Lin, "Linear", "f");
				stack_legend->AddEntry(h_Diff, "Differential", "f");
				stack_legend->SetBorderSize(0);
				stack_legend->Draw();
				hs->GetXaxis()->SetLabelSize(0.05);
				hs->GetYaxis()->SetLabelSize(0.03);
				tname->DrawLatex(0.21,0.96,"RD53A");
				tname->DrawLatex(0.8, 0.96, chipnum.c_str());

				hs->SetMaximum((h_all->GetMaximum())*1.21);
				hs->GetXaxis()->SetRangeUser((mean_all - 3*rms_all < 0) ? -0.5 : (mean_all- 3*rms_all)  , mean_all + 3*rms_all);
				c_Stack->Update();				
				
				filename6 = filename.replace(filename.find("_PLOT.pdf"), 10, "_STACK.pdf");
				c_Stack->Print(filename6.c_str());

				double mean_h[3] = {mean_hSyn, mean_hLin, mean_hDiff};
				double rms_h[3] = {rms_hSyn, rms_hLin, rms_hDiff};
				
				int zero_Syn=0, zero_Lin=0, zero_Diff=0, n=0;
				//Fill range histograms
				for (int i=0; i<rowno; i++) {
					for (int j=0; j<colno; j++) {
						double *tmp_p = &pix_values[n];
						n++;
						double tmp = *tmp_p;		
						int bin_num = whichSigma(tmp, mean_h[whichFE(j)], rms_h[whichFE(j)]);
						range_hist[whichFE(j)]->AddBinContent(bin_num);
						if (tmp == 0) {
							if(whichFE(j)==0) zero_Syn++;
							if(whichFE(j)==1) zero_Lin++;
							if(whichFE(j)==2) zero_Diff++;
						}				
					}
				}
		
				std::cout << "\n \n \n Number of Zeros are	 " << zero_Syn << "	" << zero_Lin << "	" << zero_Diff << std::endl;
	
				//Syn Range Plot	
				h_range_Syn->SetFillColor(kOrange+6);
				h_range_Syn->SetLineColor(kOrange+6);
				h_range_Syn->SetStats(0);
				TCanvas *c_range_Syn = new TCanvas("c_range_Syn", "c_range_Syn", 800, 600);
				style_TH1canvas(c_range_Syn);
				h_range_Syn->Draw();
				h_range_Syn->SetMarkerSize(1.8);
				h_range_Syn->SetMarkerColor(1);
				h_range_Syn->Draw("TEXT0 SAME");
				tname->DrawLatex(0.21,0.96,"RD53A");
				tname->DrawLatex(0.8, 0.96, chipnum.c_str());
				TLatex *zeros = new TLatex();
				zeros->SetNDC();
				zeros->SetTextAlign(13);
				zeros->SetTextFont(63);
				zeros->SetTextSizePixels(20);
				sprintf(zeros_Syn, "Untuned Pixels = %.0i", zero_Syn);
				zeros->DrawLatex(0.18,0.91, zeros_Syn);
				TLegend *syn_range_legend = new TLegend(0.7,0.82,0.87,0.91);
				syn_range_legend->SetHeader("Analog FEs", "C");
				syn_range_legend->AddEntry(h_range_Syn, "Synchronous", "f");
				syn_range_legend->SetBorderSize(0);
				syn_range_legend->Draw();		
				h_range_Syn->SetMaximum((h_range_Syn->GetMaximum())*1.25);
				c_range_Syn->Update();				
				filename7 = filename.replace(filename.find("_STACK.pdf"), 13, "_SYNrange.pdf");
				c_range_Syn->Print(filename7.c_str());

				//Lin Range Plot
				h_range_Lin->SetFillColor(kSpring+4);
				h_range_Lin->SetLineColor(kSpring+4);
				h_range_Lin->SetStats(0);
				TCanvas *c_range_Lin = new TCanvas("c_range_Lin", "c_range_Lin", 800, 600);
				style_TH1canvas(c_range_Lin);
				h_range_Lin->Draw();
				h_range_Lin->SetMarkerSize(1.8);
				h_range_Lin->SetMarkerColor(1);
				h_range_Lin->Draw("TEXT0 SAME");
				tname->DrawLatex(0.21,0.96,"RD53A");
				tname->DrawLatex(0.8, 0.96, chipnum.c_str());
				sprintf(zeros_Lin, "Untuned Pixels = %.0i", zero_Lin);
				zeros->DrawLatex(0.18,0.91, zeros_Lin);
				TLegend *lin_range_legend = new TLegend(0.7,0.82,0.87,0.91);
				lin_range_legend->SetHeader("Analog FEs", "C");
				lin_range_legend->AddEntry(h_range_Lin, "Linear", "f");
				lin_range_legend->SetBorderSize(0);
				lin_range_legend->Draw();		
				h_range_Lin->SetMaximum((h_range_Lin->GetMaximum())*1.25);
				c_range_Lin->Update();				
				filename8 = filename.replace(filename.find("_SYNrange.pdf"), 13, "_LINrange.pdf");
				c_range_Lin->Print(filename8.c_str());

				//Diff Range Plot
				h_range_Diff->SetFillColor(kAzure+5);
				h_range_Diff->SetLineColor(kAzure+5);
				h_range_Diff->SetStats(0);
				TCanvas *c_range_Diff = new TCanvas("c_range_Diff", "c_range_Diff", 800, 600);
				style_TH1canvas(c_range_Diff);
				h_range_Diff->Draw();
				h_range_Diff->SetMarkerSize(1.8);
				h_range_Diff->SetMarkerColor(1);
				h_range_Diff->Draw("TEXT0 SAME");
				tname->DrawLatex(0.21,0.96,"RD53A");
				tname->DrawLatex(0.8, 0.96, chipnum.c_str());
				sprintf(zeros_Diff, "Untuned Pixels = %.0i", zero_Syn);
				zeros->DrawLatex(0.18,0.91, zeros_Diff);
				TLegend *diff_range_legend = new TLegend(0.7,0.82,0.87,0.91);
				diff_range_legend->SetHeader("Analog FEs", "C");
				diff_range_legend->AddEntry(h_range_Diff, "Differential", "f");
				diff_range_legend->SetBorderSize(0);
				diff_range_legend->Draw();		
				h_range_Diff->SetMaximum((h_range_Diff->GetMaximum())*1.25);
				c_range_Diff->Update();				
				filename9 = filename.replace(filename.find("_LINrange.pdf"), 14, "_DIFFrange.pdf");
				c_range_Diff->Print(filename9.c_str());
	
				//Stacked Range Plot
				THStack *hs_range = new THStack("hs","");
				hs_range->Add(h_range_Syn);
				hs_range->Add(h_range_Lin);
				hs_range->Add(h_range_Diff);
				TCanvas *c_Stackr = new TCanvas("c_Stackr", "c_Stackr", 800, 600);
				c_Stackr->SetLeftMargin(0.15);
				c_Stackr->SetRightMargin(0.05);
				c_Stackr->SetBottomMargin(0.1225);
				hs_range->Draw(); 
				//Setting the title for THStack plots has to be after Draw(), and needs canvas->Modified() after
				style_THStack(hs_range, xrangetitle.c_str(), yaxistitle.c_str());
				hs_range->GetXaxis()->SetLabelSize(0.065);
				hs_range->GetYaxis()->SetLabelSize(0.045);
				gPad->SetLogy(0);
				c_Stackr->Modified();
				gStyle->SetOptStat(0);
				TLegend *stackr_legend = new TLegend(0.33,0.82,0.93,0.91);
				stackr_legend->SetNColumns(3);	
				stackr_legend->SetHeader("Analog FEs", "C");
				stackr_legend->AddEntry(h_range_Syn, "Synchronous", "f");
				stackr_legend->AddEntry(h_range_Lin, "Linear", "f");
				stackr_legend->AddEntry(h_range_Diff, "Differential", "f");
				stackr_legend->SetBorderSize(0);
				stackr_legend->Draw();
				hs_range->GetXaxis()->SetLabelSize(0.05);
				hs_range->GetYaxis()->SetLabelSize(0.03);
				tname->DrawLatex(0.21,0.96,"RD53A");
				tname->DrawLatex(0.8, 0.96, chipnum.c_str());
				hs_range->SetMaximum((hs_range->GetMaximum())*1.21);
				c_Stackr->Update();
				filename10 = filename.replace(filename.find("_DIFFrange.pdf"), 15, "_STACKrange.pdf");
				c_Stackr->Print(filename10.c_str());
				

				delete h_Syn;
				delete c_Syn;
				delete h_Lin;
				delete c_Lin;
				delete h_Diff;
				delete c_Diff;
				delete h_all;
				delete c_all;
				delete hs;
				delete c_Stack;
				delete h_range_Syn;
				delete c_range_Syn;
				delete h_range_Lin;
				delete c_range_Lin;
				delete h_range_Diff;
				delete c_range_Diff;
				delete hs_range;
				delete c_Stackr;
		
			}	
		}
	}

	return 0;
} 
