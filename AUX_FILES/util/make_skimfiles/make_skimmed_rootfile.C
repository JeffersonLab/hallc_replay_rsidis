#include <vector>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <map>
#include <sstream>
#include <stdexcept>
#include "ROOT/RDataFrame.hxx"
#include <stdio.h>
#include <iostream>
#include "TSystem.h"
#include <Math/Vector4D.h>
#include "TLorentzVector.h"
#include "TRotation.h"
#include "TMath.h"
#include "TVector3.h"

const double Mp = 0.93827;
const double Me = 0.000511;
const double HCANA_KBIG = 1.0e38;

// Placeholder corrections. Keep these at zero until the HEEP study provides
// the final arm-dependent values. Momentum is in GeV and angles are in radians.
const double H_GTR_P_OFFSET = 0.0;
const double H_GTR_TH_OFFSET = 0.0;
const double P_GTR_P_OFFSET = 0.0;
const double P_GTR_TH_OFFSET = 0.0;

// Temporary replay-geometry constants. These reproduce the values presently
// loaded by HCANA from PARAM/HMS/GEN/hmsflags.param and
// PARAM/SHMS/GEN/shmsflags.param. Revisit them whenever the replay parameters
// change. HCANA defaults the absent phi_lab parameters to zero.
const double HMS_PHI_LAB_DEG = 0.0;
const double SHMS_PHI_LAB_DEG = 0.0;
const double HMS_PHI_OFFSET_RAD = 2.85e-3;
const double SHMS_PHI_OFFSET_RAD = -8.681269905e-4;
const double HMS_OOPCENTRAL_OFFSET_RAD = 0.0;
const double SHMS_OOPCENTRAL_OFFSET_RAD = 0.0;

struct ReplayParameters
{
  double targetMass = 0.0;
  double hPartMass = 0.0;
  double pPartMass = 0.0;
  double hOopOffset = 0.0;
  double pOopOffset = 0.0;
  TRotation hToLab;
  TRotation pToLab;
};

struct PrimaryKinematics
{
  TLorentzVector beam;
  TLorentzVector scattered;
  TLorentzVector target;
  TLorentzVector q;
  double xbj = 0.0;
  double Q2 = 0.0;
  double nu = 0.0;
  double W = 0.0;
};

struct SecondaryKinematics
{
  double th_xq = 0.0;
  double ph_xq = 0.0;
  double pmiss_y = 0.0;
  double pmiss_z = 0.0;
  double emiss = 0.0;
  double mmiss = 0.0;
};

// Remove leading and trailing whitespace from report tokens and lines.
std::string trim(const std::string &text)
{
  const auto first = text.find_first_not_of(" \t\r\n");
  if (first == std::string::npos)
    return "";
  const auto last = text.find_last_not_of(" \t\r\n");
  return text.substr(first, last - first + 1);
}

// Normalize a report label so harmless capitalization and whitespace changes
// do not affect lookup.
std::string normalize_label(const std::string &text)
{
  std::string result;
  bool pendingSpace = false;
  for (unsigned char character : trim(text))
  {
    if (std::isspace(character))
    {
      pendingSpace = !result.empty();
      continue;
    }
    if (pendingSpace)
      result.push_back(' ');
    pendingSpace = false;
    result.push_back(static_cast<char>(std::tolower(character)));
  }
  return result;
}

// Read every numeric "label: value" field in a replay report. Duplicate
// labels are accepted only when their numeric values agree.
std::map<std::string, double> read_report_values(const std::string &filename)
{
  std::ifstream input(filename);
  if (!input)
    throw std::runtime_error("Cannot open replay report: " + filename);

  std::map<std::string, double> values;
  const std::vector<std::string> wantedLabels = {
      "run #", "target mass (amu)",
      "hms particle mass", "hms angle",
      "shms particle mass", "shms angle"};
  std::string line;
  while (std::getline(input, line))
  {
    const auto colon = line.find(':');
    if (colon == std::string::npos)
      continue;
    const std::string label = normalize_label(line.substr(0, colon));
    if (std::find(wantedLabels.begin(), wantedLabels.end(), label) == wantedLabels.end())
      continue;
    const std::string valueText = trim(line.substr(colon + 1));
    if (label.empty() || valueText.empty())
      continue;
    try
    {
      std::size_t consumed = 0;
      const double value = std::stod(valueText, &consumed);
      const auto existing = values.find(label);
      if (existing != values.end() && std::abs(existing->second - value) > 1.0e-12)
      {
        throw std::runtime_error("Conflicting values for report label '" + label + "'");
      }
      values[label] = value;
    }
    catch (const std::invalid_argument &)
    {
      // Most colon-delimited report lines contain text rather than a number.
    }
  }
  return values;
}

