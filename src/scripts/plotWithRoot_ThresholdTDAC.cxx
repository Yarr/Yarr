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
#include <json.hpp>

using json=nlohmann::basic_json<std::map, std::vector, std::string, bool, std::int32_t, std::uint32_t, float>;


int main(int argc, char *argv[]) { //./plotWithRoot_ThresholdTDAC path/to/directory file_ext
	//Example file extensions: png, pdf, C, root	

	SetRD53Style();

	if (argc < 4) {
		std::cout << "No directory and/or image plot extension given! \nExample: ./plotWithRoot_ThresholdTDAC path/to/directory/ pdf 1" << std::endl;
		return -1;
	}

	int thr_found=0;
	int rowno, colno, total_pix;
	int xbins, xlow, xhigh;
	std::string chipnum, filename, filename1, filename2;
	rowno = 192;
	colno = 400;
	total_pix = rowno*colno;
	xbins = 501;
	xlow = -10;
	xhigh = 10010;

	std::vector <double> thr_values;
	std::vector <int> tdac_values;

	std::string dir, filepath, file_name;
	DIR *dp;
	struct dirent *dirp;
	struct stat filestat;

	std::string delimiter = "_";
	std::string ext = argv[2];
	std::string good_Diff = argv[3];

	dp = opendir(argv[1]);	//open directory
	if (dp==NULL) { 	//if directory doesn't exist
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

		//Open ThresholdMap.dat file
		if ( thr_found != 1) { //if Threshold Map has not been found
			if (strstr(file_path, "ThresholdMap-0.dat") != NULL) {	//if filename contains string declared in argument
				thr_found = 1;
				chipnum = "Chip SN: " + file_name.substr(0, file_name.find(delimiter)); //get chip # from file name

				std::cout << "Opening file: " << filepath.c_str() << std::endl;
				filename = filepath.c_str();
				std::fstream infile(filepath.c_str(), std::ios::in);

				std::string type;
				std::string name;

				std::string xaxistitle, yaxistitle, xrangetitle;
				std::string line;	

				std::getline(infile, type);
				std::getline(infile, name);
				std::getline(infile, line); //skip "Column"
				std::getline(infile, line); //skip "Rows"
				std::getline(infile, line); //skip "Hits"
				std::getline(infile, line); //skip x range
				std::getline(infile, line); // skip y range

				int underflow, overflow;

				infile >> underflow >> overflow;

				if (!infile) {
					std::cout << "Something wrong with file ..." << std::endl;
					return -1;
				}

				//Fill Threshold array	
				for (int i=0; i<rowno; i++) {
					for (int j=0; j<colno; j++) {
						double tmp;
						infile >> tmp;
						thr_values.push_back(tmp);
					}
				}
			}	
		}

		//Open .json.after file
		if ( strstr(file_path, ".json.after") != NULL && strstr(file_path, "rd53a") !=NULL ) { //if filename contains string declared in argument.
			if (strstr(file_path, "before_") != NULL || strstr(file_path, "after_") != NULL) continue; 
			else {
				std::cout << "Opening file: " << filepath.c_str() << std::endl;
				std::string filename = filepath.c_str();
				std::fstream cfgfile(filepath.c_str(), std::ios::in);
				json cfg;
				cfgfile >> cfg;

				for (int i=0; i<rowno; i++) {
					for (int j=0; j<colno; j++) {
						int tmp;
						tmp = cfg["RD53A"]["PixelConfig"][j]["TDAC"][i];
						tdac_values.push_back(tmp);
					}
				}	
			}
		}	

	}

	//Number of TDACs in Linear and Differential FE.
	int lin_plot=16, diff_plot=31;		

    TH2F *corr_diff = new TH2F("corr_diff", "", xbins, xlow, xhigh, 31, -15.5, 15.5);

	//Create Plots to get range
	TH1* lin_all = NULL;
	lin_all =  (TH1*) new TH1F("lin_all", "", xbins, xlow, xhigh);

	TH1* diff_all = NULL;
	diff_all =  (TH1*) new TH1F("diff_all", "", xbins, xlow, xhigh);

	//Create Plots -- 1 for each TDAC value
	std::string lin_names[lin_plot] = {"lin0", "lin1", "lin2", "lin3", "lin4", "lin5", "lin6", "lin7", "lin8", "lin9", "lin10", "lin11", "lin12", "lin13", "lin14", "lin15"};
	TH1* histLin[lin_plot];
	for (int i=0; i<lin_plot;i++) histLin[i] = new TH1F(lin_names[i].c_str(), "", xbins, xlow, xhigh);	
	
	std::string diff_names[diff_plot] = {"diffn15", "diffn14", "diffn13", "diffn12", "diffn11", "diffn10", "diffn9", "diffn8", "diffn7", "diffn6", "diffn5", "diffn4", "diffn3", "diffn2", "diffn1", "diff0", "diff1", "diff2", "diff3", "diff4", "diff5", "diff6", "diff7", "diff8", "diff9", "diff10", "diff11", "diff12", "diff13", "diff14", "diff15"};
	TH1* histDiff[diff_plot];
	for (int i=0; i<diff_plot; i++) histDiff[i] = new TH1F(diff_names[i].c_str(),"", xbins, xlow, xhigh);

	//Write Legend labels
	std::string lin_num[lin_plot] = {"0","1","2","3","4","5","6","7","8","9","10","11","12","13","14","15"};
	std::string diff_num[diff_plot] = {"-15", "-14", "-13", "-12", "-11", "-10", "-9", "-8", "-7", "-6", "-5", "-4", "-3", "-2", "-1", "0","1","2","3","4","5","6","7","8","9","10","11","12","13","14","15"};

	int num_thr_values = thr_values.size();
	int num_tdac_values = tdac_values.size(); 

	if ( num_thr_values == total_pix && num_tdac_values == total_pix ) { 	
		int zero_Lin=0, zero_Diff=0, n=0;
		//Fill histogram
		for (int i=0; i<rowno; i++) {
			for (int j=0; j<colno; j++) {
				double thr_v = thr_values[n];
				int tdac_v = tdac_values[n];

				//Fill Linear Plots
				if (whichFE(j)==1) {
					if (thr_v == 0) zero_Lin++;
					histLin[tdac_v]->Fill(thr_v);
					lin_all->Fill(thr_v);	
				}

				//Fill Differential Plots
				if (whichFE(j)==2) {
                    if ((good_Diff == "1" && goodDiff(i, j)) || (good_Diff == "0")) {
                        if (thr_v == 0) zero_Diff++;
                        histDiff[tdac_v+15]->Fill(thr_v);
                        diff_all->Fill(thr_v);
                        corr_diff->Fill(thr_v, tdac_v);
                    }
				}
				
				n++;
			}
		}

		std::cout << " Number of Zeros are	Linear:" << zero_Lin << "	Differential:" << zero_Diff << std::endl;


		for (int i=0; i<diff_plot; i++) {
			histDiff[i]->SetLineColor(1);
			histDiff[i]->SetLineWidth(1);
			if (i<lin_plot) {
				histLin[i]->SetLineColor(1);
				histLin[i]->SetLineWidth(1);
			}
		}

		std::string rd53 = "RD53A Internal";
		
        TLatex *tname= new TLatex();
		tname->SetNDC();
		tname->SetTextAlign(22);
		tname->SetTextFont(73);
		tname->SetTextSizePixels(30);
        
        TCanvas *c_corr_diff = new TCanvas("c_corr_diff", "c_corr_diff", 800, 600);
		c_corr_diff->SetLeftMargin(0.15);
		c_corr_diff->SetRightMargin(0.2);
		c_corr_diff->SetBottomMargin(0.15);
        corr_diff->GetXaxis()->SetTitle("Threshold [e]");
        corr_diff->GetYaxis()->SetTitle("TDAC");
        corr_diff->GetZaxis()->SetTitle("# of Pixels");
		corr_diff->GetXaxis()->SetLabelSize(0.04);
		corr_diff->GetYaxis()->SetLabelSize(0.04);
		corr_diff->GetXaxis()->SetRangeUser((diff_all->GetMean() - 2*(diff_all->GetRMS())), (diff_all->GetMean() + 2*(diff_all->GetRMS()))); //Set the x-axis range to be the Mean +/- 2*RMS
        corr_diff->Draw("colz");
		tname->DrawLatex(0.28,0.96, rd53.c_str());
		tname->DrawLatex(0.8, 0.96, chipnum.c_str());
        std::string filename0 = filename.substr(0, filename.find(".dat")); 
        c_corr_diff->Print((filename0 + "_DIFFThrTDACcorr." + ext).c_str());
        

		//Linear FE Threshold TDAC plot
		THStack *hs_Lin = new THStack("hs_Lin","");
		gStyle->SetPalette(kRainBow);
		for (int i=0; i<lin_plot; i++) hs_Lin->Add(histLin[i]);
		TCanvas *cs_Lin = new TCanvas("cs_Lin", "cs_Lin", 800, 600);
		cs_Lin->SetLeftMargin(0.15);
		cs_Lin->SetRightMargin(0.10);
		cs_Lin->SetBottomMargin(0.1225);
		//hs_Lin->Draw();
		hs_Lin->Draw("PFC"); //Fill using the colours in the chosen palette 
		//Setting the title for THStack plots has to be after Draw(), and needs canvas->Modified() after
		style_THStack(hs_Lin, "Threshold [e]", "Number of Pixels");
		hs_Lin->GetXaxis()->SetLabelSize(0.065);
		hs_Lin->GetYaxis()->SetLabelSize(0.045);
		gPad->SetLogy(0);
		cs_Lin->Modified();
		hs_Lin->GetXaxis()->SetLabelSize(0.04);
		hs_Lin->GetYaxis()->SetLabelSize(0.04);

		TLegend *hs_Lin_legend = new TLegend(0.91,0.12, 0.99,0.93);
		hs_Lin_legend->SetHeader("TDAC","");
		for(int i=lin_plot-1; i>-1; i--) hs_Lin_legend->AddEntry(histLin[i], lin_num[i].c_str(),"f");
		hs_Lin_legend->SetBorderSize(0);
		hs_Lin_legend->SetTextSize(0.04);
		hs_Lin_legend->Draw();

		
		//hs_Lin->SetMaximum((hs_Lin->GetMaximum())*1.15);
		hs_Lin->GetXaxis()->SetRangeUser((lin_all->GetMean() - 2*(lin_all->GetRMS())), (lin_all->GetMean() + 2*(lin_all->GetRMS()))); //Set the x-axis range to be the Mean +/- 2*RMS
		cs_Lin->Update();
		std::string lin_ext = "_LINThrTDAC."; 
		filename1 = filename.replace(filename.find(".dat"), 16, lin_ext.append(ext)); 
		cs_Lin->Print(filename1.c_str());

		//Differential FE Threshold TDAC Plot
		THStack *hs_Diff = new THStack("hs_Diff","");
		gStyle->SetPalette(kBird);
		for (int i=0; i<diff_plot; i++) hs_Diff->Add(histDiff[i]);
		TCanvas *cs_Diff = new TCanvas("cs_Diff", "cs_Diff", 800, 600);
		cs_Diff->SetLeftMargin(0.15);
		cs_Diff->SetRightMargin(0.10);
		cs_Diff->SetBottomMargin(0.1225);
		//hs_Diff->Draw();
		hs_Diff->Draw("PFC"); 
		//Setting the title for THStack plots has to be after Draw(), and needs canvas->Modified() after
		style_THStack(hs_Diff, "Threshold [e]", "Number of Pixels");
		hs_Diff->GetXaxis()->SetLabelSize(0.065);
		hs_Diff->GetYaxis()->SetLabelSize(0.045);
		gPad->SetLogy(0);
		cs_Diff->Modified();
		hs_Diff->GetXaxis()->SetLabelSize(0.04);
		hs_Diff->GetYaxis()->SetLabelSize(0.04);

		TLegend *hs_Diff_legend = new TLegend(0.91,0.12, 0.99,0.93);
		hs_Diff_legend->SetHeader("TDAC","");
		for(int i=diff_plot-1; i>-1; i--) hs_Diff_legend->AddEntry(histDiff[i], diff_num[i].c_str(),"f");
		hs_Diff_legend->SetBorderSize(0);
		hs_Diff_legend->SetTextSize(0.03);
		hs_Diff_legend->Draw();

		tname->DrawLatex(0.28,0.96, rd53.c_str());
		tname->DrawLatex(0.8, 0.96, chipnum.c_str());
		//hs_Diff->SetMaximum((hs_Diff->GetMaximum())*1.15);
		hs_Diff->GetXaxis()->SetRangeUser((diff_all->GetMean() - 2*(diff_all->GetRMS())), (diff_all->GetMean() + 2*(diff_all->GetRMS())));
		cs_Diff->Update();
		std::string diff_ext = "_DIFFThrTDAC.";
		filename2 = filename.replace(filename.find(lin_ext), 17, diff_ext.append(ext)); 
		cs_Diff->Print(filename2.c_str());

		for (int i=0; i<diff_plot; i++) {
			delete histDiff[i];
			if (i<lin_plot) delete histLin[i];
		}

		delete hs_Lin;
		delete cs_Lin;
		delete hs_Diff;
		delete cs_Diff;
	}

	else { 
		std::cout << "Expected data not found. Skipping plots." << std::endl;
	}

	return 0;
} 
