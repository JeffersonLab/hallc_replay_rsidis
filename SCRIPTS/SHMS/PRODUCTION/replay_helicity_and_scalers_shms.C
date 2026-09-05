#include "MultiFileRun.h"

void replay_helicity_and_scalers_shms(Int_t RunNumber=0, Int_t MaxEvent=0,
                                      Int_t FirstEvent=1, Int_t MaxSegment=-1) {

  if(RunNumber == 0) {
    cout << "Enter a Run Number (-1 to exit): ";
    cin >> RunNumber;
    if(RunNumber <= 0) return;
  }
  if(MaxEvent == 0) {
    cout << "\nNumber of Events to analyze: ";
    cin >> MaxEvent;
    if(MaxEvent == 0) {
      cerr << "...Invalid entry\n";
      return;
    }
  }

  const char* RunFileNamePattern = "rsidis_production_%05d.dat.%u";
  vector<string> pathList;
  pathList.push_back(".");
  pathList.push_back("./raw");
  pathList.push_back("./raw/../raw.copiedtotape");
  pathList.push_back("./cache");

  const char* ROOTFileNamePattern =
    "ROOTfiles/scaler_helicity_replay_shms_%d_%d.root";

  // Load only the global parameters needed by the scaler handlers.
  gHcParms->Define("gen_run_number", "Run Number", RunNumber);
  gHcParms->AddString("g_ctp_database_filename", "DBASE/COIN/standard.database");
  gHcParms->Load(gHcParms->GetString("g_ctp_database_filename"), RunNumber);
  gHcParms->Load(gHcParms->GetString("g_ctp_parm_filename"));
  gHcParms->Load(gHcParms->GetString("g_ctp_kinematics_filename"), RunNumber);

  THcConfigEvtHandler* ev125 =
    new THcConfigEvtHandler("HC", "Config Event type 125");
  gHaEvtHandlers->Add(ev125);

  THaEpicsEvtHandler* hcepics =
    new THaEpicsEvtHandler("epics", "HC EPICS event type 182");
  gHaEvtHandlers->Add(hcepics);

  THcScalerEvtHandler* pscaler =
    new THcScalerEvtHandler("P", "Hall C scaler event type 1");
  pscaler->AddEvtType(1);
  pscaler->AddEvtType(2);
  pscaler->AddEvtType(3);
  pscaler->AddEvtType(4);
  pscaler->AddEvtType(5);
  pscaler->AddEvtType(6);
  pscaler->AddEvtType(7);
  pscaler->AddEvtType(129);
  pscaler->SetDelayedType(129);
  pscaler->SetUseFirstEvent(kTRUE);
  gHaEvtHandlers->Add(pscaler);

  THcHelicityScaler* phelscaler =
    new THcHelicityScaler("P", "Hall C helicity scaler");
  phelscaler->SetROC(8);
  phelscaler->SetUseFirstEvent(kTRUE);
  gHaEvtHandlers->Add(phelscaler);

  THcConfigEvtHandler* pconfig =
    new THcConfigEvtHandler("pconfig", "Hall C configuration event handler");
  gHaEvtHandlers->Add(pconfig);

  THcAnalyzer* analyzer = new THcAnalyzer;
  THaEvent* event = new THaEvent;

  vector<string> fileNames = {};
  Int_t iseg = 0;
  while(MaxSegment < 0 || iseg <= MaxSegment) {
    TString codafilename;
    codafilename.Form(RunFileNamePattern, RunNumber, iseg);

    Bool_t foundSegment = kFALSE;
    for(UInt_t ipath = 0; ipath < pathList.size(); ipath++) {
      TString fullPath = TString(pathList[ipath]) + "/" + codafilename;
      if(!gSystem->AccessPathName(fullPath)) {
        foundSegment = kTRUE;
        break;
      }
    }

    if(!foundSegment) {
      if(iseg == 0) {
        cerr << "ERROR: Required segment 0 file " << codafilename
             << " was not found in the replay path list. "
             << "Segment 0 is required to initialize global counters." << endl;
        cerr << "Searched paths:" << endl;
        for(UInt_t ipath = 0; ipath < pathList.size(); ipath++)
          cerr << "  " << pathList[ipath] << endl;
        return;
      }
      cout << "Segment " << iseg << " file " << codafilename
           << " not found. Finished building input file list." << endl;
      break;
    }

    cout << "codafilename = " << codafilename << endl;
    fileNames.emplace_back(codafilename.Data());
    iseg++;
  }

  auto* run = new Podd::MultiFileRun(pathList, fileNames);
  run->SetRunParamClass("THcRunParameters");
  run->SetEventRange(FirstEvent, MaxEvent);
  run->SetNscan(1);
  run->SetDataRequired(0x7);
  run->Print();

  TString ROOTFileName = Form(ROOTFileNamePattern, RunNumber, MaxEvent);
  analyzer->SetCountMode(2);
  analyzer->SetEvent(event);
  analyzer->SetEpicsEvtType(182);
  analyzer->SetCrateMapFileName("MAPS/db_cratemap.dat");
  analyzer->SetOutFile(ROOTFileName.Data());
  analyzer->SetOdefFile("DEF-files/SHMS/EPICS/epics_short.def");
  analyzer->EnablePhysicsEvents(false);
  analyzer->Process(run);
}
