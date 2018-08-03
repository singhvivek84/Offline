#ifndef __TrkPatRec_RobustHelixFinder_types_hh__
#define __TrkPatRec_RobustHelixFinder_types_hh__

#include "TObject.h"
#include <vector>

class TH1F;
class TH2F;

namespace art {
  class Event;
};

namespace fhicl {
  class ParameterSet;
};

//#include "RecoDataProducts/inc/KalSeed.hh"

namespace mu2e {

  class RobustHelixFit;

  namespace RobustHelixFinderTypes {
  
    struct Data_t {
      const art::Event*               event;
      RobustHelixFit*                 result;
      fhicl::ParameterSet*            timeOffsets;

      enum  { kMaxHelicities = 2, kMaxSeeds = 100 };
      
      int     nTimePeaks;               // number of time peaks (input)
      int     nseeds   [kMaxHelicities]; // 0:all, 1:nhits > nhitsMin; assume nseeds <= 100
      int     ntclhits [kMaxHelicities][kMaxSeeds];
      int     nhits    [kMaxHelicities][kMaxSeeds];
      int     ntriplet0[kMaxHelicities][kMaxSeeds];
      int     ntriplet1[kMaxHelicities][kMaxSeeds];
      int     ntriplet2[kMaxHelicities][kMaxSeeds];
      int     xyniter  [kMaxHelicities][kMaxSeeds];
      int     fzniter  [kMaxHelicities][kMaxSeeds];
      int     niter    [kMaxHelicities][kMaxSeeds];
      int     nrescuedhits[kMaxHelicities][kMaxSeeds];
      int     nXYSh    [kMaxHelicities][kMaxSeeds];
      int     nZPhiSh  [kMaxHelicities][kMaxSeeds];

      int     nshsxy_0    [kMaxHelicities][kMaxSeeds];
      double  rsxy_0      [kMaxHelicities][kMaxSeeds];
      double  chi2dsxy_0  [kMaxHelicities][kMaxSeeds];
                  
      int     nshsxy_1    [kMaxHelicities][kMaxSeeds];
      double  rsxy_1      [kMaxHelicities][kMaxSeeds];
      double  chi2dsxy_1  [kMaxHelicities][kMaxSeeds];
      
      int     nfz0counter [kMaxHelicities][kMaxSeeds];
                  
      int     nshszphi_1  [kMaxHelicities][kMaxSeeds];
      double  lambdaszphi_1[kMaxHelicities][kMaxSeeds];
      double  chi2dszphi_1[kMaxHelicities][kMaxSeeds];

      double  rinit    [kMaxHelicities][kMaxSeeds];
      double  radius   [kMaxHelicities][kMaxSeeds];
      double  lambda0  [kMaxHelicities][kMaxSeeds];
      double  lambda1  [kMaxHelicities][kMaxSeeds];
      double  chi2XY   [kMaxHelicities][kMaxSeeds];
      double  chi2ZPhi [kMaxHelicities][kMaxSeeds];
      double  pT       [kMaxHelicities][kMaxSeeds];
      double  p        [kMaxHelicities][kMaxSeeds];
      double  dr       [kMaxHelicities][kMaxSeeds];
      double  chi2d_helix[kMaxHelicities][kMaxSeeds];
      int maxSeeds() { return kMaxSeeds; }
      
    };
  }
}
#endif
