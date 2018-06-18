#include <iostream>
#include <fstream>
#include <vector>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <TStyle.h>
#include <TF1.h>
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

		if ( strstr( file_path, "ThresholdMap") != NULL) { //if filename contains string declared in argument.
			if (strstr(file_path, ".dat") != NULL) {

				chipnum = "Chip SN: " + file_name.substr(0, file_name.find(delimiter)); //get chip # from file name

				std::cout << "Opening file: " << filepath.c_str() << std::endl;
				std::string filename = filepath.c_str();
				std::fstream infile(filepath.c_str(), std::ios::in);

				std::string type;
				std::string name;

				std::string xaxistitle, yaxistitle, xrangetitle;
				std::string line;	

				std::string filename1, filename2, filename3, filename4, filename5, filename6, filename7, filename8, filename9;

	
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
				int rowno, colno, total_pix;
				char mean_Syn[100]={}, mean_Lin[100]={}, mean_Diff[100]={};
				char sigma_Syn[100]={}, sigma_Lin[100]={}, sigma_Diff[100]={};
				char zeros_Syn[100]={}, zeros_Lin[100]={}, zeros_Diff[100]={};
				double fit_Lin_par[4], fit_Lin_err[4], fit_Lin_range[2];
				double chisq_DOF_Lin, mean_hLin, rms_hLin;
				double fit_Diff_par[4], fit_Diff_err[4], fit_Diff_range[2];
				double chisq_DOF_Diff, mean_hDiff, rms_hDiff;	
				double fit_all_par[4], fit_all_err[4], fit_all_range[2];
				double chisq_DOF_all, mean_all, rms_all;
				const char *LabelName[6] = {"1","2","3","4","5",">5"};	
			
				rowno = 192;
				colno = 400;
				total_pix = rowno*colno;
				xaxistitle = "Threshold [e]";
				yaxistitle = "Number of Pixels";
				xrangetitle = "Deviation from the Mean [#sigma] ";
				xbins = 500;
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
					
				TH1* fe_hist[3] = {h_Syn, h_Lin, h_Diff};
				TH1* range_hist[3] = {h_range_Syn, h_range_Lin, h_range_Diff};
				std::vector <double> pix_values[total_pix];

				//Fill Threshold plots	
				for (int i=0; i<rowno; i++) {
					for (int j=0; j<colno; j++) {

						double tmp;
						infile >> tmp;
						//std::cout << i*j << " " << tmp << std::endl;
						pix_values[0].push_back(tmp);
						if (tmp != 0) {	
							h_all->Fill(tmp);
							fe_hist[whichFE(j)]->Fill(tmp);
						}	
					}
				}
			
				for (int i=0; i<3; i++) {
					style_TH1(fe_hist[i], xaxistitle.c_str(), yaxistitle.c_str());
					style_TH1(range_hist[i], xrangetitle.c_str(), yaxistitle.c_str());
					for (int j=1; j<=range_bins; j++) range_hist[i]->GetXaxis()->SetBinLabel(j, LabelName[j-1]);
					range_hist[i]->GetXaxis()->LabelsOption("h");
					range_hist[i]->GetXaxis()->SetLabelSize(0.065);
					range_hist[i]->GetYaxis()->SetLabelSize(0.045);
				}
				//Synchronous FE Plot
				h_Syn->SetFillColor(kOrange+6);
				h_Syn->SetLineColor(kOrange+6);
				h_Syn->SetStats(0);
				TCanvas *c_Syn = new TCanvas("c_Syn", "c_Syn", 800, 600);
				style_TH1canvas(c_Syn);
				h_Syn->Draw();
				TLatex *tname= new TLatex();
				tname->SetNDC();
				tname->SetTextAlign(22);
				tname->SetTextFont(73);
				tname->SetTextSizePixels(30);
				tname->DrawLatex(0.21,0.93,"RD53A");
				tname->DrawLatex(0.8, 0.93, chipnum.c_str());
				TLegend *syn_legend = new TLegend(0.75,0.77,0.93,0.88);
				syn_legend->SetHeader("Analog FEs", "C");
				syn_legend->AddEntry(h_Syn, "Synchronous", "f");
				syn_legend->SetBorderSize(0);
				syn_legend->Draw();		
				
				double fit_Syn_par[4], fit_Syn_err[4], fit_Syn_range[2];
				double chisq_DOF_Syn, mean_hSyn, rms_hSyn;	
				mean_hSyn = h_Syn->GetMean();
				rms_hSyn = h_Syn->GetRMS();
	
				TF1 *fit_Syn = new TF1("fit_Syn", "gaus", mean_hSyn-rms_hSyn, mean_hSyn+rms_hSyn);
				h_Syn->Fit("fit_Syn", "0", "", mean_hSyn-rms_hSyn, mean_hSyn+rms_hSyn);

				for (int i = 0; i<5; i++) {

					for (int j = 0; j<3; j++) {
						fit_Syn_par[j] = fit_Syn->GetParameter(j);
						fit_Syn_err[j] = fit_Syn->GetParError(j);
						std::cout << "New Values	" <<  fit_Syn_par[j] << "	" << fit_Syn_err[j] << std::endl;
					}

					fit_Syn_range[0] = fit_Syn_par[1] - (2* fit_Syn_par[2]);
					fit_Syn_range[1] = fit_Syn_par[1] + (2* fit_Syn_par[2]);
					h_Syn->Fit("fit_Syn", "0", "", fit_Syn_range[0], fit_Syn_range[1] );
				}
				std::cout << "Chi^2 is "  << fit_Syn->GetChisquare() << "	DOF is " << fit_Syn->GetNDF() << "	Prob is " << fit_Syn->GetProb() << std::endl;

				TLatex *mean_sigma = new TLatex;
				mean_sigma->SetNDC();
				mean_sigma->SetTextAlign(13);
				mean_sigma->SetTextFont(63);
				mean_sigma->SetTextSizePixels(24);
				sprintf(mean_Syn, "Mean = %.1f #pm %.1f", fit_Syn_par[1], fit_Syn_err[1]);
				mean_sigma->DrawLatex(0.18,0.88, mean_Syn);
				sprintf(sigma_Syn, "#sigma = %.1f #pm %.1f", fit_Syn_par[2], fit_Syn_err[2]);
				mean_sigma->DrawLatex(0.18,0.84, sigma_Syn);

				chisq_DOF_Syn = (fit_Syn->GetChisquare())/fit_Syn->GetNDF();

				if ( chisq_DOF_Syn < 10 ) {
					std::cout << "\n Threshold is acceptable. Plotting fit. \n" << std::endl;
					h_Syn->Fit("fit_Syn", "B", "I", xlow, xhigh); //Plot over full range.
				} else {
					std::cout<<"\n \033[1;38;5;202;5m Your threshold is crap. Choose a new threshold. \033[0m \n"<<std::endl;
				}

				std::cout << "Chi^2 is "  << fit_Syn->GetChisquare() << "	DOF is " << fit_Syn->GetNDF() << "	Prob is " << fit_Syn->GetProb() << std::endl;

				h_Syn->SetMaximum((h_Syn->GetMaximum())*1.15);
				h_Syn->GetXaxis()->SetRangeUser(mean_hSyn - 3*rms_hSyn, mean_hSyn + 3*rms_hSyn);
				c_Syn->Update();				
	
				filename1 = filename.replace(filename.find(".dat"), 12, "_SYNroot.pdf"); 
				c_Syn->Print(filename1.c_str());


				//Linear FE Plot
				h_Lin->SetFillColor(kSpring+4);
				h_Lin->SetLineColor(kSpring+4);
				h_Lin->SetStats(0);
				TCanvas *c_Lin = new TCanvas("c_Lin", "c_Lin", 800, 600);
				style_TH1canvas(c_Lin);
				h_Lin->Draw();
				tname->DrawLatex(0.21,0.93,"RD53A");
				tname->DrawLatex(0.8, 0.93, chipnum.c_str());
				TLegend *lin_legend = new TLegend(0.75,0.77,0.93,0.88);
				lin_legend->SetHeader("Analog FEs", "C");
				lin_legend->AddEntry(h_Lin, "Linear", "f");
				lin_legend->SetBorderSize(0);
				lin_legend->Draw();	
				mean_hLin = h_Lin->GetMean();
				rms_hLin = h_Lin->GetRMS();

				TF1 *fit_Lin = new TF1("fit_Lin", "gaus", mean_hLin-rms_hLin, mean_hLin+rms_hLin);
				h_Lin->Fit("fit_Lin", "0", "", mean_hLin-rms_hLin, mean_hLin+rms_hLin);

				for (int i = 0; i<5; i++) {

					for (int j = 0; j<3; j++) {
						fit_Lin_par[j] = fit_Lin->GetParameter(j);
						fit_Lin_err[j] = fit_Lin->GetParError(j);
						std::cout << "New Values	" <<  fit_Lin_par[j] << "	" << fit_Lin_err[j] << std::endl;
					}

					fit_Lin_range[0] = fit_Lin_par[1] - (2* fit_Lin_par[2]);
					fit_Lin_range[1] = fit_Lin_par[1] + (2* fit_Lin_par[2]);
					h_Lin->Fit("fit_Lin", "0", "", fit_Lin_range[0], fit_Lin_range[1] );
				}
				std::cout << "Chi^2 is "  << fit_Lin->GetChisquare() << "	DOF is " << fit_Lin->GetNDF() << "	Prob is " << fit_Lin->GetProb() << std::endl;

				chisq_DOF_Lin = (fit_Lin->GetChisquare())/fit_Lin->GetNDF();

				sprintf(mean_Lin, "Mean = %.1f #pm %.1f", fit_Lin_par[1], fit_Lin_err[1]);
				mean_sigma->DrawLatex(0.18,0.88, mean_Lin);
				sprintf(sigma_Lin, "#sigma = %.1f #pm %.1f", fit_Lin_par[2], fit_Lin_err[2]);
				mean_sigma->DrawLatex(0.18,0.84, sigma_Lin);
			
				if ( chisq_DOF_Lin < 10 ) {
					std::cout << "\n Threshold is  acceptable. Plotting fit. \n" << std::endl;
					h_Lin->Fit("fit_Lin", "B", "I", xlow, xhigh); //Plot over full range.
				} else {
					std::cout<<"\n \033[1;38;5;202;5m Your threshold is crap. Choose a new threshold. \033[0m \n"<<std::endl;
				}

				std::cout << "Chi^2 is "  << fit_Lin->GetChisquare() << "	DOF is " << fit_Lin->GetNDF() << "	Prob is " << fit_Lin->GetProb() << std::endl;

				h_Lin->GetXaxis()->SetRangeUser(mean_hLin - 3*rms_hLin, mean_hLin + 3*rms_hLin);
				h_Lin->SetMaximum((h_Lin->GetMaximum())*1.15);
				c_Lin->Update();				
	
				filename2 = filename.replace(filename.find("_SYNroot.pdf"), 12, "_LINroot.pdf"); 
				c_Lin->Print(filename2.c_str());

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
				
				TF1 *fit_Diff = new TF1("fit_Diff", "gaus", mean_hDiff-rms_hDiff, mean_hDiff+rms_hDiff);
				h_Diff->Fit("fit_Diff", "0", "", mean_hDiff-rms_hDiff, mean_hDiff+rms_hDiff);

				for (int i = 0; i<5; i++) {

					for (int j = 0; j<3; j++) {
						fit_Diff_par[j] = fit_Diff->GetParameter(j);
						fit_Diff_err[j] = fit_Diff->GetParError(j);
						std::cout << "New Values	" <<  fit_Diff_par[j] << "	" << fit_Diff_err[j] << std::endl;
					}

					fit_Diff_range[0] = fit_Diff_par[1] - (2* fit_Diff_par[2]);
					fit_Diff_range[1] = fit_Diff_par[1] + (2* fit_Diff_par[2]);
					h_Diff->Fit("fit_Diff", "0", "", fit_Diff_range[0], fit_Diff_range[1] );
				}
				std::cout << "Chi^2 is "  << fit_Diff->GetChisquare() << "	DOF is " << fit_Diff->GetNDF() << "	Prob is " << fit_Diff->GetProb() << std::endl;

				chisq_DOF_Diff = (fit_Diff->GetChisquare())/fit_Diff->GetNDF();

				sprintf(mean_Diff, "Mean = %.1f #pm %.1f", fit_Diff_par[1], fit_Diff_err[1]);
				mean_sigma->DrawLatex(0.18,0.88, mean_Diff);
				sprintf(sigma_Diff, "#sigma = %.1f #pm %.1f", fit_Diff_par[2], fit_Diff_err[2]);
				mean_sigma->DrawLatex(0.18,0.84, sigma_Diff);

				if ( chisq_DOF_Diff < 10 ) {
					std::cout << "\n Threshold is acceptable. Plotting fit. \n" << std::endl;
					h_Diff->Fit("fit_Diff", "B", "I", xlow, xhigh); //Plot over full range.
				} else {
					std::cout<<"\n \033[1;38;5;202;5m Your threshold is crap. Choose a new threshold. \033[0m \n"<<std::endl;
				}

				std::cout << "Chi^2 is "  << fit_Diff->GetChisquare() << "	DOF is " << fit_Diff->GetNDF() << "	Prob is " << fit_Diff->GetProb() << std::endl;

				
				h_Diff->SetMaximum((h_Diff->GetMaximum())*1.15);
				h_Diff->GetXaxis()->SetRangeUser(mean_hDiff - 3*rms_hDiff, mean_hDiff + 3*rms_hDiff);
				c_Diff->Update();				

				filename3 = filename.replace(filename.find("_LINroot.pdf"), 13, "_DIFFroot.pdf"); 
				c_Diff->Print(filename3.c_str());

				//All histograms plot; just need for the fit.			
				style_TH1(h_all, xaxistitle.c_str(), yaxistitle.c_str());				
				h_all->SetFillColor(kOrange);
				h_all->SetLineColor(kOrange);
				h_all->SetStats(0);
				TCanvas *c_all = new TCanvas("c_all", "c_all", 800, 600);
				style_TH1canvas(c_all);
				h_all->Draw();
				tname->DrawLatex(0.21,0.93,"RD53A");
				tname->DrawLatex(0.8, 0.93, chipnum.c_str());
				TLegend *all_legend = new TLegend(0.75,0.77,0.93,0.88);
				all_legend->SetHeader("Analog FEs", "C");
				all_legend->AddEntry(h_all, "All", "f");
				all_legend->SetBorderSize(0);
				all_legend->Draw();

				TF1 *fit_all = new TF1("fit_all", "gaus", xlow, xhigh);
				h_all->Fit("fit_all", "0", "", xlow, xhigh);
				mean_all = h_all->GetMean();
				rms_all = h_all->GetRMS();	

				for (int i = 0; i<5; i++) {

					for (int j = 0; j<3; j++) {
						fit_all_par[j] = fit_all->GetParameter(j);
						fit_all_err[j] = fit_all->GetParError(j);
						std::cout << "New Values	" <<  fit_all_par[j] << "	" << fit_all_err[j] << std::endl;
					}

					fit_all_range[0] = fit_all_par[1] - (2* fit_all_par[2]);
					fit_all_range[1] = fit_all_par[1] + (2* fit_all_par[2]);
					h_all->Fit("fit_all", "0", "", fit_all_range[0], fit_all_range[1] );
				}
				std::cout << "Chi^2 is "  << fit_all->GetChisquare() << "	DOF is " << fit_all->GetNDF() << "	Prob is " << fit_all->GetProb() << std::endl;

				chisq_DOF_all = (fit_all->GetChisquare())/fit_all->GetNDF();

				if ( chisq_DOF_all < 10 ) {
					std::cout << "\n Threshold is acceptable. Plotting fit. \n" << std::endl;
					h_all->Fit("fit_all", "B", "I", xlow, xhigh); //Plot over full range.
				} else {
					std::cout<<"\n \033[1;38;5;202;5m Your threshold is crap. Choose a new threshold. \033[0m \n"<<std::endl;
				}

				std::cout << "Chi^2 is "  << fit_all->GetChisquare() << "	DOF is " << fit_all->GetNDF() << "	Prob is " << fit_all->GetProb() << std::endl;
				
				h_all->GetXaxis()->SetRangeUser(mean_all - 3*rms_all, mean_all + 3*rms_all);
				c_all->Update();				

				filename4 = filename.replace(filename.find("_DIFFroot.pdf"), 14, "_ALLroot.pdf"); 
				c_all->Print(filename4.c_str());

				//Remove fit lines from histograms
				h_Syn->Fit("fit_Syn", "0", "", fit_Syn_range[0], fit_Syn_range[1] );
				h_Lin->Fit("fit_Lin", "0", "", fit_Lin_range[0], fit_Lin_range[1] );
				h_Diff->Fit("fit_Diff", "0", "", fit_Diff_range[0], fit_Diff_range[1] );

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
				TLegend *stack_legend = new TLegend(0.75,0.65,0.93,0.88);
				stack_legend->SetHeader("Analog FEs", "C");
				stack_legend->AddEntry(h_Syn, "Synchronous", "f");
				stack_legend->AddEntry(h_Lin, "Linear", "f");
				stack_legend->AddEntry(h_Diff, "Differential", "f");
				stack_legend->SetBorderSize(0);
				stack_legend->Draw();
				hs->GetXaxis()->SetLabelSize(0.05);
				hs->GetYaxis()->SetLabelSize(0.03);
				tname->DrawLatex(0.21,0.93,"RD53A");
				tname->DrawLatex(0.8, 0.93, chipnum.c_str());

				if ( chisq_DOF_all < 10 ) {
					std::cout << "\n Threshold is acceptable. Plotting fit. \n" << std::endl;
					fit_all->DrawF1(xlow, xhigh, "SAME");
				} else {
					std::cout<<"\n \033[1;38;5;202;5m Your threshold is crap. Choose a new threshold. \033[0m \n"<<std::endl;
				}

				hs->GetXaxis()->SetRangeUser(mean_all - 3*rms_all, mean_all + 3*rms_all);
				c_Stack->Update();				
				
				filename5 = filename.replace(filename.find("_ALLroot.pdf"), 14, "_STACKroot.pdf");
				c_Stack->Print(filename5.c_str());

				TF1* fits[3] = {fit_Syn, fit_Lin, fit_Diff};

				int zero_Syn=0, zero_Lin=0, zero_Diff=0, n=0;
				//Fill range histograms
				for (int i=0; i<rowno; i++) {
					for (int j=0; j<colno; j++) {
						double *tmp_p = &pix_values[0][n];
						n++;
						double tmp = *tmp_p;		
						int bin_num = whichSigma(tmp, fits[whichFE(j)]->GetParameter(1), fits[whichFE(j)]->GetParameter(2));
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
				h_range_Syn->Draw("TEXT0 SAME");
				tname->DrawLatex(0.21,0.93,"RD53A");
				tname->DrawLatex(0.8, 0.93, chipnum.c_str());
				TLatex *zeros = new TLatex();
				zeros->SetNDC();
				zeros->SetTextAlign(13);
				zeros->SetTextFont(63);
				zeros->SetTextSizePixels(20);
				sprintf(zeros_Syn, "Untuned Pixels = %.0i", zero_Syn);
				zeros->DrawLatex(0.18,0.88, zeros_Syn);
				TLegend *syn_range_legend = new TLegend(0.7,0.78,0.88,0.89);
				syn_range_legend->SetHeader("Analog FEs", "C");
				syn_range_legend->AddEntry(h_range_Syn, "Synchronous", "f");
				syn_range_legend->SetBorderSize(0);
				syn_range_legend->Draw();		
				h_range_Syn->SetMaximum((h_range_Syn->GetMaximum())*1.25);
				c_range_Syn->Update();				
				filename6 = filename.replace(filename.find("_STACKroot.pdf"), 14, "_SYNRroot.pdf");
				c_range_Syn->Print(filename6.c_str());

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
				sprintf(zeros_Lin, "Untuned Pixels = %.0i", zero_Lin);
				zeros->DrawLatex(0.18,0.88, zeros_Lin);
				TLegend *lin_range_legend = new TLegend(0.7,0.78,0.88,0.89);
				lin_range_legend->SetHeader("Analog FEs", "C");
				lin_range_legend->AddEntry(h_range_Lin, "Linear", "f");
				lin_range_legend->SetBorderSize(0);
				lin_range_legend->Draw();		
				h_range_Lin->SetMaximum((h_range_Lin->GetMaximum())*1.25);
				c_range_Lin->Update();				
				filename7 = filename.replace(filename.find("_SYNRroot.pdf"), 14, "_LINRroot.pdf");
				c_range_Lin->Print(filename7.c_str());

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
				sprintf(zeros_Diff, "Untuned Pixels = %.0i", zero_Syn);
				zeros->DrawLatex(0.18,0.88, zeros_Diff);
				TLegend *diff_range_legend = new TLegend(0.7,0.78,0.88,0.89);
				diff_range_legend->SetHeader("Analog FEs", "C");
				diff_range_legend->AddEntry(h_range_Diff, "Differential", "f");
				diff_range_legend->SetBorderSize(0);
				diff_range_legend->Draw();		
				h_range_Diff->SetMaximum((h_range_Diff->GetMaximum())*1.25);
				c_range_Diff->Update();				
				filename8 = filename.replace(filename.find("_LINRroot.pdf"), 15, "_DIFFRroot.pdf");
				c_range_Diff->Print(filename8.c_str());
	
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
				TLegend *stackr_legend = new TLegend(0.75,0.65,0.93,0.88);
				stackr_legend->SetHeader("Analog FEs", "C");
				stackr_legend->AddEntry(h_range_Syn, "Synchronous", "f");
				stackr_legend->AddEntry(h_range_Lin, "Linear", "f");
				stackr_legend->AddEntry(h_range_Diff, "Differential", "f");
				stackr_legend->SetBorderSize(0);
				stackr_legend->Draw();
				hs_range->GetXaxis()->SetLabelSize(0.05);
				hs_range->GetYaxis()->SetLabelSize(0.03);
				tname->DrawLatex(0.21,0.93,"RD53A");
				tname->DrawLatex(0.8, 0.93, chipnum.c_str());
				filename9 = filename.replace(filename.find("_DIFFRroot.pdf"), 15, "_STACKRroot.pdf");
				c_Stackr->Print(filename9.c_str());
				

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
