//Header file for plotWithRoot

#include <math.h> //for fabs()
#include <utility> //for pairs
#include <vector>
#include <algorithm> //for std::sort()

#include <TCanvas.h>
#include <TH2.h>
#include <TPaveStats.h>
#include <THStack.h>
#include <TLegend.h>
#include <TLatex.h>
#include <TGraph.h>

#include <RD53Style.C>

//Style for TGraph
void style_TGraph(TGraph* hist_TGraph, const char* Xtitle, const char* Ytitle ){
	hist_TGraph->GetXaxis()->SetTitle(Xtitle);
	hist_TGraph->GetYaxis()->SetTitle(Ytitle);
	hist_TGraph->GetXaxis()->SetTitleSize(0.045);
	hist_TGraph->GetXaxis()->SetTitleOffset(1.25);
	hist_TGraph->GetYaxis()->SetTitleSize(0.05);
	hist_TGraph->GetYaxis()->SetTitleOffset(1.5);
	hist_TGraph->GetXaxis()->SetLabelSize(0.05);
	hist_TGraph->GetYaxis()->SetLabelSize(0.035);

}

//Style for TH1
void style_TH1(TH1* hist_TH1, const char* Xtitle, const char* Ytitle ){
	hist_TH1->GetXaxis()->SetTitle(Xtitle);
	hist_TH1->GetYaxis()->SetTitle(Ytitle);
	hist_TH1->GetXaxis()->SetTitleSize(0.045);
	hist_TH1->GetXaxis()->SetTitleOffset(1.25);
	hist_TH1->GetYaxis()->SetTitleSize(0.05);
	hist_TH1->GetYaxis()->SetTitleOffset(1.5);
	hist_TH1->SetStats(0);
	hist_TH1->GetXaxis()->SetLabelSize(0.05);
	hist_TH1->GetYaxis()->SetLabelSize(0.035);

}

//Style for THStack (TH1)
void style_THStack(THStack* hist_Stack, const char* Xtitle, const char*Ytitle){
	hist_Stack->GetXaxis()->SetTitle(Xtitle);
	hist_Stack->GetYaxis()->SetTitle(Ytitle);
	hist_Stack->GetXaxis()->SetTitleSize(0.045);
	hist_Stack->GetXaxis()->SetTitleOffset(1.25);
	hist_Stack->GetYaxis()->SetTitleSize(0.05);
	hist_Stack->GetYaxis()->SetTitleOffset(1.5);
	hist_Stack->GetXaxis()->SetLabelSize(0.05);
	hist_Stack->GetYaxis()->SetLabelSize(0.035);
}

//Style for TH2
void style_TH2(TH2* hist_TH2, const char* Xtitle, const char*Ytitle, const char* Ztitle){
	hist_TH2->GetXaxis()->SetTitle(Xtitle);
	hist_TH2->GetYaxis()->SetTitle(Ytitle);
	hist_TH2->GetZaxis()->SetTitle(Ztitle);
	hist_TH2->GetXaxis()->SetTitleSize(0.045);
	hist_TH2->GetXaxis()->SetTitleOffset(1.1);
	hist_TH2->GetYaxis()->SetTitleSize(0.05);
	hist_TH2->GetYaxis()->SetTitleOffset(1);
	hist_TH2->GetZaxis()->SetTitleSize(0.05);
	hist_TH2->GetZaxis()->SetTitleOffset(1.2);

}

//Style for TH1 Canvas
void style_TH1canvas(TCanvas *c){
	c->SetLeftMargin(0.15);
	c->SetRightMargin(0.05);
	c->SetBottomMargin(0.1225);

}

//Style for TH2 Canvas
void style_TH2canvas(TCanvas *c){
	c->SetLeftMargin(0.1);
	c->SetRightMargin(0.18);
	c->SetBottomMargin(0.1);

}