// Return a required numeric report field.
double required_report_value(const std::map<std::string, double> &values,
                             const std::string &label)
{
  const auto found = values.find(normalize_label(label));
  if (found == values.end())
    throw std::runtime_error("Required replay-report label is missing: " + label);
  return found->second;
}

// Build the HCANA spectrometer transport-to-lab rotation from the central
// in-plane and out-of-plane geographic angles, supplied in degrees.
TRotation make_to_lab_rotation(double thetaGeoDeg, double phiGeoDeg)
{
  const double thGeo = thetaGeoDeg * TMath::DegToRad();
  const double phGeo = phiGeoDeg * TMath::DegToRad();
  const double x = std::sin(thGeo) * std::cos(phGeo);
  const double y = std::sin(phGeo);
  const double z = std::cos(thGeo) * std::cos(phGeo);
  const double thSph = std::acos(std::max(-1.0, std::min(1.0, z)));
  const double phSph = std::atan2(y, x);
  const double st = std::sin(thSph), ct = std::cos(thSph);
  const double sp = std::sin(phSph), cp = std::cos(phSph);
  const double norm = std::sqrt(ct * ct + st * st * cp * cp);
  TVector3 nx(st * st * sp * cp / norm, -norm, st * ct * sp / norm);
  TVector3 ny(ct / norm, 0.0, -st * cp / norm);
  TVector3 nz(st * cp, st * sp, ct);
  TRotation rotation;
  rotation.SetToIdentity().RotateAxes(nx, ny, nz);
  return rotation;
}

// Load immutable replay-time parameters from the labeled report fields and
// combine them with the temporary fixed phi/OOP geometry constants.
ReplayParameters load_replay_parameters(const std::string &reportFile, int run,
                                        const std::string &runtype)
{
  const auto values = read_report_values(reportFile);
  const int reportedRun = static_cast<int>(std::llround(required_report_value(values, "Run #")));
  if (reportedRun != run)
    throw std::runtime_error("Report run " + std::to_string(reportedRun) +
                             " does not match requested run " + std::to_string(run));

  ReplayParameters result;
  result.targetMass = required_report_value(values, "Target mass (amu)") * 0.9315;
  result.hOopOffset = HMS_OOPCENTRAL_OFFSET_RAD;
  result.pOopOffset = SHMS_OOPCENTRAL_OFFSET_RAD;

  const bool hasHMS = runtype != "SHMSDIS";
  const bool hasSHMS = runtype != "HMSDIS";
  if (hasHMS)
  {
    result.hPartMass = required_report_value(values, "HMS Particle Mass");
    const double hPhi = HMS_PHI_LAB_DEG + HMS_PHI_OFFSET_RAD * TMath::RadToDeg();
    result.hToLab = make_to_lab_rotation(required_report_value(values, "HMS Angle"), hPhi);
  }
  if (hasSHMS)
  {
    result.pPartMass = required_report_value(values, "SHMS Particle Mass");
    const double pPhi = SHMS_PHI_LAB_DEG + SHMS_PHI_OFFSET_RAD * TMath::RadToDeg();
    result.pToLab = make_to_lab_rotation(required_report_value(values, "SHMS Angle"), pPhi);
  }
  return result;
}

// Convert transport coordinates (p, theta, phi) into a lab-frame momentum
// using the same slope convention and rotation as HCANA.
TVector3 transport_to_lab(double p, double th, double ph,
                          double oopOffset, const TRotation &rotation)
{
  TVector3 vector(th + oopOffset, ph, 1.0);
  vector *= p / std::sqrt(1.0 + std::pow(th + oopOffset, 2) + ph * ph);
  return rotation * vector;
}

