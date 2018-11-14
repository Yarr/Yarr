#include <iostream>
#include <fstream>
#include <vector>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <TH1.h>
#include <TStyle.h>
#include <TF1.h>
#include <plotWithRoot.h>
#include <RD53Style.h>

int main(int argc, char *argv[]) { //./plotWithRoot_Threshold path/to/directory file_ext goodDiff_On
	//Example file extensions: png, pdf, C, root	

	SetRD53Style();	
	gStyle->SetTickLength(0.02);
	gStyle->SetTextFont();

	if (argc < 4) {
		std::cout << "No directory, image plot extension, and/or good differential FE pixels option given! \nFor only good differential FE pixels, write '1'. \n./plotWithRoot_Threshold path/to/directory/ file_ext goodDiff_On \nExample: ./plotWithRoot_Threshold path/to/directory/ pdf 1" << std::endl;
		return -1;
	}

	std::string dir, filepath, file_name, chipnum;
	DIR *dp;
	struct dirent *dirp;
	struct stat filestat;

	std::string delimiter = "_";
	std::string ext = argv[2];
	std::string good_Diff = argv[3];

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

		if ( strstr( file_path, "ThresholdMap.dat") != NULL) { //if filename contains string declared in argument.

			chipnum = "Chip SN: " + file_name.substr(0, file_name.find(delimiter)); //get chip # from file name

			std::cout << "Opening file: " << filepath.c_str() << std::endl;
			std::string filename = filepath.c_str();
			std::fstream infile(filepath.c_str(), std::ios::in);

			std::string type;
			std::string name;

			std::string xaxistitle, yaxistitle, gausrangetitle, rmsrangetitle;
			std::string line;	

			std::string filename1, filename2, filename3, filename4, filename5, filename6;


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

			rowno = 192;
			colno = 400;
			xaxistitle = "Threshold [e]";
			yaxistitle = "Number of Pixels";
			gausrangetitle = "Deviation from the Mean [#sigma] ";
			rmsrangetitle = "Deviation from the Mean [RMS] ";
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

			
			//Create histograms
			TH1 *fe_hist[4];
			TH1 *range_rms[3];
			TH1 *range_gaus[3];
			
			std::string fe_name[4] = {"h_all", "h_Syn", "h_Lin", "h_Diff"};
			std::string rms_names[3] = {"hrms_Syn", "hrms_Lin", "hrms_Diff"};
			std::string gaus_names[3] = {"hgaus_Syn", "hgaus_Lin", "hgaus_Diff"};

			for (unsigned i=0; i<4; i++) {
				fe_hist[i] = new TH1F(fe_name[i].c_str(),"", xbins, xlow, xhigh);
				if (i<3) {
					range_rms[i] = new TH1F(rms_names[i].c_str(), "", range_bins, range_low, range_high);
					range_gaus[i] = new TH1F(gaus_names[i].c_str(), "", range_bins, range_low, range_high);
				}
			}

			TH2F *h_plot = NULL;
			h_plot = new TH2F("h_plot", "", colno, 0, colno, rowno, 0, rowno); 

			std::vector <double> pix_values;

			//Fill Threshold plots	
			for (int i=0; i<rowno; i++) {
				for (int j=0; j<colno; j++) {
					double tmp;
					infile >> tmp;
					if (good_Diff != "1" || whichFE(j) != 2 ) {		//if not in Differential FE
						if (tmp > 0) {
							fe_hist[0]->Fill(tmp);
							fe_hist[whichFE(j)+1]->Fill(tmp);
						}
						h_plot->SetBinContent(j+1,i+1,tmp);	
						pix_values.push_back(tmp);
					}
					else {
						if (good_Diff == "1" && goodDiff(i,j) == 1) {
							if (tmp > 0) {
								fe_hist[0]->Fill(tmp);
								fe_hist[whichFE(j)+1]->Fill(tmp);
							}
							h_plot->SetBinContent(j+1,i+1,tmp);	
							pix_values.push_back(tmp);
						}
						else if (good_Diff == "1" && goodDiff(i,j) == 0) {
							h_plot->SetBinContent(j+1,i+1,-1);	
							pix_values.push_back(-1);
						}
					}
				}
			}

			char mean_All[100]={}, mean_Syn[100]={}, mean_Lin[100]={}, mean_Diff[100]={};
			char rms_All[100]={}, rms_Syn[100]={}, rms_Lin[100]={}, rms_Diff[100]={};
			char gmean_All[100]={}, gmean_Syn[100]={}, gmean_Lin[100]={}, gmean_Diff[100]={};
			char gsigma_All[100]={}, gsigma_Syn[100]={}, gsigma_Lin[100]={}, gsigma_Diff[100]={};
			char chidof_All[100]={}, chidof_Syn[100]={}, chidof_Lin[100]={}, chidof_Diff[100]={};

			char* mean_char[4] = {mean_All, mean_Syn, mean_Lin, mean_Diff};
			char* rms_char[4] = {rms_All, rms_Syn, rms_Lin, rms_Diff};
			char* gmean_char[4] = {gmean_All, gmean_Syn, gmean_Lin, gmean_Diff};
			char* gsigma_char[4] = {gsigma_All, gsigma_Syn, gsigma_Lin, gsigma_Diff};
			char* chidof_char[4] = {chidof_All, chidof_Syn, chidof_Lin, chidof_Diff};
			double mean_h[4];
			double rms_h[4]; 	
			double fit_par[4], fit_err[4], fit_range[8];
			double chisq_DOF[4];

			const char *LabelName[6] = {"1","2","3","4","5",">5"};	

			//For RD53A and Chip ID labels.
			TLatex *tname= new TLatex();
			tname->SetNDC();
			tname->SetTextAlign(22);
			tname->SetTextFont(73);
			tname->SetTextSizePixels(30);
			std::string rd53 = "RD53A Internal";

			//For the Mean and Sigma of the Gaussian Fit 
			TLatex *mean_rms = new TLatex;
			mean_rms->SetNDC();
			mean_rms->SetTextAlign(13);
			mean_rms->SetTextFont(63);
			mean_rms->SetTextSizePixels(24);

			TLegend* fe_legend[4];
			std::string legend_name[4] = {"All", "Synchronous", "Linear", "Differential"};					
			TF1* fe_fit[4];
			std::string fit_name[4] = {"fit_All", "fit_Syn", "fit_Lin", "fit_Diff"};	

			int color[4] = {800, 806, 824, 865}; //(kOrange, kOrange+6, kSpring+4, kAzure+5)

			//Create canvases
			std::string c_name[4]  = {"c_All", "c_Syn", "c_Lin", "c_Diff"};
			TCanvas* fe_c[4];

			std::string plot_ext[21] = {".dat", "_All.", "_SYN.", "_LIN.", "_DIFF.", "_PLOT.", "_STACK.", "_SYNrmsrange.", "_LINrmsrange.", "_DIFFrmsrange.", "_STACKrmsrange.", "_SYNgaussrange.", "_LINgaussrange.", "_DIFFgaussrange.", "_STACKgaussrange.", "_SYNrmsrangenorm.", "_LINrmsrangenorm.", "_DIFFrmsrangenorm.", "_SYNgaussrangenorm.", "_LINgaussrangenorm.", "_DIFFgaussrangenorm."};
			for (int i=1; i<21; i++) plot_ext[i].append(ext);

			//Plots for All, Synchronous, Linear, and Differential FEs
			for (int i=0; i<4; i++) {
				style_TH1(fe_hist[i], xaxistitle.c_str(), yaxistitle.c_str());
				fe_hist[i]->SetFillColor(color[i]);
				fe_hist[i]->SetLineColor(color[i]);
				fe_hist[i]->GetXaxis()->SetLabelSize(0.035);
				fe_hist[i]->SetStats(0);

				fe_c[i] = new TCanvas(c_name[i].c_str(), c_name[i].c_str(), 800, 600);
				style_TH1canvas(fe_c[i]);
				fe_hist[i]->Draw();

				//Write RD53A and Chip ID
				tname->DrawLatex(0.28,0.96, rd53.c_str());
				tname->DrawLatex(0.8, 0.96, chipnum.c_str());

				//Create legend
				fe_legend[i] = new TLegend(0.7, 0.82, 0.87, 0.91);
				fe_legend[i]->SetHeader("Analog FEs", "C");
				fe_legend[i]->AddEntry(fe_hist[i], legend_name[i].c_str(), "f");
				fe_legend[i]->SetBorderSize(0);
				fe_legend[i]->Draw();

				mean_h[i] = fe_hist[i]->GetMean();
				rms_h[i] = fe_hist[i]->GetRMS();

				//Fit plot with Gaussian; for single FE plots
				if (i == 0) {	
					fe_fit[i] = new TF1(fit_name[i].c_str(), "gaus", xlow, xhigh);
					fe_hist[i]->Fit(fit_name[i].c_str(), "0", "", xlow, xhigh);

				}	
				if (i > 0) {
					fe_fit[i] = new TF1(fit_name[i].c_str(), "gaus", mean_h[i]-rms_h[i], mean_h[i]+rms_h[i]);
					fe_hist[i]->Fit(fit_name[i].c_str(), "0", "", mean_h[i]-rms_h[i], mean_h[i]+rms_h[i]);			

				}

				//Change range and refit 5 times to get better fit.	
				for (int j=0; j<5; j++) {
					for (int k = 0; k<3; k++) {
						fit_par[k] = fe_fit[i]->GetParameter(k);
						fit_err[k] = fe_fit[i]->GetParError(k);
					}
					fit_range[2*i] = fit_par[1] - (2* fit_par[2]);
					fit_range[(2*i)+1] = fit_par[1] + (2* fit_par[2]);
					fe_hist[i]->Fit(fit_name[i].c_str(), "0", "", fit_range[(2*i)], fit_range[(2*i)+1] );
				}

				chisq_DOF[i] = (fe_fit[i]->GetChisquare())/(fe_fit[i]->GetNDF());

				//If Chi Squared divided by Degrees of Freedom is more than 10, warn user.
				fe_hist[i]->Fit(fit_name[i].c_str(), "B", "I", xlow, xhigh); //Plot over full range
				if (chisq_DOF[i] > 10)  std::cout<<"\n \033[1;38;5;202;5m Your threshold is crap. Choose a new threshold. \033[0m \n"<<std::endl;


				std::cout << "Chi^2 is "  << fe_fit[i]->GetChisquare() << "	DOF is " << fe_fit[i]->GetNDF() << "	Prob is " << fe_fit[i]->GetProb() << std::endl;

				//Write the Gaussian mean/sigma and the histogram mean/rms on the plot.
				sprintf(mean_char[i], "Mean_{hist} = %.1f", mean_h[i]);
				mean_rms->DrawLatex(0.18,0.91, mean_char[i]);
				sprintf(rms_char[i], "RMS_{hist} = %.1f", rms_h[i]);
				mean_rms->DrawLatex(0.18,0.86, rms_char[i]);
				sprintf(gmean_char[i], "Mean_{gaus} = %.1f #pm %.1f", fit_par[1], fit_err[1]);
				mean_rms->DrawLatex(0.18, 0.81, gmean_char[i]);
				sprintf(gsigma_char[i], "#sigma_{gaus} = %.1f #pm %.1f", fit_par[2], fit_err[2]);
				mean_rms->DrawLatex(0.18, 0.76, gsigma_char[i]);
				sprintf(chidof_char[i], "#chi^{2} / DOF = %.1f / %i", fe_fit[i]->GetChisquare(), fe_fit[i]->GetNDF());
				mean_rms->DrawLatex(0.18, 0.71, chidof_char[i]);	

				fe_hist[i]->GetYaxis()->SetRangeUser(0,((fe_hist[i]->GetMaximum())*1.5)); //Leave extra room for legend
				fe_hist[i]->GetXaxis()->SetRangeUser((mean_h[i] - 5*rms_h[i] < 0) ? -0.5 : (mean_h[i]- 5*rms_h[i]), mean_h[i] + 5*rms_h[i]); //Change the x-axis range to be the Mean +/- 5*RMS. If the lower bound is less than -0.5, make it -0.5. 
				fe_c[i]->Update();

				filename1 = filename.replace(filename.find(plot_ext[i].c_str()), 10, plot_ext[i+1].c_str()); 
				fe_c[i]->Print(filename1.c_str());
			}


			//Map of Pixels
			style_TH2(h_plot, "Column", "Row", xaxistitle.c_str()); 
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
			h_plot->GetZaxis()->SetRangeUser((mean_h[0] - 5*rms_h[0] < 0) ? -0.5 : (mean_h[0]- 5*rms_h[0])  , mean_h[0] + 5*rms_h[0]);	
			h_plot->GetZaxis()->SetLabelSize(0.04);
			filename2 = filename.replace(filename.find(plot_ext[4].c_str()), 10, plot_ext[5].c_str());
			c_plot->Print(filename2.c_str());

			//Remove fit lines from histograms for the stack plot
			for (int i=1; i<4; i++) fe_hist[i]->Fit(fit_name[i].c_str(), "0", "", fit_range[(2*i)], fit_range[(2*i)+1]);

			//Stack Plot for all 3 FEs
			THStack *hs = new THStack("hs","");
			for (int i=1; i<4; i++) hs->Add(fe_hist[i]);
			TCanvas *c_Stack = new TCanvas("c_Stack", "c_Stack", 800, 600);
			c_Stack->SetLeftMargin(0.15);
			c_Stack->SetRightMargin(0.05);
			c_Stack->SetBottomMargin(0.1225);
			hs->Draw(); 
		
			//Setting the title for THStack plots has to be after Draw(), and needs canvas->Modified() after
			style_THStack(hs, xaxistitle.c_str(), yaxistitle.c_str());
			hs->GetXaxis()->SetLabelSize(0.035);
			gPad->SetLogy(0);
			c_Stack->Modified();
			gStyle->SetOptStat(0);
			
			TLegend *stack_legend = new TLegend(0.7, 0.7, 0.87, 0.91);
			stack_legend->SetHeader("Analog FEs", "C");
			for (int i=1; i<4; i++) stack_legend->AddEntry(fe_hist[i], legend_name[i].c_str(), "f"); 
			stack_legend->SetBorderSize(0);
			stack_legend->Draw();
			tname->DrawLatex(0.28,0.96, rd53.c_str());
			tname->DrawLatex(0.8, 0.96, chipnum.c_str());

			if ( chisq_DOF[0] < 10 ) {
				std::cout << "\n Threshold is acceptable. Plotting fit. \n" << std::endl;
				fe_fit[0]->DrawF1(xlow, xhigh, "SAME");
			} 
			else std::cout<<"\n \033[1;38;5;202;5m Your threshold is crap. Choose a new threshold. \033[0m \n"<<std::endl;

			//Write RD53A Internal and the Chip ID
			tname->DrawLatex(0.28,0.96, rd53.c_str());
			tname->DrawLatex(0.8, 0.96, chipnum.c_str());
			
			//Write the Gaussian mean/sigma and the histogram mean/rms on the plot.
			mean_rms->DrawLatex(0.18,0.91, mean_char[0]);
			mean_rms->DrawLatex(0.18,0.86, rms_char[0]);

			hs->SetMaximum((hs->GetMaximum())*1.1);
			hs->GetXaxis()->SetRangeUser((mean_h[0] - 5*rms_h[0] < 0) ? -0.5 : (mean_h[0]- 5*rms_h[0])  , mean_h[0] + 5*rms_h[0]);	
			c_Stack->Update();				
			filename3 = filename.replace(filename.find(plot_ext[5].c_str()), 11, plot_ext[6].c_str());
			c_Stack->Print(filename3.c_str());


			int zeros_FE[3] = { 0, 0, 0};
			int n=0;
			char zeros_Syn[100]={}, zeros_Lin[100]={}, zeros_Diff[100]={};
			char* zeros_char[3] = {zeros_Syn, zeros_Lin, zeros_Diff};

			//Fill range histograms
			for (int i=0; i<rowno; i++) {
				for (int j=0; j<colno; j++) {
					double *tmp_p = &pix_values[n];
					n++;
					double tmp = *tmp_p;		
					if (tmp != -1) {
						int bin_rms = whichSigma(tmp, mean_h[whichFE(j)+1], rms_h[whichFE(j+1)]);
						range_rms[whichFE(j)]->AddBinContent(bin_rms);
						
						int bin_gaus = whichSigma(tmp, fe_fit[whichFE(j)+1]->GetParameter(1), fe_fit[whichFE(j)+1]->GetParameter(2));
						range_gaus[whichFE(j)]->AddBinContent(bin_gaus);

					}
					if (tmp == 0) zeros_FE[whichFE(j)]++;	
				}
			}

			std::cout << "\n \n \n Number of Zeros are	 " << zeros_FE[0] << "	" << zeros_FE[1] << "	" << zeros_FE[2] << std::endl;

			//Label for untuned pixels
			TLatex *zeros = new TLatex();
			zeros->SetNDC();
			zeros->SetTextAlign(13);
			zeros->SetTextFont(63);
			zeros->SetTextSizePixels(20);

			std::string crms_name[3]  = {"crms_Syn", "crms_Lin", "crms_Diff"};
			TCanvas* rmsrange_c[3];
			TLegend* rmsrange_legend[3];

			//Plot Range histograms for Synchronous, Linear, and Differential FEs.	
			for (int i=0; i<3; i++) {

				style_TH1(range_rms[i], rmsrangetitle.c_str(), yaxistitle.c_str());
				for (int j=1; j<=range_bins; j++) range_rms[i]->GetXaxis()->SetBinLabel(j, LabelName[j-1]);
				range_rms[i]->GetXaxis()->LabelsOption("h");
				range_rms[i]->SetFillColor(color[i+1]);
				range_rms[i]->SetLineColor(color[i+1]);
				range_rms[i]->SetStats(0);

				rmsrange_c[i] = new TCanvas(crms_name[i].c_str(), crms_name[i].c_str(), 800, 600);
				style_TH1canvas(rmsrange_c[i]);
				range_rms[i]->Draw();
				range_rms[i]->SetMarkerSize(1.8);
				range_rms[i]->SetMarkerColor(1);
				range_rms[i]->Draw("TEXT0 SAME");	

				tname->DrawLatex(0.28,0.96, rd53.c_str());
				tname->DrawLatex(0.8, 0.96, chipnum.c_str());

				sprintf(zeros_char[i], "Untuned Pixels = %i", zeros_FE[i]);
				zeros->DrawLatex(0.18,0.91, zeros_char[i]);

				rmsrange_legend[i] = new TLegend(0.7, 0.82, 0.87, 0.91);
				rmsrange_legend[i]->SetHeader("Analog FEs", "C");
				rmsrange_legend[i]->AddEntry(range_rms[i], legend_name[i+1].c_str(), "f");
				rmsrange_legend[i]->SetBorderSize(0);
				rmsrange_legend[i]->Draw();

				range_rms[i]->GetYaxis()->SetRangeUser(0,((range_rms[i]->GetMaximum())*1.25));
				rmsrange_c[i]->Update();

				filename4 = filename.replace(filename.find(plot_ext[i+6].c_str()), 18, plot_ext[i+7].c_str()); 
				rmsrange_c[i]->Print(filename4.c_str());

			}

			//Stacked Range Plot
			THStack *hs_rmsrange = new THStack("hs","");
			for (int i=0; i<3; i++) hs_rmsrange->Add(range_rms[i]);
			TCanvas *c_Stackrms = new TCanvas("c_Stackrms", "c_Stackrms", 800, 600);
			c_Stackrms->SetLeftMargin(0.15);
			c_Stackrms->SetRightMargin(0.05);
			c_Stackrms->SetBottomMargin(0.1225);
			hs_rmsrange->Draw(); 
			//Setting the title for THStack plots has to be after Draw(), and needs canvas->Modified() after
			style_THStack(hs_rmsrange, rmsrangetitle.c_str(), yaxistitle.c_str());
			gPad->SetLogy(0);
			c_Stackrms->Modified();
			gStyle->SetOptStat(0);
			
			TLegend *stackrms_legend = new TLegend(0.33,0.82,0.93,0.91);
			stackrms_legend->SetNColumns(3);
			stackrms_legend->SetHeader("Analog FEs", "C");
			for (int i=0; i<3; i++) stackrms_legend->AddEntry(range_rms[i], legend_name[i+1].c_str(), "f"); 
			stackrms_legend->SetBorderSize(0);
			stackrms_legend->Draw();
			
			tname->DrawLatex(0.28,0.96, rd53.c_str());
			tname->DrawLatex(0.8, 0.96, chipnum.c_str());
			
			hs_rmsrange->GetYaxis()->SetRangeUser(0,((hs_rmsrange->GetMaximum())*1.21));
			c_Stackrms->Update();
			filename5 = filename.replace(filename.find(plot_ext[9].c_str()), 19, plot_ext[10].c_str());
			c_Stackrms->Print(filename5.c_str());

			std::string cgaus_name[3]  = {"cgaus_Syn", "cgaus_Lin", "cgaus_Diff"};
			TCanvas* gausrange_c[3];
			TLegend* gausrange_legend[3];

			//Plot Gaus Range histograms for Synchronous, Linear, and Differential FEs.	
			for (int i=0; i<3; i++) {

				style_TH1(range_gaus[i], gausrangetitle.c_str(), yaxistitle.c_str());
				for (int j=1; j<=range_bins; j++) range_gaus[i]->GetXaxis()->SetBinLabel(j, LabelName[j-1]);
				range_gaus[i]->GetXaxis()->LabelsOption("h");
				range_gaus[i]->SetFillColor(color[i+1]);
				range_gaus[i]->SetLineColor(color[i+1]);
				range_gaus[i]->SetStats(0);

				gausrange_c[i] = new TCanvas(cgaus_name[i].c_str(), cgaus_name[i].c_str(), 800, 600);
				style_TH1canvas(gausrange_c[i]);
				range_gaus[i]->Draw();
				range_gaus[i]->SetMarkerSize(1.8);
				range_gaus[i]->SetMarkerColor(1);
				range_gaus[i]->Draw("TEXT0 SAME");	

				tname->DrawLatex(0.28,0.96, rd53.c_str());
				tname->DrawLatex(0.8, 0.96, chipnum.c_str());

				sprintf(zeros_char[i], "Untuned Pixels = %i", zeros_FE[i]);
				zeros->DrawLatex(0.18,0.91, zeros_char[i]);

				gausrange_legend[i] = new TLegend(0.7, 0.82, 0.87, 0.91);
				gausrange_legend[i]->SetHeader("Analog FEs", "C");
				gausrange_legend[i]->AddEntry(range_gaus[i], legend_name[i+1].c_str(), "f");
				gausrange_legend[i]->SetBorderSize(0);
				gausrange_legend[i]->Draw();

				range_gaus[i]->GetYaxis()->SetRangeUser(0,((range_gaus[i]->GetMaximum())*1.25));
				gausrange_c[i]->Update();

				filename4 = filename.replace(filename.find(plot_ext[i+10].c_str()), 19, plot_ext[i+11].c_str()); 
				gausrange_c[i]->Print(filename4.c_str());

			}

			//Stacked Range Plot
			THStack *hs_gausrange = new THStack("hs","");
			for (int i=0; i<3; i++) hs_gausrange->Add(range_gaus[i]);
			TCanvas *c_Stackgaus = new TCanvas("c_Stackgaus", "c_Stackgaus", 800, 600);
			c_Stackgaus->SetLeftMargin(0.15);
			c_Stackgaus->SetRightMargin(0.05);
			c_Stackgaus->SetBottomMargin(0.1225);
			hs_gausrange->Draw(); 
			//Setting the title for THStack plots has to be after Draw(), and needs canvas->Modified() after
			style_THStack(hs_gausrange, gausrangetitle.c_str(), yaxistitle.c_str());
			gPad->SetLogy(0);
			c_Stackgaus->Modified();
			gStyle->SetOptStat(0);
			TLegend *stackgaus_legend = new TLegend(0.33,0.82,0.93,0.91);
			stackgaus_legend->SetNColumns(3);
			stackgaus_legend->SetHeader("Analog FEs", "C");
			for (int i=0; i<3; i++) stackgaus_legend->AddEntry(range_gaus[i], legend_name[i+1].c_str(), "f"); 
			stackgaus_legend->SetBorderSize(0);
			stackgaus_legend->Draw();
			tname->DrawLatex(0.28,0.96, rd53.c_str());
			tname->DrawLatex(0.8, 0.96, chipnum.c_str());
			hs_gausrange->GetYaxis()->SetRangeUser(0,((hs_gausrange->GetMaximum())*1.21));
			c_Stackgaus->Update();
			filename5 = filename.replace(filename.find(plot_ext[13].c_str()), 20, plot_ext[14].c_str());
			c_Stackgaus->Print(filename5.c_str());

			//Normalized Plots for RMS Range
			TH1* rms_norm[3];
			TCanvas* rmsnorm_c[3];
			std::string rmsnorm_names[3] = {"rmsn_SYN", "rmsn_LIN", "rmsn_DIFF"}; 
			
			for (int i=0; i<3; i++) {
				rmsnorm_c[i] = new TCanvas(rmsnorm_names[i].c_str(), rmsnorm_names[i].c_str(), 800, 600);
				style_TH1canvas(rmsnorm_c[i]);
				rms_norm[i] = range_rms[i]->DrawNormalized();
				rms_norm[i]->Draw();
				
				gStyle->SetPaintTextFormat(".4f");	
				rms_norm[i]->SetMarkerSize(1.8);
				rms_norm[i]->SetMarkerColor(1);
				rms_norm[i]->Draw("TEXT0 SAME");	
			
				tname->DrawLatex(0.28,0.96, rd53.c_str());
				tname->DrawLatex(0.8, 0.96, chipnum.c_str());
				
				rmsrange_legend[i]->Draw();

				rms_norm[i]->GetYaxis()->SetRangeUser(0,1.05);
				rmsnorm_c[i]->Update();

				filename6 = filename.replace(filename.find(plot_ext[i+14].c_str()), 20, plot_ext[i+15].c_str()); 
				rmsnorm_c[i]->Print(filename6.c_str());
			}

			//Normalized Plots for Gaussian Range
			TH1* gaus_norm[3];
			TCanvas* gausnorm_c[3];
			std::string gausnorm_names[3] = {"gausn_SYN", "gausn_LIN", "gausn_DIFF"}; 
			
			for (int i=0; i<3; i++) {
				gausnorm_c[i] = new TCanvas(gausnorm_names[i].c_str(), gausnorm_names[i].c_str(), 800, 600);
				style_TH1canvas(gausnorm_c[i]);
				gaus_norm[i] = range_gaus[i]->DrawNormalized();
				gaus_norm[i]->Draw();
				
				gStyle->SetPaintTextFormat(".4f");	
				gaus_norm[i]->SetMarkerSize(1.8);
				gaus_norm[i]->SetMarkerColor(1);
				gaus_norm[i]->Draw("TEXT0 SAME");	
			
				tname->DrawLatex(0.28,0.96, rd53.c_str());
				tname->DrawLatex(0.8, 0.96, chipnum.c_str());
				
				gausrange_legend[i]->Draw();

				gaus_norm[i]->GetYaxis()->SetRangeUser(0,1.05);
				gausnorm_c[i]->Update();

				filename6 = filename.replace(filename.find(plot_ext[i+17].c_str()), 22, plot_ext[i+18].c_str()); 
				gausnorm_c[i]->Print(filename6.c_str());
			}


			float outlierSpread = meanDeviation(pix_values);
			std::cout << "\033[1;31m Range of values (excluding 0.997 or 0.95 times the mean.) : \033[0m" << outlierSpread << std::endl; 

			for (int i=0; i<4; i++) {
				delete fe_hist[i];
				delete fe_c[i];
				if (i<3) {
					delete range_rms[i];
					delete rmsrange_c[i];
					delete range_gaus[i];
					delete gausrange_c[i];
					delete rms_norm[i];
					delete rmsnorm_c[i];
					delete gaus_norm[i];
					delete gausnorm_c[i];	
				}
			}

			delete h_plot;
			delete c_plot;
			delete hs;
			delete c_Stack;
			delete hs_rmsrange;
			delete c_Stackrms;
			delete hs_gausrange;
			delete c_Stackgaus;

		}
	}

	return 0;
} 
