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

int main(int argc, char *argv[]) { //./plotWithRoot_ToT path/to/directory file_ext goodDiff_On
	//Example file extensions: png, pdf, C, root	

	SetRD53Style();	
	gStyle->SetTickLength(0.02);
	gStyle->SetTextFont();

	if (argc < 4) {
		std::cout << "No directory, image plot extension, and/or good differential FE pixels option given! \nFor only good differential FE pixels, write '1'. \n./plotWithRoot_ToT path/to/directory/ file_ext goodDiff_On \nExample: ./plotWithRoot_ToT path/to/directory/ pdf 1" << std::endl;
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

		if ( strstr( file_path, "TotMap0.dat") != NULL && ( strstr( file_path, "Mean") != NULL || strstr( file_path, "Sigma") != NULL)  ) { //if filename contains string declared in argument.

			chipnum = "Chip SN: " + file_name.substr(0, file_name.find(delimiter)); //get chip # from file name

			std::cout << "Opening file: " << filepath.c_str() << std::endl;
			std::string filename = filepath.c_str();
			std::fstream infile(filepath.c_str(), std::ios::in);

			std::string type;
			std::string name;

			std::string xaxistitle, yaxistitle;
			std::string line;	

			std::string filename1, filename2, filename3;


			std::getline(infile, type);
			std::getline(infile, name);
			std::getline(infile, line); //skip "Column"
			std::getline(infile, line); //skip "Rows"
			std::getline(infile, xaxistitle); //get xaxistitle "Mean ToT [bc]" or "Sigma ToT [bc]"
			std::getline(infile, line); //skip x range
			std::getline(infile, line); // skip y range

			int xbins;
			double xlow, xhigh;
			int underflow, overflow;
			int rowno, colno;

			rowno = 192;
			colno = 400;
			yaxistitle = "Number of Pixels";
			xbins = 14;
			xlow = 0.5;
			xhigh = 14.5;

			if ( strstr( file_path, "Sigma") != NULL ) {
				xlow = -0.05;
				xhigh = 10.05;
				xbins = 101;
			}

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
			std::string fe_hist_names[4] = {"h_all", "h_Syn", "h_Lin", "h_Diff"};
			TH1 *fe_hist[4];
			for (unsigned i=0; i<4; i++) fe_hist[i] = new TH1F(fe_hist_names[i].c_str(),"", xbins, xlow, xhigh);

			TH2F *h_plot = NULL;
			h_plot = new TH2F("h_plot", "", colno, 0, colno, rowno, 0, rowno); 

			//int zeros_FE[3] = { 0, 0, 0};
			//char zeros_Syn[100]={}, zeros_Lin[100]={}, zeros_Diff[100]={};
			//char* zeros_char[3] = {zeros_Syn, zeros_Lin, zeros_Diff};
			//Fill ToT plots	
			for (int i=0; i<rowno; i++) {
				for (int j=0; j<colno; j++) {

					double tmp;
					infile >> tmp;
					if(whichFE(j) != 2 || good_Diff != "1") {
						//if (tmp == 0) zeros_FE[whichFE(j)]++;				
						fe_hist[0]->Fill(tmp);
						fe_hist[whichFE(j)+1]->Fill(tmp);
						h_plot->SetBinContent(j+1,i+1,tmp);	
					}
					else {
						if (good_Diff == "1" && goodDiff(i,j) == 1) {
							//if (tmp == 0) zeros_FE[whichFE(j)]++;				
							fe_hist[0]->Fill(tmp);
							fe_hist[whichFE(j)+1]->Fill(tmp);
							h_plot->SetBinContent(j+1,i+1,tmp);	
						}	
						else if (good_Diff == "1" && goodDiff(i,j) == 0) {
							h_plot->SetBinContent(j+1,i+1,-1);	
						}
					}
				}
			}

			char  mean_Syn[100]={}, mean_Lin[100]={}, mean_Diff[100]={};
			char rms_Syn[100]={}, rms_Lin[100]={}, rms_Diff[100]={};

			char* mean_char[3] = {mean_Syn, mean_Lin, mean_Diff};
			char* rms_char[3] = {rms_Syn, rms_Lin, rms_Diff};

			//For RD53A and Chip ID labels.
			TLatex *tname= new TLatex();
			tname->SetNDC();
			tname->SetTextAlign(22);
			tname->SetTextFont(73);
			tname->SetTextSizePixels(30);
			std::string rd53 = "RD53A Internal";

			//For Mean and RMS labels
			TLatex *mean_rms_z = new TLatex;
			mean_rms_z->SetNDC();
			mean_rms_z->SetTextAlign(13);
			mean_rms_z->SetTextFont(63);
			mean_rms_z->SetTextSizePixels(24);

			TLegend* fe_legend[4];
			std::string legend_name[4] = {"All", "Synchronous", "Linear", "Differential"};					

			int color[4] = {800, 806, 824, 865}; //(kOrange, kOrange+6, kSpring+4, kAzure+5)

			//Create canvases	
			std::string c_name[4]  = {"c_All", "c_Syn", "c_Lin", "c_Diff"};
			TCanvas* fe_c[4];

			//Create filename extensions
			std::string plot_ext[7] = {".dat", "_All.", "_SYN.", "_LIN.", "_DIFF.", "_PLOT.", "_STACK."};
			for (int i=1; i<7; i++) plot_ext[i].append(ext);

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

				//Write the Mean, RMS, and number of untuned pixels
				if (i>0) {
					sprintf(mean_char[i-1], "Mean = %.1f", fe_hist[i]->GetMean());
					mean_rms_z->DrawLatex(0.18,0.91, mean_char[i-1]);
					sprintf(rms_char[i-1], "RMS = %.1f", fe_hist[i]->GetRMS());
					mean_rms_z->DrawLatex(0.18,0.87, rms_char[i-1]);
					//sprintf(zeros_char[i-1], "Untuned Pixels = %i", zeros_FE[i]);
					//mean_rms_z->DrawLatex(0.18,0.83, zeros_char[i-1]);
				}					

				fe_hist[i]->SetMaximum((fe_hist[i]->GetMaximum())*1.21); //Leave extra room for legend.
				if ( strstr( file_path, "Sigma") != NULL ) fe_hist[i]->GetXaxis()->SetRangeUser(((fe_hist[i]->GetMean()) - 2*(fe_hist[i]->GetRMS()) < 0) ? 0 : ((fe_hist[i]->GetMean())- 2*(fe_hist[i]->GetRMS())), (fe_hist[i]->GetMean()) + 2*(fe_hist[i]->GetRMS())); //Change the x-axis range to be the Mean +/- 2*RMS. If the lower bound is less than 0, make it 0.
				fe_c[i]->Update();

				filename1 = filename.replace(filename.find(plot_ext[i].c_str()), 10, plot_ext[i+1].c_str()); 
				fe_c[i]->Print(filename1.c_str());
			}

			//Map of Pixels
			style_TH2(h_plot, "Column", "Row", xaxistitle.c_str()); 
			TCanvas *c_plot = new TCanvas("c_plot", "c_plot", 800, 600);
			style_TH2canvas(c_plot);
			//gStyle->SetPalette(kTemperatureMap);
			h_plot->Draw("colz");
			//h_plot->Draw("colz X+ Y+ SAME");
			h_plot->GetXaxis()->SetLabelSize(0.04);
			h_plot->GetYaxis()->SetLabelSize(0.04);
			tname->DrawLatex(0.23,0.96, rd53.c_str());
			tname->DrawLatex(0.72, 0.96, chipnum.c_str());
			gStyle->SetOptStat(0);
			c_plot->RedrawAxis();
			c_plot->Update();
			if ( strstr(file_path, "Mean") != NULL ) h_plot->GetZaxis()->SetRangeUser(0,15);
			if ( strstr( file_path, "Sigma") != NULL ) h_plot->GetZaxis()->SetRangeUser(((fe_hist[0]->GetMean()) - 2*(fe_hist[0]->GetRMS()) < 0) ? 0 : ((fe_hist[0]->GetMean())- 2*(fe_hist[0]->GetRMS())), (fe_hist[0]->GetMean()) + 2*(fe_hist[0]->GetRMS()));
			h_plot->GetZaxis()->SetLabelSize(0.04);
			filename2 = filename.replace(filename.find(plot_ext[4].c_str()), 10, plot_ext[5].c_str());
			c_plot->Print(filename2.c_str());

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
			TLegend *stack_legend = new TLegend(0.33,0.82,0.93,0.91);
			stack_legend->SetNColumns(3);
			stack_legend->SetHeader("Analog FEs", "C");
			for (int i=1; i<4; i++) stack_legend->AddEntry(fe_hist[i], legend_name[i].c_str(), "f"); 
			stack_legend->SetBorderSize(0);
			stack_legend->Draw();
			tname->DrawLatex(0.28,0.96, rd53.c_str());
			tname->DrawLatex(0.8, 0.96, chipnum.c_str());

			hs->SetMaximum((fe_hist[0]->GetMaximum())*1.21);
			if ( strstr( file_path, "Sigma") != NULL ) hs->GetXaxis()->SetRangeUser(((fe_hist[0]->GetMean()) - 2*(fe_hist[0]->GetRMS()) < 0) ? 0 : ((fe_hist[0]->GetMean())- 2*(fe_hist[0]->GetRMS())), (fe_hist[0]->GetMean()) + 2*(fe_hist[0]->GetRMS()));
			c_Stack->Update();				
			filename3 = filename.replace(filename.find(plot_ext[5].c_str()), 11, plot_ext[6].c_str());
			c_Stack->Print(filename3.c_str());


			for (int i=0; i<4; i++) {
				delete fe_hist[i];
				delete fe_c[i];
			}

			delete h_plot;
			delete c_plot;
			delete hs;
			delete c_Stack;

		}
	}

	return 0;
} 
