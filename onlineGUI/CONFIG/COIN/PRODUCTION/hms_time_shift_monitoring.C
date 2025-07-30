void PlotCutRegion(double xmin, double xmax, EColor fcolor, double alpha);
TPaveText* CreateSummaryPaveText();

void hms_time_shift_monitoring() {

  TPad *padptr = (TPad*) gPad;
  padptr->Divide(1,3);

  padptr->cd(1);
  TH2F *htrig_hdc_ref3 = (TH2F*)gDirectory->Get("htrig_hdc_ref3");
  if (!htrig_hdc_ref3) {
    std::cerr << "ERROR: Histogram 'htrig_hdc_ref3' not found in gDirectory!" << std::endl;
    return;  // or throw, or exit, or handle appropriately
  }
  htrig_hdc_ref3->Draw();
  htrig_hdc_ref3->SetStats(0);  
  htrig_hdc_ref3->SetLineColor(kBlack);
  htrig_hdc_ref3->SetLineWidth(2);
  htrig_hdc_ref3->GetXaxis()->CenterTitle();
  htrig_hdc_ref3->GetYaxis()->CenterTitle();  
  PlotCutRegion(2220,2330,kGreen,0.3);  
  padptr->Update();

  padptr->cd(2);
  TH2F *htrig_hFADC_TREF_ROC1_adcPulseTimeRaw = (TH2F*)gDirectory->Get("htrig_hFADC_TREF_ROC1_adcPulseTimeRaw");
  if (!htrig_hFADC_TREF_ROC1_adcPulseTimeRaw) {
    std::cerr << "ERROR: Histogram 'htrig_hFADC_TREF_ROC1_adcPulseTimeRaw' not found in gDirectory!" << std::endl;
    return;  // or throw, or exit, or handle appropriately
  }
  htrig_hFADC_TREF_ROC1_adcPulseTimeRaw->Draw();
  htrig_hFADC_TREF_ROC1_adcPulseTimeRaw->SetStats(0);  
  htrig_hFADC_TREF_ROC1_adcPulseTimeRaw->SetLineColor(kBlack);
  htrig_hFADC_TREF_ROC1_adcPulseTimeRaw->SetLineWidth(2);
  htrig_hFADC_TREF_ROC1_adcPulseTimeRaw->GetXaxis()->CenterTitle();
  htrig_hFADC_TREF_ROC1_adcPulseTimeRaw->GetYaxis()->CenterTitle();  
  PlotCutRegion(2490,3450,kGreen,0.3);
 
  padptr->cd(3);
  TPaveText* pvtxt = CreateSummaryPaveText();  
  pvtxt->Draw();
  padptr->Update();    
}

//----------------------------------------------------------
double UnfoldyNDC(double yNDC) 
/* Returns y value for a given y coordinate in NDC */
{
  gPad->Update();
  //std::cout << gPad->GetY1() << ", " << gPad->GetY2() << "\n";
  return yNDC*(gPad->GetY2()-gPad->GetY1()) + gPad->GetY1();
}
//----------------------------------------------------------
void PlotCutRegion(double xmin, double xmax, EColor fillcolor, double alpha)
/* Plots cut region for given x range */
{
  TBox *cutRegion = new TBox(xmin,UnfoldyNDC(0.075),xmax,UnfoldyNDC(0.875));
  cutRegion->SetFillColorAlpha(fillcolor, 0.3);
  cutRegion->Draw();
}
//----------------------------------------------------------
TPaveText* CreateSummaryPaveText()
/* Function to create a summary canvas (old) */
{  
  TPaveText* pvtxt = new TPaveText(.05, .1, .95, .8);
  pvtxt->SetTextFont(42);
  pvtxt->SetTextSize(0.1);

  pvtxt->AddText("The pulses in the above plots MUST be entirely within the green region.");
  pvtxt->AddText("If not, then stop the run, reset CODA, start a new run.");
  pvtxt->AddText("If the issue persists in the new run, call the RC.");  

  // Highlight key lines
  if (TText* t1 = pvtxt->GetLineWith("The")) t1->SetTextColor(kRed);
  if (TText* t2 = pvtxt->GetLineWith("If not")) t2->SetTextColor(kRed);
  // if (TText* t2 = pvtxt->GetLineWith("If not")) t2->SetTextColor(kRed);  

  return pvtxt;
}
