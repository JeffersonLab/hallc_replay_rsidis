/*
  Script to calculate good DIS events.
  ----
  [terminal]$ root -l
  root [0] .x get_good_dis_ev.C(<run_number>,<nevents_replayed>,<spectrometer>)
  ----
  P. Datta <pdbforce@jlab.org> Created 17 July 2025
*/

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <regex>

#include "TChain.h"
#include "TCanvas.h"
#include "TStopwatch.h"

/* ------ ########### ------
   ###### USER INPUTS ######
   ------ ########### ------ */
// --- Basic ---
// ---
// 1. list of analysis cuts for HMS DIS
std::string anacutsHMS = "abs(H.gtr.dp)<8&&H.cer.npeSum>1";
// 2. list of analysis cuts for SHMS DIS
std::string anacutsSHMS = "abs(P.gtr.dp-5.)<15.&&P.aero.npeSum>4";
// 3. histo ranges - Convention: {nbin,hmin,hmax}
std::vector<double> heOVp_range{200,0.3,2.0};
std::vector<double> hQ2_range{200,0.1,10},hx_range{200,0.01,1.2},hW_range{200,0.1,5};
// 4. E/p cut lower limit
double cutrangeHMS = 0.7;
double cutrangeSHMS = 0.8;
// --- **** ---
// --- **** ---

void CustomizeHist(TH1F *h);
void PlotCutRegion(double xmin, double xmax, EColor fcolor, double alpha);
double CalcNormYield(std::string const &inrepfile, double Nrealcoinev, std::string spec, int verbosity);
std::vector<std::string> SplitString(char const delim, std::string const myStr);
double ExtractValueFromReportFile(const std::string& filename, const std::string& key, const char delimiter, int skipCount);
TPaveText* CreateSummaryPaveText(int rnum, ULong64_t totevintree, const std::string& anacuts, const double counts, double normyield, TStopwatch* sw);
void PrintCSVLine(std::ofstream &out, int runnum, double gooddis, double normyield);

// global variables
bool is_50k = false;