// Build primary electron kinematics from the event beam momentum and the
// reconstructed scattered-electron lab momentum. SetVectM applies the same
// massive-electron energy relation used by HCANA.
PrimaryKinematics calculate_primary(const TVector3 &scatteredMomentum,
                                    const TVector3 &beamMomentum,
                                    double targetMass)
{
  PrimaryKinematics result;
  result.beam.SetVectM(beamMomentum, Me);
  result.scattered.SetVectM(scatteredMomentum, Me);
  result.target.SetXYZM(0.0, 0.0, 0.0, targetMass);
  result.q = result.beam - result.scattered;
  result.Q2 = -result.q.M2();
  result.nu = result.q.E();
  TLorentzVector proton;
  proton.SetXYZM(0.0, 0.0, 0.0, Mp);
  const double W2 = (proton + result.q).M2();
  result.W = W2 > 0.0 ? std::sqrt(W2) : HCANA_KBIG;
  result.xbj = result.Q2 / (2.0 * Mp * result.nu);
  return result;
}

// Reproduce the required THcSecondaryKine quantities from reconstructed
// primary kinematics and the detected secondary-particle lab momentum.
SecondaryKinematics calculate_secondary(const PrimaryKinematics &primary,
                                        const TVector3 &secondaryMomentum,
                                        double secondaryMass)
{
  SecondaryKinematics result;
  TLorentzVector detected;
  detected.SetVectM(secondaryMomentum, secondaryMass);
  const TLorentzVector recoil = primary.target + primary.q - detected;
  TRotation toQ;
  toQ.SetZAxis(primary.q.Vect(), primary.scattered.Vect()).Invert();
  TVector3 xq = detected.Vect();
  TVector3 bq = recoil.Vect();
  xq *= toQ;
  bq *= toQ;
  const TVector3 pmiss = -bq;
  result.th_xq = xq.Theta();
  result.ph_xq = xq.Phi();
  result.pmiss_y = pmiss.Y();
  result.pmiss_z = pmiss.Z();
  result.emiss = primary.nu + primary.target.M() - detected.E();
  result.mmiss = std::sqrt(std::abs(result.emiss * result.emiss - pmiss.Mag2()));
  return result;
}

// Calculate the invariant mass of the undetected recoil system from HCANA's
// original virtual photon and a massive detected-particle four-vector.
double calculate_original_missing_mass(double qx, double qy, double qz, double nu,
                                       double px, double py, double pz,
                                       double targetMass, double detectedMass)
{
  TLorentzVector target;
  target.SetXYZM(0.0, 0.0, 0.0, targetMass);
  TLorentzVector q(qx, qy, qz, nu);
  TLorentzVector detected;
  detected.SetVectM(TVector3(px, py, pz), detectedMass);
  const TLorentzVector recoil = target + q - detected;
  return std::sqrt(std::abs(recoil.M2()));
}

// Common SHMS vairables
std::vector<std::string> shmsVars = {
    "P.gtr.x", "P.gtr.y", "P.gtr.dp", "P.gtr.p", "P.gtr.ph", "P.gtr.th", "P.gtr.beta", "P.gtr.index",
    "P.dc.x_fp", "P.dc.y_fp", "P.dc.xp_fp", "P.dc.yp_fp", "P.dc.InsideDipoleExit",
    "P.ngcer.npeSum", "P.hgcer.npeSum", "P.aero.npeSum", "P.cal.etottracknorm",
    "P.react.x", "P.react.y", "P.react.z",
    "P.hod.goodstarttime","P.hod.fpHitsTime"};

// SHMS DIS kin & raster variables
std::vector<std::string> shmskinVars = {
    "P.kin.x_bj", "P.kin.Q2", "P.kin.nu", "P.kin.W",
    "P.rb.raster.frxaRawAdc", "P.rb.raster.frxbRawAdc", "P.rb.raster.fryaRawAdc", "P.rb.raster.frybRawAdc"};

// Common HMS variables
std::vector<std::string> hmsVars = {
    "H.gtr.x", "H.gtr.y", "H.gtr.dp", "H.gtr.p", "H.gtr.ph", "H.gtr.th", "H.gtr.beta", "H.gtr.index",
    "H.dc.x_fp", "H.dc.y_fp", "H.dc.xp_fp", "H.dc.yp_fp", "H.dc.InsideDipoleExit",
    "H.cer.npeSum", "H.cal.etottracknorm",
    "H.react.x", "H.react.y", "H.react.z",
    "H.hod.goodstarttime","H.hod.fpHitsTime"};

// HMS DIS kin & raster variables
std::vector<std::string> hmskinVars = {
    "H.kin.x_bj", "H.kin.Q2", "H.kin.nu", "H.kin.W",
    "H.rb.raster.fr_xa", "H.rb.raster.fr_xb", "H.rb.raster.fr_ya", "H.rb.raster.fr_yb"};

