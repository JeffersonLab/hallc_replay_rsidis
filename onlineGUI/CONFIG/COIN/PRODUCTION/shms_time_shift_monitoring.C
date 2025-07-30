void PlotCutRegion(double xmin, double xmax, EColor fcolor, double alpha);
TPaveText* CreateSummaryPaveText();

void shms_time_shift_monitoring() {

  TPad *padptr = (TPad*) gPad;
  padptr->Divide(1,3);

  padptr->cd(1);
  TH2F *ptrig_pdc_ref7 = (TH2F*)gDirectory->Get("ptrig_pdc_ref7");
  if (!ptrig_pdc_ref7) {
    std::cerr << "ERROR: Histogram 'ptrig_pdc_ref7' not found in gDirectory!" << std::endl;
    return;  // or throw, or exit, or handle appropriately
  }
  ptrig_pdc_ref7->Draw();
  ptrig_pdc_ref7->SetLineColor(kBlack);
  ptrig_pdc_ref7->SetLineWidth(2);
  ptrig_pdc_ref7->GetXaxis()->CenterTitle();
  ptrig_pdc_ref7->GetYaxis()->CenterTitle();  
  PlotCutRegion(2135,2275,kGreen,0.3);  
  padptr->Update();

  padptr->cd(2);
  TH2F *pcal_shwr_raw_atime = (TH2F*)gDirectory->Get("pcal_shwr_raw_atime");
  if (!pcal_shwr_raw_atime) {
    std::cerr << "ERROR: Histogram 'pcal_shwr_raw_atime' not found in gDirectory!" << std::endl;
    return;  // or throw, or exit, or handle appropriately
  }
  pcal_shwr_raw_atime->Draw();
  pcal_shwr_raw_atime->SetLineColor(kBlack);
  pcal_shwr_raw_atime->SetLineWidth(2);
  pcal_shwr_raw_atime->GetXaxis()->CenterTitle();
  pcal_shwr_raw_atime->GetYaxis()->CenterTitle();  
  PlotCutRegion(1440,3680,kGreen,0.3);
 
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
