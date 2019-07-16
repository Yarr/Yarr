// Example for plot style

#include <iostream>

#include "RD53Style.h"

#include "TROOT.h"

void SetRD53Style()
{
  static TStyle* rd53Style = 0;
  std::cout << "\nApplying RD53 plotting style settings...\n" << std::endl ;
  if ( rd53Style==0 ) rd53Style = RD53Style();
  gROOT->SetStyle("RD53");
  gROOT->ForceStyle();
}

TStyle* RD53Style() 
{
  TStyle *rd53Style = new TStyle("RD53","RD53 style");

  // use plain black on white colors
  Int_t icol=0; // WHITE
  rd53Style->SetFrameBorderMode(icol);
  rd53Style->SetFrameFillColor(icol);
  rd53Style->SetCanvasBorderMode(icol);
  rd53Style->SetCanvasColor(icol);
  rd53Style->SetPadBorderMode(icol);
  rd53Style->SetPadColor(icol);
  rd53Style->SetStatColor(icol);
  //rd53Style->SetFillColor(icol); // don't use: white fill color for *all* objects

  //Legend
  Int_t ileg=0;
  rd53Style->SetLegendBorderSize(ileg);
  rd53Style->SetLegendFillColor(ileg);
  rd53Style->SetLegendTextSize(0.045);
  rd53Style->SetLegendFont(42);

  // set the paper & margin sizes
  rd53Style->SetPaperSize(20,26);

  // set margin sizes
  rd53Style->SetPadTopMargin(0.07);
  rd53Style->SetPadRightMargin(0.05);
  rd53Style->SetPadBottomMargin(0.16);
  rd53Style->SetPadLeftMargin(0.16);

  // set title offsets (for axis label)
  rd53Style->SetTitleXOffset(1.5);
  rd53Style->SetTitleYOffset(1.5);

  // set label offset
  rd53Style->SetLabelOffset(0.01,"xyz");

  // use large fonts
  Int_t font=42; //  helvetica-medium-r-normal "Arial"
  Double_t tsize=0.055;
  rd53Style->SetTextFont(font);

  rd53Style->SetTextSize(tsize);
  rd53Style->SetLabelFont(font,"x");
  rd53Style->SetTitleFont(font,"x");
  rd53Style->SetLabelFont(font,"y");
  rd53Style->SetTitleFont(font,"y");
  rd53Style->SetLabelFont(font,"z");
  rd53Style->SetTitleFont(font,"z");
  
  rd53Style->SetLabelSize(tsize,"x");
  rd53Style->SetTitleSize(tsize,"x");
  rd53Style->SetLabelSize(tsize,"y");
  rd53Style->SetTitleSize(tsize,"y");
  rd53Style->SetLabelSize(tsize,"z");
  rd53Style->SetTitleSize(tsize,"z");

  // use bold lines and markers
  rd53Style->SetMarkerStyle(20);
  rd53Style->SetMarkerSize(2);
  rd53Style->SetHistLineWidth(2.);
  rd53Style->SetLineStyleString(2,"[12 12]"); // postscript dashes
  rd53Style->SetMarkerColor(kAzure+2);
  rd53Style->SetLineColor(kAzure+2);

  // get rid of X error bars (as recommended in ATLAS figure guidelines)
  rd53Style->SetErrorX(0.0001);
  // get rid of error bar caps
  rd53Style->SetEndErrorSize(0.);

  // do not display any of the standard histogram decorations
  rd53Style->SetOptTitle(0);
  //rd53Style->SetOptStat(1111);
  rd53Style->SetOptStat(0);
  //rd53Style->SetOptFit(1111);
  rd53Style->SetOptFit(0);

  // put tick marks on top and RHS of plots
  rd53Style->SetPadTickX(1);
  rd53Style->SetPadTickY(1);

  return rd53Style;

}

