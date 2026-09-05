#include "MultiFileRun.h"

void replay_helicity_and_scalers_hms(Int_t RunNumber=0, Int_t MaxEvent=0,
                                     Int_t FirstEvent=0, Int_t MaxSegment=-1) {

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
    "ROOTfiles/scaler_helicity_replay_hms_%d_%d.root";

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

  THcScalerEvtHandler* hscaler =
    new THcScalerEvtHandler("H", "Hall C scaler event type 2");
  hscaler->AddEvtType(1);
  hscaler->AddEvtType(2);
  hscaler->AddEvtType(3);
  hscaler->AddEvtType(4);
  hscaler->AddEvtType(5);
  hscaler->AddEvtType(6);
  hscaler->AddEvtType(7);
  hscaler->AddEvtType(131);
  hscaler->SetDelayedType(131);
  hscaler->SetUseFirstEvent(kTRUE);
  gHaEvtHandlers->Add(hscaler);

  THcHelicityScaler* hhelscaler =
    new THcHelicityScaler("H", "Hall C helicity scaler");
  hhelscaler->SetROC(5);
  hhelscaler->SetUseFirstEvent(kTRUE);
  gHaEvtHandlers->Add(hhelscaler);

  THcConfigEvtHandler* hconfig =
    new THcConfigEvtHandler("hconfig", "Hall C configuration event handler");
  gHaEvtHandlers->Add(hconfig);

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
  analyzer->SetOdefFile("DEF-files/HMS/EPICS/epics_short.def");
  analyzer->EnablePhysicsEvents(false);
  analyzer->Process(run);
}