//Style for TLatex --> RD53A and Chip Id labels.
void latex_Chip(TLatex* name){
	name->SetNDC();
	name->SetTextAlign(22);
	name->SetTextFont(73);
	name->SetTextSizePixels(30);

}

//Function for if pixel was in certain FE
int whichFE(int col) {
	int division1 = 128, division2 = 264;
	int which_fe = 10; //add if 10, give error in .cxx file
	if (col<division1){ //Syn FE
		which_fe = 0;
	} else if (col>=division1 && col<division2){ //Lin FE
		which_fe = 1;
	} else if (col>=division2) { //Diff FE
		which_fe = 2;
	}
	return which_fe;
}

//Write a function where you give it occupancy and it returns a bin number
int whichBin(double value, double occnum) {
	int hist_bin;
	double zero = 0.0;
	double ninety8 = value * 0.98;
	double hundred02 = value * 1.02;
	if (occnum == zero) hist_bin = 1;
	else if (occnum > zero && occnum < ninety8) hist_bin = 2;
	else if (occnum >= ninety8 && occnum < value) hist_bin = 3;
	else if (occnum == value) hist_bin = 4;
	else if (occnum > value && occnum <= hundred02) hist_bin = 5;
	else if (occnum > hundred02) hist_bin = 6;
	return hist_bin;
}

int whichSigma(double value, double mean, double sigma, double bin_width, int bin_num) {
	int hist_bin = -1;
	for (int i=1; i<bin_num+1; i++) {
		if ( fabs(mean-value) >= ((i-1)*bin_width*sigma) && fabs(mean-value) < (i*bin_width*sigma) ) hist_bin=i;
	}

	if ( fabs(mean-value) >= (bin_num*bin_width*sigma) ) hist_bin=bin_num;
	else if ( fabs(mean-value) < (bin_width*sigma) ) hist_bin=1;

	return hist_bin;
}

int goodDiff(int row, int col) {
	int good = 10;
	int start_col = 264;

	std::array<std::array<unsigned, 8>, 8> mask;
	mask[0] = {{0, 0, 1, 1, 0, 0, 0, 0}};
	mask[1] = {{0, 0, 0, 1, 0, 0, 0, 0}};
	mask[2] = {{0, 0, 0, 1, 0, 0, 0, 0}};
	mask[3] = {{0, 0, 0, 1, 1, 0, 0, 0}};
	mask[4] = {{0, 0, 0, 1, 1, 0, 0, 0}};
	mask[5] = {{0, 0, 0, 1, 1, 0, 0, 0}};
	mask[6] = {{0, 0, 0, 1, 0, 0, 0, 0}};
	mask[7] = {{0, 0, 0, 0, 1, 0, 0, 0}};

	if (col < start_col) {
		//std::cout << "This pixel is not part of the differential FE." << std::endl;	
	}	
	else {
		if (mask[row%8][col%8] == 1) good = 1;
		else good = 0;
	}

	return good;
}

bool sortbysec(const std::pair<double, float> &a, const std::pair<double, float> &b) {	//sort using second element of pairs in vector.
		return (a.second < b.second);
}

