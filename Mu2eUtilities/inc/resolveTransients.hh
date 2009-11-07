#ifndef Mu2eUtilities_resolveTransients_HH
#define Mu2eUtilities_resolveTransients_HH

//
// A utility function to compute values for all of the transients
// in members of a collection.
//
// $Id: resolveTransients.hh,v 1.2 2009/11/07 17:04:15 kutschke Exp $
// $Author: kutschke $
// $Date: 2009/11/07 17:04:15 $
//
// Original author Rob Kutschke
//

// Framework includes.
#include "FWCore/Framework/interface/Event.h"

namespace mu2e {

  template<typename T>
  void resolveTransients( T const& v, edm::Event const& event){
    
    for ( typename T::const_iterator i=v.begin(), e=v.end();
	  i<e; ++i ){
      i->resolveTransients(event);
    }
  }

}

#endif
