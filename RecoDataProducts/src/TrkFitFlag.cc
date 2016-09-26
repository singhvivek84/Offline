//
// Class to describe flag bits used for straw hits
//
// $Id: TrkFitFlag.cc,v 1.4 2013/04/04 01:08:20 brownd Exp $
// $Author: brownd $
// $Date: 2013/04/04 01:08:20 $
//
// Original author David Brown
//
// Mu2e includes
#include "RecoDataProducts/inc/TrkFitFlag.hh"
#include <stdexcept>
#include <iostream>
#include <stdio.h>

namespace mu2e {

  std::string const& TrkFitFlagDetail::typeName() {
    static std::string type("TrkFitFlag");
    return type;
  }

  std::map<std::string,TrkFitFlagDetail::mask_type> const& TrkFitFlagDetail::bitNames() {
    static std::map<std::string,mask_type> bitnames;
    if(bitnames.size()==0){
      bitnames[std::string("HitsOK")]             = bit_to_mask(hitsOK);
      bitnames[std::string("CircleOK")]           = bit_to_mask(circleOK);
      bitnames[std::string("HelixOK")]              = bit_to_mask(helixOK);
      bitnames[std::string("SeedOK")]              = bit_to_mask(seedOK);
      bitnames[std::string("KalmanOK")]              = bit_to_mask(kalmanOK);
    }
    return bitnames;
  }

}