std::vector <float> thresholdPercent(std::vector <double> pix_values, int whichFrontEnd, int goodDiffPix,  double gaussMean, double percentValue) {
	//whichFrontEnd --> Synchronous=0, Linear=1, Differential=2. goodDiffPix --> all pixels=0, only good Differential pixels=1
	int rowno=192, colno=400, numPix;
	float numPixels;
	int FEcol[4] = {0, 128, 264, 400};
	if ( whichFrontEnd == 2 && goodDiffPix == 1) numPixels = (FEcol[whichFrontEnd+1] - FEcol[whichFrontEnd]) * rowno * 0.1875 ; //Only account for good Differential pixels (0.1875 = 12/64 pixels in core)
	else numPixels = (FEcol[whichFrontEnd+1] - FEcol[whichFrontEnd]) * rowno; 

	//Create vector of pairs <value, value-gaussMean>. Exclude values less than 0 or greater than 5*Gaussian Mean
	std::vector < std::pair<float,float> > mean_Diff;
	int inDex = 0, zeroCounter=0, overCounter=0;
	for (int i=0; i<rowno; i++ ) {
		for (int j=0; j<colno; j++ ) { 
			if (whichFrontEnd == 2 && goodDiffPix == 1) {
				if ( goodDiff(i, j) == 1 && pix_values[inDex] > 0 && pix_values[inDex] <= 5*gaussMean)  mean_Diff.push_back( std::make_pair(pix_values[inDex], (fabs(pix_values[inDex]-gaussMean))) );
				else if ( goodDiff(i,j) == 1 && pix_values[inDex] <= 0 ) zeroCounter++; 
				else if ( goodDiff(i,j) == 1 && pix_values[inDex] > 5*gaussMean ) zeroCounter++; 
			}
			else {
				if ( j>=FEcol[whichFrontEnd] && j<FEcol[whichFrontEnd+1] && pix_values[inDex] > 0 && pix_values[inDex] <= 5*gaussMean) mean_Diff.push_back( std::make_pair(pix_values[inDex], (fabs(pix_values[inDex]-gaussMean))) );
				else if ( j>=FEcol[whichFrontEnd] && j<FEcol[whichFrontEnd+1] &&  pix_values[inDex] <= 0 ) zeroCounter++;	
				else if ( j>=FEcol[whichFrontEnd] && j<FEcol[whichFrontEnd+1] && pix_values[inDex] > 5*gaussMean ) overCounter++;	
			}
			inDex++;
		}
	}

	std::cout << "\033[1;31mWarning!\033[0m Excluding " << zeroCounter << " pixels with threshold<0 and " << overCounter << " pixels with threshold>5*(GaussianMean)." << std::endl;
	
	numPix = (numPixels - (zeroCounter+overCounter)) * percentValue * 0.01;

	std::sort (mean_Diff.begin(), mean_Diff.end(), sortbysec); //sort from least to greatest for the second element of the pair.

	float vectorSize = mean_Diff.size();

	//Find the value corresponding to X% num of pix. Look at the values around X% of pixels and choose the min/max from there. Save how many iterations it took. 
	float diffMax=99999999.0, diffMin=999999999.0, tryMax, tryMin;
	for (int i=0; i<2000; i++) {
		if (mean_Diff[numPix-i].first > gaussMean) {
			diffMax = mean_Diff[numPix-i].first;
			tryMax=i*-1;
			break;
		}
		else if (mean_Diff[numPix+i].first > gaussMean && numPix+i < vectorSize) {
			diffMax = mean_Diff[numPix+i].first;
			tryMax=i;
			break;
		}
	}	
	if (numPix+tryMax >= vectorSize) std::cout << "\033[1;31mWarning!\033[0m Have reached the maximum in the + direction for the maximum value." << std::endl;
	
	for (int i=0; i<2000; i++) {
		if (mean_Diff[numPix-i].first < gaussMean) {
			diffMin = mean_Diff[numPix-i].first;
			tryMin=i*-1;
			break;
		}
		else if (mean_Diff[numPix+i].first < gaussMean && numPix+i < vectorSize) {
			diffMin = mean_Diff[numPix+i].first;
			tryMin=i;
			break;
		}
	}	
	if (numPix+tryMax >= vectorSize) std::cout << "\033[1;31mWarning!\033[0m Have reached the maximum in the + direction for the minimum." << std::endl;
	
	float checkSize = numPixels - (zeroCounter+overCounter);
	if ( vectorSize != checkSize ) {
		diffMax = -100000;
		diffMin = -100000;
	}

	float maxMin[5] = {vectorSize, diffMin, tryMin, diffMax, tryMax};
	std::vector <float> results;
	for (int i=0; i<5; i++) {
		results.push_back(maxMin[i]);
	}	
	
	return results;
}
