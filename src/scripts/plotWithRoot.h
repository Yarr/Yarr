//Header file for plotWithRoot

#include <TCanvas.h>
#include <TH2F.h>
#include <TPaveStats.h>
#include <THStack.h>
#include <TLegend.h>
#include <TLatex.h>

#include <RD53Style.C>

//Style for TH1
void style_TH1(TH1* hist_TH1, const char* Xtitle, const char* Ytitle ){
	hist_TH1->GetXaxis()->SetTitle(Xtitle);
	hist_TH1->GetYaxis()->SetTitle(Ytitle);
	hist_TH1->GetXaxis()->SetTitleSize(0.045);
	hist_TH1->GetXaxis()->SetTitleOffset(1.25);
	hist_TH1->GetYaxis()->SetTitleSize(0.05);
	hist_TH1->GetYaxis()->SetTitleOffset(1.5);
	hist_TH1->SetStats(0);
}

//Style for THStack (TH1)
void style_THStack(THStack* hist_Stack, const char* Xtitle, const char*Ytitle){
	hist_Stack->GetXaxis()->SetTitle(Xtitle);
	hist_Stack->GetYaxis()->SetTitle(Ytitle);
	hist_Stack->GetXaxis()->SetTitleSize(0.045);
	hist_Stack->GetXaxis()->SetTitleOffset(1.25);
	hist_Stack->GetYaxis()->SetTitleSize(0.05);
	hist_Stack->GetYaxis()->SetTitleOffset(1.5);
}

//Style for TH1 Canvas
void style_TH1canvas(TCanvas *c){
	c->SetLeftMargin(0.15);
	c->SetRightMargin(0.05);
	c->SetBottomMargin(0.1225);

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
	//const char fe_type[3] = ["syn", "lin", "diff"];
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

int whichSigma(double value, double mean, double sigma) {
	int hist_bin;
	if ( value < (mean - 5*sigma)) hist_bin = 6;
	else if ( value >= (mean - 5*sigma) && value < (mean - 4*sigma)) hist_bin = 5;
	else if ( value	>= (mean - 4*sigma) && value < (mean - 3*sigma)) hist_bin = 4;
	else if ( value >= (mean - 3*sigma) && value < (mean - 2*sigma)) hist_bin = 3;
	else if ( value >= (mean - 2*sigma) && value < (mean - sigma)) hist_bin = 2;
	else if ( value >= (mean - sigma) && value < mean) hist_bin = 1;
	else if ( value == mean) hist_bin = 1;
	else if ( value > mean && value <= (mean + sigma)) hist_bin = 1;
	else if ( value > (mean + sigma) && value <= (mean + 2*sigma)) hist_bin = 2;
	else if ( value > (mean + 2*sigma) && value <= (mean + 3*sigma)) hist_bin = 3;
	else if ( value > (mean + 3*sigma) && value <= (mean + 4*sigma)) hist_bin = 4;
	else if ( value > (mean + 4*sigma) && value <= (mean + 5*sigma)) hist_bin = 5;
	else if ( value > (mean + 5*sigma)) hist_bin = 6;
	return hist_bin;
}


