// Detail class used to implement the MVAStruct for TrkQual, the track quality
// measure
// D. Brown, LBNL 1/30/2017
//
#ifndef RecoDataProducts_TrkQual_hh
#define RecoDataProducts_TrkQual_hh
//
// Class to describe flag bits used for straw hits
// 
// $Id: TrkQual.hh,v 1.4 2013/04/04 01:08:20 brownd Exp $
// $Author: brownd $
// $Date: 2013/04/04 01:08:20 $
//
// Original author David Brown
//
// Mu2e includes
#include "GeneralUtilities/inc/MVAStruct.hh"
#include <string>
#include <map>
namespace mu2e {
  struct TrkQualDetail {
// enumerate the input varibles used in TrkQual.  The order should match that used in
// the MVA configuration
    enum MVA_varindex { nactive=0, factive, log10fitcon, momerr, t0err, d0, rmax,
      fdouble, fnullambig, fstraws, n_vars};
    typedef std::map<std::string,MVA_varindex> map_type;
    static std::string const& typeName();
    static std::map<std::string,MVA_varindex> const& varNames();

  };
  typedef MVAStruct<TrkQualDetail> TrkQual;
}
#endif
