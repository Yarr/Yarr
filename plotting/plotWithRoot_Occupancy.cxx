#include <iostream>
#include <fstream>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <plotWithRoot.h>
#include <RD53Style.h>

int main(int argc, char *argv[]) { //./plotWithRoot_Occupancy path/to/directory file_ext
	//Example file extensions: png, pdf, C, root	

	SetRD53Style();
	gStyle->SetTickLength(0.02);
	gStyle->SetTextFont();

	if (argc < 3) {
		std::cout << "No directory and/or image plot extension given! \nExample: ./plotWithRoot_Occupancy path/to/directory/ pdf" << std::endl;
		return -1;
	}

	std::string dir, filepath, file_name, chipnum;
	DIR *dp;
	struct dirent *dirp;
	struct stat filestat;

	std::string delimiter = "_";
	std::string ext = argv[2];

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

		if ( strstr( file_path, "OccupancyMap.dat") != NULL) { //if filename contains string declared in argument.
				chipnum = "Chip SN: " + file_name.substr(0, file_name.find(delimiter)); //get chip # from file name

				std::cout << "Opening file: " << filepath.c_str() << std::endl;
				std::string filename = filepath.c_str();
				std::fstream infile(filepath.c_str(), std::ios::in);

				std::string type;
				std::string name;

				std::string line;	

				std::string filename1, filename2, filename3, filename4;

				int underflow, overflow;

				std::getline(infile, type);
				std::getline(infile, name);
				std::getline(infile, line); //skip "Column"
				std::getline(infile, line); //skip "Rows"
				std::getline(infile, line); //skip "Hits"
				std::getline(infile, line); //skip x range
				std::getline(infile, line); //skip y range

				std::string xaxistitle = "Occupancy [%]";
				std::string yaxistitle = "Number of Pixels";
				int rowno = 192;
				int colno = 400;
				int xbins = 6;
				double xlow = 0.5;
				double xhigh = 6.5;	
				double inj_value = 100;

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
				TH1 *h_Syn = NULL;
				h_Syn = (TH1*) new TH1F("h_Syn", "", xbins, xlow, xhigh);

				TH1 *h_Lin = NULL;
				h_Lin = (TH1*) new TH1F("h_Lin","", xbins, xlow, xhigh);

				TH1 *h_Diff = NULL;
				h_Diff = (TH1*) new TH1F("h_Diff","", xbins, xlow, xhigh);

				THStack *hs = new THStack("hs","");

				TH1* fe_hist[3] = {h_Syn, h_Lin, h_Diff};
				const char *LabelName[6] = {"0%", " 0-98%", " 98-100%", "100%", "100-102%", " >102%"};

				//Fill histograms
				for (int i=0; i<rowno; i++) {
					for (int j=0; j<colno; j++) {

						double tmp;
						infile >> tmp;

						int bin_num = whichBin(inj_value, tmp);
						fe_hist[whichFE(j)]->AddBinContent(bin_num);
					}
				}

				//Formatting histograms
				for(int i=0; i<3; i++){
					style_TH1(fe_hist[i], xaxistitle.c_str(), yaxistitle.c_str());
					for (int j=1; j<=xbins; j++) fe_hist[i]->GetXaxis()->SetBinLabel(j,LabelName[j-1]);
					fe_hist[i]->GetXaxis()->LabelsOption("h");
					
					fe_hist[i]->SetMarkerSize(1.8);
					fe_hist[i]->SetMarkerColor(1);
				}

				std::string rd53 = "RD53A Internal";

				//Synchronous FE Plot	
				h_Syn->SetFillColor(kOrange+6);
				h_Syn->SetLineColor(kOrange+6);
				TCanvas *c_Syn = new TCanvas("c_Syn", "c_Syn", 800, 600);
				style_TH1canvas(c_Syn);
				h_Syn->Draw();
				h_Syn->Draw("TEXT0 SAME");
				TLatex *tname= new TLatex();
				latex_Chip(tname);
				tname->DrawLatex(0.28,0.96,rd53.c_str());
				tname->DrawLatex(0.8, 0.96, chipnum.c_str());
				TLegend *syn_legend = new TLegend(0.7,0.82,0.88,0.91);
				syn_legend->SetHeader("Analog FEs", "C");
				syn_legend->AddEntry(h_Syn, "Synchronous", "f");
				syn_legend->SetBorderSize(0);
				syn_legend->Draw();		
				h_Syn->SetMaximum((h_Syn->GetMaximum())*1.21);
				c_Syn->Update();
				std::string syn_ext = "_SYN.";
				filename1 = filename.replace(filename.find(".dat"), 9, syn_ext.append(ext)); 
				c_Syn->Print(filename1.c_str());

				//Linear FE Plot
				h_Lin->SetFillColor(kSpring+4);
				h_Lin->SetLineColor(kSpring+4);
				TCanvas *c_Lin = new TCanvas("c_Lin", "c_Lin", 800, 600);
				style_TH1canvas(c_Lin);
				h_Lin->Draw();
				h_Lin->Draw("TEXT0 SAME");
				tname->DrawLatex(0.28,0.96, rd53.c_str());
				tname->DrawLatex(0.8, 0.96, chipnum.c_str());
				TLegend *lin_legend = new TLegend(0.7,0.82,0.87,0.91);
				lin_legend->SetHeader("Analog FEs", "C");
				lin_legend->AddEntry(h_Lin, "Linear", "f");
				lin_legend->SetBorderSize(0);
				lin_legend->Draw();		
				h_Lin->SetMaximum((h_Lin->GetMaximum())*1.21);
				c_Lin->Update();
				std::string lin_ext = "_LIN.";
				filename2 = filename.replace(filename.find(syn_ext), 9, lin_ext.append(ext)); 
				c_Lin->Print(filename2.c_str());

				//Diff FE Plot
				h_Diff->SetFillColor(kAzure+5);
				h_Diff->SetLineColor(kAzure+5);
				TCanvas *c_Diff = new TCanvas("c_Diff", "c_Diff", 800, 600);
				style_TH1canvas(c_Diff);
				h_Diff->Draw();
				h_Diff->Draw("TEXT0 SAME");
				tname->DrawLatex(0.28,0.96, rd53.c_str());
				tname->DrawLatex(0.8, 0.96, chipnum.c_str());
				TLegend *diff_legend = new TLegend(0.7,0.82,0.87,0.91);
				diff_legend->SetHeader("Analog FEs", "C");
				diff_legend->AddEntry(h_Diff, "Differential", "f");
				diff_legend->SetBorderSize(0);
				diff_legend->Draw();		
				h_Diff->SetMaximum((h_Diff->GetMaximum())*1.21);
				c_Diff->Update();
				std::string diff_ext = "_DIFF.";
				filename3 = filename.replace(filename.find(lin_ext), 10, diff_ext.append(ext)); 
				c_Diff->Print(filename3.c_str());

				//Stack Plot for all 3 FEs
				hs->Add(h_Syn);
				hs->Add(h_Lin);
				hs->Add(h_Diff);
				TCanvas *c_Stack = new TCanvas("c_Stack", "c_Stack", 800, 600);
				style_TH1canvas(c_Stack);
				hs->Draw(); 
				//Setting the title for THStack plots has to be after Draw(), and needs canvas->Modified() after
				style_THStack(hs, xaxistitle.c_str(), yaxistitle.c_str());	
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
				for (int i=1; i<=6; i++) hs->GetXaxis()->SetBinLabel(i,LabelName[i-1]);
				hs->GetXaxis()->LabelsOption("h");	
				tname->DrawLatex(0.28,0.96, rd53.c_str());
				tname->DrawLatex(0.8, 0.96, chipnum.c_str());
				hs->SetMaximum((hs->GetMaximum())*1.2);
				c_Stack->Update();		
				std::string stack_ext = "_STACK.";
				filename4 = filename.replace(filename.find(diff_ext), 11, stack_ext.append(ext));
				c_Stack->Print(filename4.c_str());

				for (int i=0; i<3; i++) delete fe_hist[i];
				delete hs;
				delete c_Syn;
				delete c_Lin;
				delete c_Diff;
				delete c_Stack;
		}
	}

	return 0;
} 
