#include "unshittify.hh"

// root crap
#include "TROOT.h"
#include "TSystem.h"
#include "TError.h"

// all the things needed to unshittify root
void unshittify() {
  // disable signals
  for (int sig = 0; sig < kMAXSIGNALS; sig++) {
    gSystem->ResetSignal((ESignals)sig);
  }
  // // make vectors work
  // gROOT->ProcessLine("#include <vector>");

  // ignore all non-fatal errors
  // gErrorIgnoreLevel = kFatal;
}