// Main function
int get_good_dis_ev(int rnum,                  // Run number to analyze
		     int nevent,               // # of events replayed
		     std::string spec,         // Specify spectrometer: HMS or, SHMS
		     std::string indirroot="ROOTfiles", // Path to directory containing input ROOT file
		     std::string outfilebase="output_get_good_dis_ev") // output filename prefix
{
  gErrorIgnoreLevel = kError; // Ignores all ROOT warnings

  if (nevent==50000) is_50k = true;  

  // Define a clock to keep track of macro processing time
  TStopwatch *sw = new TStopwatch();
  sw->Start();

  // Defining spectrometer specific variables
  std::string anacuts = spec.compare("HMS")==0 ? anacutsHMS : anacutsSHMS;
  std::string specvar = spec.compare("HMS")==0 ? "H" : "P";
  std::string speclower = spec.compare("HMS")==0 ? "hms" : "shms";
  
  // Reading input ROOT files
  std::string inrfile = Form("%s/%s_coin_replay_production_%d_%d.root",indirroot.c_str(),speclower.c_str(),rnum,nevent); // input ROOT file name with directory path
  ROOT::EnableImplicitMT();
  ROOT::RDataFrame data_rdf_raw("T",inrfile.c_str());

  // defining output directories and ROOT file 
  std::string indirreport=Form("REPORT_OUTPUT/%s/PRODUCTION",spec.c_str()); // Path to directory containing input report file
  std::string outdirplot=Form("HISTOGRAMS/%s/PDF",spec.c_str());            // Path to directory to save output plots
  //
  TString outfile = inrfile; // Let's save the output files in the input root file
  TFile *fout = new TFile(outfile.Data(),"UPDATE");
  
  // Defining histos
  // eOVp 
  TH1F *heOVp = (TH1F*)data_rdf_raw.Filter(anacuts)
    .Histo1D({"heOVp","",int(heOVp_range[0]),heOVp_range[1],heOVp_range[2]},Form("%s.cal.etottracknorm",specvar.c_str()))->Clone();
  heOVp->GetXaxis()->SetTitle("E/p"); CustomizeHist(heOVp); 
  // kine
  TH1F *hx = (TH1F*)data_rdf_raw.Filter(Form("%s&&abs(%s.kin.x_bj)<3",anacuts.c_str(),specvar.c_str()))
    .Histo1D({"hx","",int(hx_range[0]),hx_range[1],hx_range[2]},Form("%s.kin.x_bj",specvar.c_str()))->Clone();
  hx->GetXaxis()->SetTitle("x_{bj}"); CustomizeHist(hx);
  TH1F *hQ2 = (TH1F*)data_rdf_raw.Filter(Form("%s&&abs(%s.kin.Q2)<10",anacuts.c_str(),specvar.c_str()))
    .Histo1D({"hQ2","",int(hQ2_range[0]),hQ2_range[1],hQ2_range[2]},Form("%s.kin.Q2",specvar.c_str()))->Clone();
  hQ2->GetXaxis()->SetTitle("Q^{2} (GeV/c)^{2}"); CustomizeHist(hQ2); 
  TH1F *hW = (TH1F*)data_rdf_raw.Filter(Form("%s&&abs(%s.kin.W)<10",anacuts.c_str(),specvar.c_str()))
    .Histo1D({"hW","",int(hW_range[0]),hW_range[1],hW_range[2]},Form("%s.kin.W",specvar.c_str()))->Clone();
  hW->GetXaxis()->SetTitle("W (GeV)"); CustomizeHist(hW);

  // Plotting the eOVp time histo and calculating count
  TCanvas *ceOVp = new TCanvas("ceOVp","ceOVp",1500,600);
  ceOVp->Divide(2,1);  
  ceOVp->cd(1);
  heOVp->Draw();
  heOVp->Write("",TObject::kOverwrite);  
  double cutrange = spec.compare("HMS")==0 ? cutrangeHMS : cutrangeSHMS;
  double counts = heOVp->Integral(heOVp->FindBin(cutrange),heOVp->FindBin(heOVp_range[2]));  
  PlotCutRegion(cutrange,heOVp_range[2],kGreen,0.3); // main coin peak

  //std::cout << counts << "\n";

  // Calculate charge normalized and efficiency ccorrected yield
  std::string inrepfile = Form("%s/replay_%s_coin_production_%d_%d.report",indirreport.c_str(),speclower.c_str(),rnum,nevent); // input report file name with directory path  
  double normyield = CalcNormYield(inrepfile,counts,spec,1);
  // Write important stuff to a summary canvas
  ceOVp->cd(2);
  ULong64_t nEntries = *data_rdf_raw.Count();
  //std::cout << nEntries << "\n";  
  TPaveText* pvtxt = CreateSummaryPaveText(rnum, nEntries, anacuts, counts, normyield, sw);
  pvtxt->Draw();
  ceOVp->Update();
  ceOVp->Write("",TObject::kOverwrite);

  // Ploting several physics histograms
  TCanvas *cphys = new TCanvas("cphys","cphys",1500,800);
  cphys->Divide(2,2);  
  gStyle->SetOptStat(1111);
  //
  cphys->cd(1);
  hx->Draw();
  hx->Write("",TObject::kOverwrite);
  //
  cphys->cd(2);
  hQ2->Draw();
  hQ2->Write("",TObject::kOverwrite);
  //
  cphys->cd(3);
  hW->Draw();
  hW->Write("",TObject::kOverwrite);
  
  // Writing out the canvas
  TString outplot = Form("%s/%s_%d_%d.pdf",outdirplot.c_str(),outfilebase.c_str(),rnum,nevent);
  ceOVp->SaveAs(Form("%s[",outplot.Data()));
  ceOVp->SaveAs(Form("%s",outplot.Data())); 
  cphys->SaveAs(Form("%s",outplot.Data()));
  cphys->SaveAs(Form("%s]",outplot.Data()));

  // Writing out some useful stuff
  std::string outcsv = Form("%s/%s_%d_%d.csv",indirreport.c_str(),outfilebase.c_str(),rnum,nevent);
  std::ofstream outcsv_data(outcsv.c_str());
  PrintCSVLine(outcsv_data,rnum,counts,normyield);

  std::cout << "------" << std::endl;
  std::cout << " Output CSV file  : " << outcsv << std::endl;
  std::cout << " Output PDF file  : " << outplot << std::endl;
  std::cout << " Output ROOT file  : " << outfile << std::endl;  
  std::cout << "------" << std::endl << std::endl;
  
  // Reporting macro processing time and resources
  std::cout << "\nCPU time = " << sw->CpuTime() << "s. Real time = " << sw->RealTime() << "s.\n";

  return 0;  
}