// Common coin data (SIDIS) vairiables
std::vector<std::string> ctimeVars = {
    "CTime.ePiCoinTime_ROC1", "CTime.ePiCoinTime_ROC2",
    "CTime.epCoinTime_ROC1", "CTime.epCoinTime_ROC2",
    "CTime.CoinTime_RAW_ROC1", "CTime.CoinTime_RAW_ROC2",
    "T.coin.hRF_tdcTime", "T.coin.pRF_tdcTime",
    "H.kin.primary.x_bj", "H.kin.primary.Q2", "H.kin.primary.nu", "H.kin.primary.W",
    "P.kin.secondary.th_xq", "P.kin.secondary.ph_xq", "P.kin.secondary.MMpi",
    "T.helicity.helicity", "T.helicity.hel",
    "H.rb.raster.frxaRawAdc", "H.rb.raster.frxbRawAdc", "H.rb.raster.fryaRawAdc", "H.rb.raster.frybRawAdc",
    "P.rb.raster.frxaRawAdc", "P.rb.raster.frxbRawAdc", "P.rb.raster.fryaRawAdc", "P.rb.raster.frybRawAdc"};

std::vector<std::string> hmsHeepVars = {
    "CTime.epCoinTime_ROC1", "CTime.epCoinTime_ROC2",
    "CTime.CoinTime_RAW_ROC1", "CTime.CoinTime_RAW_ROC2",
    "T.coin.hRF_tdcTime", "T.coin.pRF_tdcTime",
    "H.kin.primary.x_bj", "H.kin.primary.Q2", "H.kin.primary.nu", "H.kin.primary.W",
    "P.kin.secondary.th_xq", "P.kin.secondary.ph_xq",
    "P.kin.secondary.emiss", "P.kin.secondary.pmiss_y", "P.kin.secondary.pmiss_z",
    "H.rb.raster.frxaRawAdc", "H.rb.raster.frxbRawAdc", "H.rb.raster.fryaRawAdc", "H.rb.raster.frybRawAdc",
    "P.rb.raster.frxaRawAdc", "P.rb.raster.frxbRawAdc", "P.rb.raster.fryaRawAdc", "P.rb.raster.frybRawAdc"};

std::vector<std::string> shmsHeepVars = {
    "CTime.epCoinTime_ROC1", "CTime.epCoinTime_ROC2",
    "CTime.CoinTime_RAW_ROC1", "CTime.CoinTime_RAW_ROC2",
    "T.coin.hRF_tdcTime", "T.coin.pRF_tdcTime",
    "P.kin.primary.x_bj", "P.kin.primary.Q2", "P.kin.primary.nu", "P.kin.primary.W",
    "H.kin.secondary.th_xq", "H.kin.secondary.ph_xq",
    "H.kin.secondary.emiss", "H.kin.secondary.pmiss_y", "H.kin.secondary.pmiss_z",
    "H.rb.raster.frxaRawAdc", "H.rb.raster.frxbRawAdc", "H.rb.raster.fryaRawAdc", "H.rb.raster.frybRawAdc",
    "P.rb.raster.frxaRawAdc", "P.rb.raster.frxbRawAdc", "P.rb.raster.fryaRawAdc", "P.rb.raster.frybRawAdc"};

// // BCM variables
// std::vector<std::string> bcmVars = {
//   "ibcm1", "ibcm2"
// };

// Assemble the snapshot branch list appropriate to the requested run type.
std::vector<std::string> get_varnames(const std::string &runtype)
{
  auto concat = [](std::vector<std::string> base,
                   const std::vector<std::string> &add)
  {
    base.insert(base.end(), add.begin(), add.end());
    return base;
  };

  if (runtype == "SIDIS")
  {
    auto vars = shmsVars;
    vars.insert(vars.end(), hmsVars.begin(), hmsVars.end());
    vars.insert(vars.end(), ctimeVars.begin(), ctimeVars.end());
    // vars.insert(vars.end(), bcmVars.begin(), bcmVars.end());
    return vars;
  }
  else if (runtype == "HMSHEEP")
  {
    auto vars = shmsVars;
    vars.insert(vars.end(), hmsVars.begin(), hmsVars.end());
    vars.insert(vars.end(), hmsHeepVars.begin(), hmsHeepVars.end());
    return vars;
  }
  else if (runtype == "SHMSHEEP")
  {
    auto vars = shmsVars;
    vars.insert(vars.end(), hmsVars.begin(), hmsVars.end());
    vars.insert(vars.end(), shmsHeepVars.begin(), shmsHeepVars.end());
    // vars.insert(vars.end(), bcmVars.begin(), bcmVars.end());
    return vars;
  }
  else if (runtype == "SHMSDIS")
  {
    auto vars = shmsVars;
    vars.insert(vars.end(), shmskinVars.begin(), shmskinVars.end());
    // vars.insert(vars.end(), bcmVars.begin(), bcmVars.end());
    return vars;
  }
  else if (runtype == "HMSDIS")
  {
    auto vars = hmsVars;
    vars.insert(vars.end(), hmskinVars.begin(), hmskinVars.end());
    // vars.insert(vars.end(), bcmVars.begin(), bcmVars.end());
    return vars;
  }
  else
  {
    std::cout << "Invalid runtype specified! Choose from SIDIS, HMSHEEP, SHMSHEEP, SHMSDIS, HMSDIS.\n";
    return {};
  }
}

