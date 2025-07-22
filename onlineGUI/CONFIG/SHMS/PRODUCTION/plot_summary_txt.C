/*
  Script to plot the content of the summary canvas
*/

void plot_summary_txt() {
  
  // Grab the original canvas (with text in pad 2)
  TCanvas *ceOVp = (TCanvas*)gDirectory->Get("ceOVp");
  if (!ceOVp) {
    std::cerr << "ERROR: Canvas 'ceOVp' not found in gDirectory!" << std::endl;
    return;
  }

  // cd-ing to the current canvas
  Ecanvas_1->cd();
  // TPad *pad = (TPad*)gPad;
  // pad->cd();

  // Get source pad from ceOVp (pad 2)
  int padIndex = 2;
  TPad* srcPad = (TPad*)ceOVp->GetPad(padIndex);
  if (!srcPad) {
    std::cerr << "ERROR: Pad index " << padIndex << " not found in canvas 'ceOVp'" << std::endl;
    return;
  }
  
  // Clone and draw TPaveTexts from srcPad into gPad
  TIter next(srcPad->GetListOfPrimitives());
  TObject* obj;
  while ((obj = next())) {
    if (obj->InheritsFrom("TPaveText")) {
      TPaveText* orig = (TPaveText*)obj;
      TPaveText* clone = (TPaveText*)orig->Clone();

      // Resize text
      for (int i = 0; i < clone->GetListOfLines()->GetSize(); ++i) {
	TText* line = (TText*)clone->GetLine(i);
	if (line) {
	  line->SetTextFont(42);
	  line->SetTextSize(0.03);  
	}
      }
      
      clone->Draw();
    }
  }
}