//----------------------------------------------------------
void CustomizeHist(TH1F *h) {
  // Customizes coin histo
  
  h->SetLineColor(kBlack);
  //h->SetLineWidth(2);
  h->GetXaxis()->CenterTitle();
}
//----------------------------------------------------------
double UnfoldyNDC(double yNDC) 
/* Returns y value for a given y coordinate in NDC */
{
  gPad->Update();
  return yNDC*(gPad->GetY2()-gPad->GetY1()) + gPad->GetY1();
}
//----------------------------------------------------------
void PlotCutRegion(double xmin, double xmax, EColor fillcolor, double alpha)
/* Plots cut region for given x range */
{
  TBox *cutRegion = new TBox(xmin,UnfoldyNDC(0.1),xmax,UnfoldyNDC(0.9));
  cutRegion->SetFillColorAlpha(fillcolor, 0.3);
  cutRegion->Draw();
}
//----------------------------------------------------------
std::string trim(const std::string& str) {
  // Function to trim leading and trailing spaces
  size_t first = str.find_first_not_of(" \t");
  if (first == std::string::npos) return ""; // Empty or only spaces
  size_t last = str.find_last_not_of(" \t");
  return str.substr(first, (last - first + 1));
}
//----------------------------------------------------------
std::string normalizeSpaces(const std::string& str) {
  // Function to normalize spaces (convert multiple spaces to a single space)
  return std::regex_replace(str, std::regex("\\s+"), " ");
}
//----------------------------------------------------------
double ExtractValueFromReportFile(const std::string& filename, const std::string& key, const char delimiter, int skipCount = 0)
/*
  Reads the hcana report file and extracts the number (int or double) associated with
  a given string and delimiter (: or =), skipping `skipCount` number of valid matches
  before returning the number.
*/
{
  std::ifstream file(filename);
  if (!file.is_open()) {
    std::cerr << "Error: Unable to open file " << filename << std::endl;
    return -1;  // Indicate failure
  }

  std::string line;
  std::string normalizedKey = normalizeSpaces(trim(key));  // Normalize the key once
  int matchCount = 0;

  while (std::getline(file, line)) {
    std::string normalizedLine = normalizeSpaces(trim(line));  // Normalize spaces and trim

    size_t pos = normalizedLine.find(normalizedKey);
    
    // Ensure the match is exact (not a substring of another key)
    if (pos != std::string::npos) {
      bool validMatch = false;

      if (pos == 0 || std::isspace(normalizedLine[pos - 1])) {
        size_t searchPos = pos + normalizedKey.length();
        while (searchPos < normalizedLine.length() && std::isspace(normalizedLine[searchPos])) {
          searchPos++;
        }
        if (searchPos < normalizedLine.length() && normalizedLine[searchPos] == delimiter) {
          validMatch = true;
        }
      }

      if (validMatch) {
        if (matchCount < skipCount) {
          matchCount++;
          continue;  // Skip this occurrence
        }

        std::istringstream iss(normalizedLine.substr(normalizedLine.find(delimiter) + 1));
        double number;
        if (iss >> number) {
          return number;
        }
      }
    }
  }

  std::cerr << "Error: Key '" << key << "' not found after skipping " << skipCount << " occurrences." << std::endl;
  return -1;  // Indicate failure
}
//----------------------------------------------------------
double CalcNormYield(std::string const &inrepfile, // Input report file name with path
		     double discount,          // No. of good DIS count
		     std::string spec,         // HMS or SHMS
		     int verbosity)
/* Calculates charge normalized and efficiency corrected yeild from random subtracted coin events */
{
  //double charge = ExtractValueFromReportFile(inrepfile, "HMS BCM4A Beam Cut Charge", ':'); //mC
  double charge;
  double compdeadtime;
  double treffi;
  double trigeffi;

  if (spec.compare("HMS")==0) {
    charge = ExtractValueFromReportFile(inrepfile, "BCM4C Beam Cut Charge", ':', 0); //uC  
    compdeadtime = ExtractValueFromReportFile(inrepfile, "HMS Computer Dead Time", ':', 0)/100.0;
    treffi = ExtractValueFromReportFile(inrepfile, "E SING FID TRACK EFFIC", ':', 0);  
    trigeffi = 1.0; // assuming 100% efficiency for the moment
  } else {
    charge = ExtractValueFromReportFile(inrepfile, "BCM4C Beam Cut Charge", ':', 0); //uC  
    compdeadtime = ExtractValueFromReportFile(inrepfile, "SHMS Computer Dead Time", ':', 0)/100.0;
    treffi = ExtractValueFromReportFile(inrepfile, "E SING FID TRACK EFFIC", ':', 0);
    trigeffi = 1.0; // assuming 100% efficiency for the moment
  }

  double normyield = discount / (charge * compdeadtime * treffi * trigeffi); // 1/mC
  
  if (verbosity>0) {
    std::cout << "\n--- Normalized Yield ---\n";
    std::cout << "Spectrometer            : " << spec << "\n";    
    std::cout << "Good DIS Ev             : " << (int)discount << "\n";    
    std::cout << "Charge (uC)             : " << charge << "\n";
    std::cout << "Computer dead time      : " << compdeadtime << "\n";
    std::cout << "Tracking Effi.   S      : " << treffi << "\n";
    std::cout << "Trigger Effi.           : " << trigeffi << "\n";
    std::cout << "Normalized Yield (1/uC) : " << (int)normyield << "\n";            
    std::cout << "-------\n";
  }

  return normyield;
}
//----------------------------------------------------------
std::vector<std::string> SplitString(char const delim, std::string const myStr)
/* Splits a string by a delimiter (doesn't include empty sub-strings) */
{
  std::stringstream ss(myStr);
  std::vector<std::string> out;
  while (ss.good()) {
    std::string substr;
    std::getline(ss, substr, delim);
    if (!substr.empty()) out.push_back(substr);
  }
  if (out.empty()) std::cerr << "WARNING! No substrings found!\n";
  return out;
}
//----------------------------------------------------------
TPaveText* CreateSummaryPaveText(int rnum,
				 ULong64_t totevintree,				 
				 const std::string& anacuts,
				 const double counts,
				 double normyield,
				 TStopwatch* sw)
