#include <vector>
#include <limits>

void plot_beam_hms_or_shms(TString dir)
{
  // Normalize dir to "X" or "Y"
  dir.ToUpper();
  if (dir != "X" && dir != "Y") {
    Warning("plot_beam", "dir must be \"X\" or \"Y\". Got \"%s\"; defaulting to X.", dir.Data());
    dir = "X";
  }

  const char* bases[4] = {"BPMA","BPMB","BPMC","BPMT"};
  TH1F* h1d[4] = {nullptr, nullptr, nullptr, nullptr};

  // Try h* first, then p* as fallback
  for (int i = 0; i < 4; ++i) {
    TString name = Form("h%s_%s", bases[i], dir.Data());
    h1d[i] = (TH1F*) gDirectory->Get(name);
    if (!h1d[i]) {
      name = Form("p%s_%s", bases[i], dir.Data());
      h1d[i] = (TH1F*) gDirectory->Get(name);
    }
    if (!h1d[i]) {
      Warning("plot_beam", "Missing histogram for %s_%s (tried h* and p*).", bases[i], dir.Data());
    }
  }

  // Z positions
  const double zbpm_all[4] = {-320.82, -224.86, -129.44, 0.0};

  // Collect only the points we actually have
  std::vector<double> z, mean;
  z.reserve(4); mean.reserve(4);
  for (int i = 0; i < 4; ++i) {
    if (h1d[i]) {
      z.push_back(zbpm_all[i]);
      mean.push_back(h1d[i]->GetMean());
    }
  }

  if (z.empty()) {
    Error("plot_beam", "No BPM histograms found (neither h* nor p*). Nothing to plot.");
    return;
  }

  TGraph* grbeam = new TGraph((int)z.size(), z.data(), mean.data());
  grbeam->SetMarkerStyle(20);
  grbeam->SetMarkerSize(1);

  // Axis titles
  grbeam->SetTitle(Form("; Z BPM (cm); %s BPM (cm) (+Y up) ", (dir=="Y") ? "Y" : "X"));

  // Cosmetics and ranges
  if (gPad) {
    gPad->SetGridx();
    gPad->SetGridy();
  }
  grbeam->GetXaxis()->SetLimits(-400, 50);
  grbeam->SetMinimum(-0.5);
  grbeam->SetMaximum( 0.5);

  grbeam->Draw("AP");
}