// Return the existing loose analysis cut expression for each supported run
// type; reconstructed branches deliberately do not alter event selection.
std::string get_anacuts(std::string runtype)
{
  std::string hmscuts_gen = "H.gtr.index>-1 && abs(H.gtr.dp)<12. ";
  std::string hmscuts_pid = "H.cer.npeSum>1"; // HMS PID cut for electrons
  std::string hmscuts = hmscuts_gen + " && " + hmscuts_pid;
  std::string shmscuts = "P.gtr.index>-1 && abs(P.gtr.dp)<30.";
  std::string sidiscuts = hmscuts + " && " + shmscuts;
  std::string heepcuts = hmscuts_gen + " && " + shmscuts;

  if (runtype == "SIDIS")
  {
    return sidiscuts;
  }
  else if (runtype == "HMSHEEP" || runtype == "SHMSHEEP")
  {
    return heepcuts;
  }
  else if (runtype == "SHMSDIS")
  {
    return shmscuts;
  }
  else if (runtype == "HMSDIS")
  {
    return hmscuts;
  }
  else
  {
    std::cout << "Invalid runtype specified! Choose from SIDIS, HMSHEEP, SHMSHEEP, SHMSDIS, HMSDIS.\n";
    return "";
  }
}

// Add a generated branch to an output list without creating duplicates.
auto add_if_missing = [](std::vector<std::string> &vec, const std::string &item)
{
  if (std::find(vec.begin(), vec.end(), item) == vec.end())
  {
    vec.push_back(item);
  }
};

// Report peak resident memory on Linux for farm-job resource monitoring.
void log_peak_memory()
{
  std::ifstream status_file("/proc/self/status");
  std::string label;
  while (status_file >> label)
  {
    if (label == "VmHWM:")
    { // Peak resident set size
      unsigned long peak_kb;
      status_file >> peak_kb;
      std::string unit;
      status_file >> unit;
      std::cout << "[MEMORY] Peak RSS: " << peak_kb << " kB" << std::endl;
      break;
    }
  }
  // Handy unix command
  // grep "\[MEMORY\]" *.out | sort -nk4
}

