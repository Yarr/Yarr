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


int main(int argc, char *argv[]) { //./plotWithRoot_ThresholdTDAC path/to/directory

	SetRD53Style();

	int thr_found=0;
	int rowno, colno, total_pix;
	int xbins, xlow, xhigh, xyzero;
	std::string chipnum, filename, filename1, filename2;
	rowno = 192;
	colno = 400;
	total_pix = rowno*colno;
	xyzero = 0;
	xbins = 500;
	xlow = -0.5;
	xhigh = 10000.5;

	std::vector <double> thr_values;
	std::vector <int> tdac_values;

	if (argc < 2) {
		std::cout << "No directory given!" << std::endl;
		return -1;
	}

	std::string dir, filepath, file_name;
	DIR *dp;
	struct dirent *dirp;
	struct stat filestat;

	std::string delimiter = "_";

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
			if (strstr(file_path, "ThresholdMap.dat") != NULL) {	//if filename contains string declared in argument
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

				//Fill Threshold plots	
				for (int i=0; i<rowno; i++) {
					for (int j=0; j<colno; j++) {
						double tmp;
						infile >> tmp;
						//std::cout << i*j << " " << tmp << std::endl;
						thr_values.push_back(tmp);
					}
				}
			}	
		}

		//Open .json.after file
		if ( strstr( file_path, "json.after") != NULL) { //if filename contains string declared in argument.
			if (strstr(file_path, ".pdf") != NULL) continue; 
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


	int lin_plot=16, diff_plot=31;		

	TH1* lin_all = NULL;
	lin_all =  (TH1*) new TH1F("lin_all", "", xbins, xlow, xhigh);
	TH1* lin0 = NULL;
	lin0 =  (TH1*) new TH1F("lin0", "", xbins, xlow, xhigh);
	TH1* lin1 = NULL;
	lin1 =  (TH1*) new TH1F("lin1", "", xbins, xlow, xhigh);
	TH1* lin2 = NULL;
	lin2 =  (TH1*) new TH1F("lin2", "", xbins, xlow, xhigh);
	TH1* lin3 = NULL;
	lin3 =  (TH1*) new TH1F("lin3", "", xbins, xlow, xhigh);
	TH1* lin4 = NULL;
	lin4 =  (TH1*) new TH1F("lin4", "", xbins, xlow, xhigh);
	TH1* lin5 = NULL;
	lin5 =  (TH1*) new TH1F("lin5", "", xbins, xlow, xhigh);
	TH1* lin6 = NULL;
	lin6 =  (TH1*) new TH1F("lin6", "", xbins, xlow, xhigh);
	TH1* lin7 = NULL;
	lin7 =  (TH1*) new TH1F("lin7", "", xbins, xlow, xhigh);
	TH1* lin8 = NULL;
	lin8 =  (TH1*) new TH1F("lin8", "", xbins, xlow, xhigh);
	TH1* lin9 = NULL;
	lin9 =  (TH1*) new TH1F("lin9", "", xbins, xlow, xhigh);
	TH1* lin10 = NULL;
	lin10 =  (TH1*) new TH1F("lin10", "", xbins, xlow, xhigh);
	TH1* lin11 = NULL;
	lin11 =  (TH1*) new TH1F("lin11", "", xbins, xlow, xhigh);
	TH1* lin12 = NULL;
	lin12 =  (TH1*) new TH1F("lin12", "", xbins, xlow, xhigh);
	TH1* lin13 = NULL;
	lin13 =  (TH1*) new TH1F("lin13", "", xbins, xlow, xhigh);
	TH1* lin14 = NULL;
	lin14 =  (TH1*) new TH1F("lin14", "", xbins, xlow, xhigh);
	TH1* lin15 = NULL;
	lin15 =  (TH1*) new TH1F("lin15", "", xbins, xlow, xhigh);

	TH1* diff_all = NULL;
	diff_all =  (TH1*) new TH1F("diff_all", "", xbins, xlow, xhigh);
	TH1* diffn1 = NULL;
	diffn1 =  (TH1*) new TH1F("diffn1", "", xbins, xlow, xhigh);
	TH1* diffn2 = NULL;
	diffn2 =  (TH1*) new TH1F("diffn2", "", xbins, xlow, xhigh);
	TH1* diffn3 = NULL;
	diffn3 =  (TH1*) new TH1F("diffn3", "", xbins, xlow, xhigh);
	TH1* diffn4 = NULL;
	diffn4 =  (TH1*) new TH1F("diffn4", "", xbins, xlow, xhigh);
	TH1* diffn5 = NULL;
	diffn5 =  (TH1*) new TH1F("diffn5", "", xbins, xlow, xhigh);
	TH1* diffn6 = NULL;
	diffn6 =  (TH1*) new TH1F("diffn6", "", xbins, xlow, xhigh);
	TH1* diffn7 = NULL;
	diffn7 =  (TH1*) new TH1F("diffn7", "", xbins, xlow, xhigh);
	TH1* diffn8 = NULL;
	diffn8 =  (TH1*) new TH1F("diffn8", "", xbins, xlow, xhigh);
	TH1* diffn9 = NULL;
	diffn9 =  (TH1*) new TH1F("diffn9", "", xbins, xlow, xhigh);
	TH1* diffn10 = NULL;
	diffn10 =  (TH1*) new TH1F("diffn10", "", xbins, xlow, xhigh);
	TH1* diffn11 = NULL;
	diffn11 =  (TH1*) new TH1F("diffn11", "", xbins, xlow, xhigh);
	TH1* diffn12 = NULL;
	diffn12 =  (TH1*) new TH1F("diffn12", "", xbins, xlow, xhigh);
	TH1* diffn13 = NULL;
	diffn13 =  (TH1*) new TH1F("diffn13", "", xbins, xlow, xhigh);
	TH1* diffn14 = NULL;
	diffn14 =  (TH1*) new TH1F("diffn14", "", xbins, xlow, xhigh);
	TH1* diffn15 = NULL;
	diffn15 =  (TH1*) new TH1F("diffn15", "", xbins, xlow, xhigh);
	TH1* diff0 = NULL;
	diff0 =  (TH1*) new TH1F("diff0", "", xbins, xlow, xhigh);
	TH1* diff1 = NULL;
	diff1 =  (TH1*) new TH1F("diff1", "", xbins, xlow, xhigh);
	TH1* diff2 = NULL;
	diff2 =  (TH1*) new TH1F("diff2", "", xbins, xlow, xhigh);
	TH1* diff3 = NULL;
	diff3 =  (TH1*) new TH1F("diff3", "", xbins, xlow, xhigh);
	TH1* diff4 = NULL;
	diff4 =  (TH1*) new TH1F("diff4", "", xbins, xlow, xhigh);
	TH1* diff5 = NULL;
	diff5 =  (TH1*) new TH1F("diff5", "", xbins, xlow, xhigh);
	TH1* diff6 = NULL;
	diff6 =  (TH1*) new TH1F("diff6", "", xbins, xlow, xhigh);
	TH1* diff7 = NULL;
	diff7 =  (TH1*) new TH1F("diff7", "", xbins, xlow, xhigh);
	TH1* diff8 = NULL;
	diff8 =  (TH1*) new TH1F("diff8", "", xbins, xlow, xhigh);
	TH1* diff9 = NULL;
	diff9 =  (TH1*) new TH1F("diff9", "", xbins, xlow, xhigh);
	TH1* diff10 = NULL;
	diff10 =  (TH1*) new TH1F("diff10", "", xbins, xlow, xhigh);
	TH1* diff11 = NULL;
	diff11 =  (TH1*) new TH1F("diff11", "", xbins, xlow, xhigh);
	TH1* diff12 = NULL;
	diff12 =  (TH1*) new TH1F("diff12", "", xbins, xlow, xhigh);
	TH1* diff13 = NULL;
	diff13 =  (TH1*) new TH1F("diff13", "", xbins, xlow, xhigh);
	TH1* diff14 = NULL;
	diff14 =  (TH1*) new TH1F("diff14", "", xbins, xlow, xhigh);
	TH1* diff15 = NULL;
	diff15 =  (TH1*) new TH1F("diff15", "", xbins, xlow, xhigh);

	TH1* lin_hist[lin_plot]={lin0, lin1, lin2, lin3, lin4, lin5, lin6, lin7, lin8, lin9, lin10, lin11, lin12, lin13, lin14, lin15};
	TH1* diff_hist[diff_plot]={diffn15, diffn14, diffn13, diffn12, diffn11, diffn10, diffn9, diffn8, diffn7, diffn6, diffn5, diffn4, diffn3, diffn2, diffn1, diff0, diff1, diff2, diff3, diff4, diff5, diff6, diff7, diff8, diff9, diff10, diff11, diff12, diff13, diff14, diff15};

	std::string lin_num[lin_plot] = {"0","1","2","3","4","5","6","7","8","9","10","11","12","13","14","15"};
	std::string diff_num[diff_plot] = {"-15", "-14", "-13", "-12", "-11", "-10", "-9", "-8", "-7", "-6", "-5", "-4", "-3", "-2", "-1", "0","1","2","3","4","5","6","7","8","9","10","11","12","13","14","15"};


	std::cout << thr_found << "		" <<  thr_values.size() << "	" << tdac_values.size() << std::endl;

	if ( thr_values.size() == total_pix && tdac_values.size() == total_pix ) { 	
		int zero_Lin=0, zero_Diff=0, n=0;
		//Fill histogram
		for (int i=0; i<rowno; i++) {
			for (int j=0; j<colno; j++) {
				double thr_v = thr_values[n];
				int tdac_v = tdac_values[n];

				//Fill Linear Plots
				if (whichFE(j)==1) {
					if (thr_v == 0) zero_Lin++;
					lin_hist[tdac_v]->Fill(thr_v);
					lin_all->Fill(thr_v);	
				}

				//Fill Differential Plots
				if (whichFE(j)==2) {
					if (thr_v == 0) zero_Diff++;
					diff_hist[tdac_v+15]->Fill(thr_v);
					diff_all->Fill(thr_v);
				}

				std::cout << n << "	" << thr_values[n] << "	" << tdac_values[n] << std::endl;
				//	std::cout << n << "	" << thr_values.size() << "	" << tdac_values.size() << std::endl;
				n++;
			}
		}

		std::cout << "\n \n \n Number of Zeros are	Linear:" << zero_Lin << "	Differential:" << zero_Diff << std::endl;


		for (int i=0; i<diff_plot; i++) {
			diff_hist[i]->SetLineColor(1);
			diff_hist[i]->SetLineWidth(1);
			if (i<lin_plot) {
				lin_hist[i]->SetLineColor(1);
				lin_hist[i]->SetLineWidth(1);
			}
		}


		//Threshold TDAC plot
		THStack *hs_Lin = new THStack("hs_Lin","");
		gStyle->SetPalette(kBird);
		for (int i=0; i<lin_plot; i++) hs_Lin->Add(lin_hist[i]);
		TCanvas *cs_Lin = new TCanvas("cs_Lin", "cs_Lin", 800, 600);
		cs_Lin->SetLeftMargin(0.15);
		cs_Lin->SetRightMargin(0.10);
		cs_Lin->SetBottomMargin(0.1225);
		//hs_Lin->Draw();
		hs_Lin->Draw("PFC"); 
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
		for(int i=lin_plot-1; i>-1; i--) hs_Lin_legend->AddEntry(lin_hist[i], lin_num[i].c_str(),"f");
		hs_Lin_legend->SetBorderSize(0);
		hs_Lin_legend->SetTextSize(0.04);
		hs_Lin_legend->Draw();
		TLatex *tname= new TLatex();
		tname->SetNDC();
		tname->SetTextAlign(22);
		tname->SetTextFont(73);
		tname->SetTextSizePixels(30);
		tname->DrawLatex(0.21,0.96,"RD53A");
		tname->DrawLatex(0.8, 0.96, chipnum.c_str());
		//hs_Lin->SetMaximum((hs_Lin->GetMaximum())*1.15);
		hs_Lin->GetXaxis()->SetRangeUser((lin_all->GetMean() - 3*(lin_all->GetRMS())), (lin_all->GetMean() + 3*(lin_all->GetRMS())));
		cs_Lin->Update();
		filename1 = filename.replace(filename.find(".dat"), 15, "_LINThrTDAC.pdf"); 
		cs_Lin->Print(filename1.c_str());

		THStack *hs_Diff = new THStack("hs_Diff","");
		gStyle->SetPalette(kBird);
		for (int i=0; i<diff_plot; i++) hs_Diff->Add(diff_hist[i]);
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
		for(int i=diff_plot-1; i>-1; i--) hs_Diff_legend->AddEntry(diff_hist[i], diff_num[i].c_str(),"f");
		hs_Diff_legend->SetBorderSize(0);
		hs_Diff_legend->SetTextSize(0.03);
		hs_Diff_legend->Draw();

		tname->DrawLatex(0.21,0.96,"RD53A");
		tname->DrawLatex(0.8, 0.96, chipnum.c_str());
		//hs_Diff->SetMaximum((hs_Diff->GetMaximum())*1.15);
		hs_Diff->GetXaxis()->SetRangeUser((diff_all->GetMean() - 3*(diff_all->GetRMS())), (diff_all->GetMean() + 3*(diff_all->GetRMS())));
		cs_Diff->Update();
		filename2 = filename.replace(filename.find("_LINThrTDAC.pdf"), 16, "_DIFFThrTDAC.pdf"); 
		cs_Diff->Print(filename2.c_str());

		for (int i=0; i<diff_plot; i++) {
			delete diff_hist[i];
			if (i<lin_plot) delete lin_hist[i];
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