/* Function to create a summary canvas */
{  
  TPaveText* pvtxt = new TPaveText(.05, .1, .95, .8);
  pvtxt->SetTextFont(42);
  pvtxt->SetTextSize(0.03);

  pvtxt->AddText(Form("Run Number : %d", rnum));
  pvtxt->AddText("Analysis cuts:");
  std::vector<std::string> aCutList = SplitString('&', anacuts.c_str());
  std::string tmpstr = "";
  int ccount = 0;
  for (std::size_t i = 0; i < aCutList.size(); ++i) {
    if (i > 0 && i % 3 == 0) {
      pvtxt->AddText(Form(" %s", tmpstr.c_str()));
      tmpstr = "";
    }
    ccount++;
    tmpstr += aCutList[i];
    if (ccount != aCutList.size()) tmpstr += ", ";

    if ((aCutList.size() - i) < 3 && ccount == aCutList.size()) {
      pvtxt->AddText(Form(" %s", tmpstr.c_str()));
      tmpstr = "";
    }
  }
  pvtxt->AddText("Counts:");
  pvtxt->AddText(Form("Number of Good DIS Events : %.0f", counts));
  //pvtxt->AddText(Form("Charge Normalized and Efficiency Corrected Yield : %.1f (1/mC)", normyield));
  sw->Stop();
  if (is_50k) {
    pvtxt->AddText("Predictions");
    pvtxt->AddText(Form("Rate of Good DIS Events : %.4f per CODA event", (double)counts/50000.));
    pvtxt->AddText("To get \"Extrapolated # good events (this run)\" for the run sheet:");
    pvtxt->AddText(Form("Multiply the rate of good DIS events w/ total CODA events, once the run is complete."));
  }
  else {
    pvtxt->AddText(Form("Macro processing time: CPU %.1fs | Real %.1fs", sw->CpuTime(), sw->RealTime()));
  }
    
  // Highlight key lines
  if (TText* t1 = pvtxt->GetLineWith("Run")) t1->SetTextColor(kGreen + 3);
  if (TText* t2 = pvtxt->GetLineWith("Analysis")) t2->SetTextColor(kBlue);
  if (TText* t3 = pvtxt->GetLineWith("Counts")) t3->SetTextColor(kBlue);
  if (TText* t4 = pvtxt->GetLineWith("Predictions")) t4->SetTextColor(kBlue);
  if (TText* t5 = pvtxt->GetLineWith("Rate of Good")) t5->SetTextColor(kRed);
  if (TText* t6 = pvtxt->GetLineWith("To get")) {
    t6->SetTextColor(kGreen + 3);
    t6->SetTextFont(62);
  }
  if (TText* t7 = pvtxt->GetLineWith("Multiply")) t7->SetTextColor(kRed);
  if (TText* t8 = pvtxt->GetLineWith("Macro")) t8->SetTextColor(kGreen+3);  
  return pvtxt;
}
//----------------------------------------------------------
void PrintCSVLine(std::ofstream &out, int runnum, double gooddis, double normyield) {

  std::ostringstream oss;
  oss << "runnum,gooddis,normyield\n";
  oss << runnum << ","
      << gooddis << ","
      << normyield;

  out << oss.str() << std::endl;
}
