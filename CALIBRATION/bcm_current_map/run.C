// R__LOAD_LIBRARY(ScalerCalib_C)
// cdaq ROOT places the ACLiC library under .root_build_dir, while the bare
// library name (ScalerCalib_C) in run.C is not found through ROOT's search
// path. So, I commented out this line and load the library from root prompt
// by running: .L ScalerCalib.C+

void run(string fin="fin.root", string spec_name="H")
{

  //H: HMS, P: SHMS
  ScalerCalib scalib(spec_name);
  scalib.SetInputFile(fin);
  scalib.SetPrintFlag(1); //0: bcm1 and bcm2 only, 1: all
  scalib.Run();

}