// Build one skim: validate its run type, apply the original loose cuts, define
// reconstructed branches, and snapshot the selected original and new columns.
void make_skimmed_rootfile(int run,             // run number to process
                           std::string runtype, // SIDIS, HMSHEEP, SHMSHEEP, SHMSDIS, HMSDIS
                           std::string indir,   // input directory containing replayed root files
                           std::string outdir,  // output directory to save skimmed root files
                           std::string reportdir = "" // replay-report directory; defaults to indir
)
{
  const std::vector<std::string> validRuntypes = {
      "SIDIS", "HMSHEEP", "SHMSHEEP", "SHMSDIS", "HMSDIS"};
  if (std::find(validRuntypes.begin(), validRuntypes.end(), runtype) == validRuntypes.end())
  {
    std::cerr << "Invalid runtype '" << runtype
              << "'. Choose SIDIS, HMSHEEP, SHMSHEEP, SHMSDIS, or HMSDIS.\n";
    if (runtype == "HEEP")
      std::cerr << "HEEP is ambiguous; use HMSHEEP or SHMSHEEP explicitly.\n";
    return;
  }

  // Defining input root file name based on run type
  std::string rfilename = Form("coin_replay_production_%d_-1.root", run); // SIDIS and HEEP
  if (runtype == "SHMSDIS")
  {
    rfilename = Form("shms_coin_replay_production_%d_-1.root", run);
  }
  else if (runtype == "HMSDIS")
  {
    rfilename = Form("hms_coin_replay_production_%d_-1.root", run);
  }
  std::string inrootfile = Form("%s/%s", indir.c_str(), rfilename.c_str());

  if (reportdir.empty())
    reportdir = indir;
  std::string reportFilename = Form("replay_coin_production_%d_-1.report", run);
  if (runtype == "SHMSDIS")
    reportFilename = Form("replay_shms_coin_production_%d_-1.report", run);
  else if (runtype == "HMSDIS")
    reportFilename = Form("replay_hms_coin_production_%d_-1.report", run);
  const std::string reportFile = Form("%s/%s", reportdir.c_str(), reportFilename.c_str());

  // Check if input root file exists
  if (gSystem->AccessPathName(inrootfile.c_str()))
  {
    std::cerr << "Input root file does not exist: " << inrootfile << std::endl;
    return;
  }
  if (gSystem->AccessPathName(reportFile.c_str()))
  {
    std::cerr << "Replay report does not exist: " << reportFile << std::endl;
    return;
  }

  // Create RDataFrame from the input root file
  ROOT::RDataFrame df("T", inrootfile.c_str());

  // apply loose analysis cuts
  auto df_filtered = df.Filter(get_anacuts(runtype));

  ReplayParameters parameters;
  try
  {
    parameters = load_replay_parameters(reportFile, run, runtype);
  }
  catch (const std::exception &error)
  {
    std::cerr << "Failed to load replay parameters for run " << run
              << ": " << error.what() << std::endl;
    return;
  }

  const bool hasHMS = runtype != "SHMSDIS";
  const bool hasSHMS = runtype != "HMSDIS";
  const bool primaryIsHMS = runtype == "SIDIS" || runtype == "HMSHEEP" || runtype == "HMSDIS";
  const std::string primaryArm = primaryIsHMS ? "H" : "P";

  if (hasHMS)
  {
    df_filtered = df_filtered
                      .Define("H_gtr_p_recon", [](double value) { return value + H_GTR_P_OFFSET; }, {"H.gtr.p"})
                      .Define("H_gtr_th_recon", [](double value) { return value + H_GTR_TH_OFFSET; }, {"H.gtr.th"})
                      .Define("_H_lab_recon", [parameters](double pRecon, double thRecon, double ph)
                              { return transport_to_lab(pRecon, thRecon, ph,
                                                        parameters.hOopOffset, parameters.hToLab); },
                              {"H_gtr_p_recon", "H_gtr_th_recon", "H.gtr.ph"});
    add_if_missing(hmsVars, "H_gtr_p_recon");
    add_if_missing(hmsVars, "H_gtr_th_recon");
  }

  if (hasSHMS)
  {
    df_filtered = df_filtered
                      .Define("P_gtr_p_recon", [](double value) { return value + P_GTR_P_OFFSET; }, {"P.gtr.p"})
                      .Define("P_gtr_th_recon", [](double value) { return value + P_GTR_TH_OFFSET; }, {"P.gtr.th"})
                      .Define("_P_lab_recon", [parameters](double pRecon, double thRecon, double ph)
                              { return transport_to_lab(pRecon, thRecon, ph,
                                                        parameters.pOopOffset, parameters.pToLab); },
                              {"P_gtr_p_recon", "P_gtr_th_recon", "P.gtr.ph"});
    add_if_missing(shmsVars, "P_gtr_p_recon");
    add_if_missing(shmsVars, "P_gtr_th_recon");
  }

  // Build the incident-electron four-vector from HCANA's event-level
  // raster-beam momentum. These branches are required skim inputs.
  const std::string beamColumn = "_beam_recon";
  const std::vector<std::string> rasterBeamInputs = {
      primaryArm + ".rb.px", primaryArm + ".rb.py", primaryArm + ".rb.pz"};
  const bool hasRasterBeam = df.HasColumn(rasterBeamInputs[0]) &&
                             df.HasColumn(rasterBeamInputs[1]) &&
                             df.HasColumn(rasterBeamInputs[2]);
  if (!hasRasterBeam)
  {
    std::cerr << "Required raster-beam momentum branches are missing: "
              << primaryArm << ".rb.{px,py,pz}" << std::endl;
    return;
  }
  std::cout << "Reconstructed beam source: " << primaryArm
            << ".rb.{px,py,pz}\n";
  df_filtered = df_filtered.Define(beamColumn,
                                   [](double px, double py, double pz)
                                   { return TVector3(px, py, pz); },
                                   rasterBeamInputs);

  const std::string primaryColumn = "_primary_recon";
  if (primaryIsHMS)
  {
    df_filtered = df_filtered.Define(primaryColumn,
                                     [parameters](const TVector3 &momentum, const TVector3 &beam)
                                     { return calculate_primary(momentum, beam, parameters.targetMass); },
                                     {"_H_lab_recon", beamColumn});
  }
  else
  {
    df_filtered = df_filtered.Define(primaryColumn,
                                     [parameters](const TVector3 &momentum, const TVector3 &beam)
                                     { return calculate_primary(momentum, beam, parameters.targetMass); },
                                     {"_P_lab_recon", beamColumn});
  }

  const std::string primaryPrefix = primaryIsHMS ?
      ((runtype == "HMSDIS") ? "H_kin_" : "H_kin_primary_") :
      ((runtype == "SHMSDIS") ? "P_kin_" : "P_kin_primary_");
  const std::string xbjRecon = primaryPrefix + "x_bj_recon";
  const std::string q2Recon = primaryPrefix + "Q2_recon";
  const std::string nuRecon = primaryPrefix + "nu_recon";
  const std::string wRecon = primaryPrefix + "W_recon";
  df_filtered = df_filtered
                    .Define(xbjRecon, [](const PrimaryKinematics &value) { return value.xbj; }, {primaryColumn})
                    .Define(q2Recon, [](const PrimaryKinematics &value) { return value.Q2; }, {primaryColumn})
                    .Define(nuRecon, [](const PrimaryKinematics &value) { return value.nu; }, {primaryColumn})
                    .Define(wRecon, [](const PrimaryKinematics &value) { return value.W; }, {primaryColumn});

  std::vector<std::string> *primaryOutput = nullptr;
  if (runtype == "SIDIS") primaryOutput = &ctimeVars;
  else if (runtype == "HMSHEEP") primaryOutput = &hmsHeepVars;
  else if (runtype == "SHMSHEEP") primaryOutput = &shmsHeepVars;
  else if (runtype == "HMSDIS") primaryOutput = &hmskinVars;
  else primaryOutput = &shmskinVars;
  add_if_missing(*primaryOutput, xbjRecon);
  add_if_missing(*primaryOutput, q2Recon);
  add_if_missing(*primaryOutput, nuRecon);
  add_if_missing(*primaryOutput, wRecon);

  if (runtype == "SIDIS" || runtype == "HMSHEEP" || runtype == "SHMSHEEP")
  {
    const bool secondaryIsHMS = runtype == "SHMSHEEP";
    const std::string secondaryColumn = "_secondary_recon";
    if (secondaryIsHMS)
    {
      df_filtered = df_filtered.Define(secondaryColumn,
                                       [parameters](const PrimaryKinematics &primary, const TVector3 &momentum)
                                       { return calculate_secondary(primary, momentum, parameters.hPartMass); },
                                       {primaryColumn, "_H_lab_recon"});
    }
    else
    {
      df_filtered = df_filtered.Define(secondaryColumn,
                                       [parameters](const PrimaryKinematics &primary, const TVector3 &momentum)
                                       { return calculate_secondary(primary, momentum, parameters.pPartMass); },
                                       {primaryColumn, "_P_lab_recon"});
    }

    const std::string secondaryPrefix = secondaryIsHMS ? "H_kin_secondary_" : "P_kin_secondary_";
    const std::string thxqRecon = secondaryPrefix + "th_xq_recon";
    const std::string phxqRecon = secondaryPrefix + "ph_xq_recon";
    df_filtered = df_filtered
                      .Define(thxqRecon, [](const SecondaryKinematics &value) { return value.th_xq; }, {secondaryColumn})
                      .Define(phxqRecon, [](const SecondaryKinematics &value) { return value.ph_xq; }, {secondaryColumn});

    std::vector<std::string> *secondaryOutput = runtype == "SIDIS" ? &ctimeVars :
                                                 (runtype == "HMSHEEP" ? &hmsHeepVars : &shmsHeepVars);
    add_if_missing(*secondaryOutput, thxqRecon);
    add_if_missing(*secondaryOutput, phxqRecon);

    if (runtype == "HMSHEEP" || runtype == "SHMSHEEP")
    {
      const std::string emissRecon = secondaryPrefix + "emiss_recon";
      const std::string pmyRecon = secondaryPrefix + "pmiss_y_recon";
      const std::string pmzRecon = secondaryPrefix + "pmiss_z_recon";
      df_filtered = df_filtered
                        .Define(emissRecon, [](const SecondaryKinematics &value) { return value.emiss; }, {secondaryColumn})
                        .Define(pmyRecon, [](const SecondaryKinematics &value) { return value.pmiss_y; }, {secondaryColumn})
                        .Define(pmzRecon, [](const SecondaryKinematics &value) { return value.pmiss_z; }, {secondaryColumn});
      add_if_missing(*secondaryOutput, emissRecon);
      add_if_missing(*secondaryOutput, pmyRecon);
      add_if_missing(*secondaryOutput, pmzRecon);
    }
  }

  if (runtype == "SIDIS")
  {
    std::cout << "Adding extra columns for SIDIS run type...\n";

    std::string pt = "sqrt(pow(P.gtr.p,2)*(1.-pow(cos(P.kin.secondary.th_xq),2)))";
    std::string ptx = pt + "*cos(P.kin.secondary.ph_xq)";
    std::string pty = pt + "*sin(P.kin.secondary.ph_xq)";
    auto calc_mm = [parameters](double qx, double qy, double qz, double nu,
                                double px, double py, double pz)
    {
      return calculate_original_missing_mass(qx, qy, qz, nu, px, py, pz,
                                             parameters.targetMass,
                                             parameters.pPartMass);
    };

    // defining new columns
    df_filtered = df_filtered
                      .Define("z_wHadMom", [](double p, double nu) { return p / nu; },
                              {"P.gtr.p", "H.kin.primary.nu"})
                      .Define("z_wHadEn", [parameters](double p, double nu)
                              { return std::sqrt(p * p + parameters.pPartMass * parameters.pPartMass) / nu; },
                              {"P.gtr.p", "H.kin.primary.nu"})
                      .Define("pt", pt.c_str())
                      .Define("ptx", ptx.c_str())
                      .Define("pty", pty.c_str())
                      .Define("mmass", calc_mm,
                              {"H.kin.primary.q_x", "H.kin.primary.q_y",
                               "H.kin.primary.q_z", "H.kin.primary.nu",
                               "P.gtr.px", "P.gtr.py", "P.gtr.pz"});

    df_filtered = df_filtered
                      .Define("z_wHadMom_recon", [](double p, double nu) { return p / nu; },
                              {"P_gtr_p_recon", nuRecon})
                      .Define("z_wHadEn_recon", [parameters](double p, double nu)
                              { return std::sqrt(p * p + parameters.pPartMass * parameters.pPartMass) / nu; },
                              {"P_gtr_p_recon", nuRecon})
                      .Define("pt_recon", [](double p, double theta) { return p * std::sin(theta); },
                              {"P_gtr_p_recon", "P_kin_secondary_th_xq_recon"})
                      .Define("ptx_recon", [](double ptValue, double phi) { return ptValue * std::cos(phi); },
                              {"pt_recon", "P_kin_secondary_ph_xq_recon"})
                      .Define("pty_recon", [](double ptValue, double phi) { return ptValue * std::sin(phi); },
                              {"pt_recon", "P_kin_secondary_ph_xq_recon"})
                      .Define("mmass_recon", [](const SecondaryKinematics &value) { return value.mmiss; },
                              {"_secondary_recon"});

    // add these new variables to ctimeVars for output
    add_if_missing(ctimeVars, "z_wHadMom");
    add_if_missing(ctimeVars, "z_wHadEn");
    add_if_missing(ctimeVars, "pt");
    add_if_missing(ctimeVars, "ptx");
    add_if_missing(ctimeVars, "pty");
    add_if_missing(ctimeVars, "mmass");
    add_if_missing(ctimeVars, "z_wHadMom_recon");
    add_if_missing(ctimeVars, "z_wHadEn_recon");
    add_if_missing(ctimeVars, "pt_recon");
    add_if_missing(ctimeVars, "ptx_recon");
    add_if_missing(ctimeVars, "pty_recon");
    add_if_missing(ctimeVars, "mmass_recon");
  }

  // write out the skimmed root files
  df_filtered.Snapshot(
      "T",
      Form("%s/skimmed_%s", outdir.c_str(), rfilename.c_str()),
      get_varnames(runtype));
  std::cout << "Skimmed root file created for run " << run << "\n";

  // Log peak memory
  log_peak_memory();
}
