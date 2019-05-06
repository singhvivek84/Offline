//
// Namespace for collecting tools used in TrkDiag tree filling
// Original author: A. Edmonds (November 2018)
//
#include "TrkDiag/inc/TrkTools.hh"
#include "RecoDataProducts/inc/TrkStrawHitSeed.hh"

#include "TrackerGeom/inc/Tracker.hh"
#include <cmath>

namespace mu2e {
  void TrkTools::fillHitCount(StrawHitFlagCollection const& shfC, HitCount& hitcount) {
    hitcount._nsd = shfC.size();
    for(const auto& shf : shfC) {
      if(shf.hasAllProperties(StrawHitFlag::energysel))++hitcount._nesel;
      if(shf.hasAllProperties(StrawHitFlag::radsel))++hitcount._nrsel;
      if(shf.hasAllProperties(StrawHitFlag::timesel))++hitcount._ntsel;
      if(shf.hasAllProperties(StrawHitFlag::bkg))++hitcount._nbkg;
      if(shf.hasAllProperties(StrawHitFlag::trksel))++hitcount._ntpk;
    }
  }

  void TrkTools::fillHitCount(RecoCount const& nrec, HitCount& hitcount) {
    hitcount._nsd = nrec._nstrawdigi;
    hitcount._ncd = nrec._ncalodigi;
    hitcount._ncc = nrec._ncaloclust;
    hitcount._ncrvd = nrec._ncrvdigi;
    hitcount._nesel = nrec._nshfesel;
    hitcount._nrsel = nrec._nshfrsel;
    hitcount._ntsel = nrec._nshftsel;
    hitcount._nbkg = nrec._nshfbkg;
    hitcount._ntpk = nrec._nshftpk;
  }

  void TrkTools::fillTrkInfo(const KalSeed& kseed,TrkInfo& trkinfo) {
    if(kseed.status().hasAllProperties(TrkFitFlag::kalmanConverged))
      trkinfo._status = 1;
    else if(kseed.status().hasAllProperties(TrkFitFlag::kalmanOK))
      trkinfo._status = 2;
    else
      trkinfo._status = -1;
    if(kseed.status().hasAllProperties(TrkFitFlag::CPRHelix))
      trkinfo._alg = 1;
    else if(kseed.status().hasAllProperties(TrkFitFlag::TPRHelix))
      trkinfo._alg = 0;
    else
      trkinfo._alg = -1;

    trkinfo._pdg = kseed.particle().particleType();
    trkinfo._t0 = kseed.t0().t0();
    trkinfo._t0err = kseed.t0().t0Err();

    fillTrkInfoHits(kseed, trkinfo);

    trkinfo._chisq = kseed.chisquared();
    trkinfo._fitcon = kseed.fitConsistency();
    trkinfo._nbend = kseed.nBend();

    for(std::vector<TrkStrawHitSeed>::const_iterator ihit=kseed.hits().begin(); ihit != kseed.hits().end(); ++ihit) {
      if(ihit->flag().hasAllProperties(StrawHitFlag::active)) {
	trkinfo._firstflt = ihit->trkLen();
	break;
      }
    }
    for(std::vector<TrkStrawHitSeed>::const_reverse_iterator ihit=kseed.hits().rbegin(); ihit != kseed.hits().rend(); ++ihit) {
      if(ihit->flag().hasAllProperties(StrawHitFlag::active)) {
	trkinfo._lastflt = ihit->trkLen();
	break;
      }
    }

    // Loop through the KalSegments
    double firstflt = 9999999;
    double lastflt = -9999999;
    for (const auto& kseg : kseed.segments()) {
      //	std::cout << "AE: min = " << kseg.fmin() << ", max = " << kseg.fmax() << std::endl;
      if (kseg.globalFlt(kseg.fmin()) < firstflt) {
	firstflt = kseg.globalFlt(kseg.fmin());
      }
      if (kseg.globalFlt(kseg.fmax()) > lastflt) {
	lastflt = kseg.globalFlt(kseg.fmax());
      }
    }
    trkinfo._startvalid = firstflt;
    trkinfo._endvalid = lastflt;

    fillTrkInfoStraws(kseed, trkinfo);

    const KalSegment& kseg = *(kseed.segments().begin()); // is this the correct segment to get? TODO
    trkinfo._ent._fitmom = kseg.mom();
    trkinfo._ent._fitmomerr = kseg.momerr();
    trkinfo._ent._fitpar = kseg.helix();
    CLHEP::HepSymMatrix pcov;
    kseg.covar().symMatrix(pcov);
    trkinfo._ent._fitparerr = helixpar(pcov);
  }

  void TrkTools::fillTrkInfoHits(const KalSeed& kseed, TrkInfo& trkinfo) {
    trkinfo._nhits = 0; trkinfo._nactive = 0; trkinfo._ndouble = 0; trkinfo._ndactive = 0; trkinfo._nnullambig = 0;
    static StrawHitFlag active(StrawHitFlag::active);
    for (auto ihit = kseed.hits().begin(); ihit != kseed.hits().end(); ++ihit) {
      ++trkinfo._nhits;
      if (ihit->flag().hasAllProperties(active)) {
	++trkinfo._nactive;
	if (ihit->ambig()==0) {
	  ++trkinfo._nnullambig;
	}
      }
      auto jhit = ihit; jhit++;
      if(jhit != kseed.hits().end() && ihit->strawId().uniquePanel() ==
	 jhit->strawId().uniquePanel()){
	++trkinfo._ndouble;
	if(ihit->flag().hasAllProperties(active)) { ++trkinfo._ndactive; }
      }
    }

    trkinfo._ndof = trkinfo._nactive -5;
    if (kseed.hasCaloCluster()) {
      ++trkinfo._ndof;
    }
  }

  void TrkTools::fillTrkInfoStraws(const KalSeed& kseed, TrkInfo& trkinfo) {
    trkinfo._nmat = 0; trkinfo._nmatactive = 0; trkinfo._radlen = 0.0;
    for (std::vector<TrkStraw>::const_iterator i_straw = kseed.straws().begin(); i_straw != kseed.straws().end(); ++i_straw) {
      ++trkinfo._nmat;
      if (i_straw->active()) {
	++trkinfo._nmatactive;
	trkinfo._radlen += i_straw->radLen();
      }
    }
  }    

  void TrkTools::fillHitInfo(const KalSeed& kseed, std::vector<TrkStrawHitInfo>& tshinfos ) {
    tshinfos.clear();
    // loop over hits

    static StrawHitFlag active(StrawHitFlag::active);
    const Tracker& tracker = *GeomHandle<Tracker>();
    for(std::vector<TrkStrawHitSeed>::const_iterator ihit=kseed.hits().begin(); ihit != kseed.hits().end(); ++ihit) {
      TrkStrawHitInfo tshinfo;
      auto const& straw = tracker.getStraw(ihit->strawId());

      tshinfo._active = ihit->flag().hasAllProperties(active);
      tshinfo._plane = ihit->strawId().plane();
      tshinfo._panel = ihit->strawId().panel();
      tshinfo._layer = ihit->strawId().layer();
      tshinfo._straw = ihit->strawId().straw();

      // find nearest segment
      auto ikseg = kseed.nearestSegment(ihit->trkLen());
      if(ikseg != kseed.segments().end()){
	XYZVec dir;
	ikseg->helix().direction(ikseg->localFlt(ihit->trkLen()),dir);
	auto tdir = Geom::Hep3Vec(dir);
	tshinfo._wdot = tdir.dot(straw.getDirection());
      }
      tshinfo._residerr = ihit->radialErr();
      // note; the following is the BIASED residual FIXME!
      tshinfo._resid = ihit->driftRadius()-ihit->wireDOCA()*ihit->ambig();
      tshinfo._rdrift = ihit->driftRadius();
      tshinfo._rdrifterr = ihit->radialErr();

      double rstraw = straw.getRadius();
      tshinfo._dx = std::sqrt(std::max(0.0,rstraw*rstraw-tshinfo._rdrift*tshinfo._rdrift));
	
      tshinfo._trklen = ihit->trkLen();
      tshinfo._hlen = ihit->hitLen();
      tshinfo._t0 = ihit->t0().t0();
      tshinfo._t0err = ihit->t0().t0Err();
      tshinfo._ht = ihit->hitTime();
      tshinfo._ambig = ihit->ambig();
      tshinfo._doca = ihit->wireDOCA();
      tshinfo._edep = ihit->energyDep();
      tshinfo._wdist = ihit->wireDist();
      tshinfo._werr = ihit->wireRes();	
      tshinfo._driftend = ihit->driftEnd();
      tshinfo._tdrift = ihit->hitTime() - ihit->signalTime() - ihit->t0().t0();
      auto const& wiredir = straw.getDirection();
      auto const& mid = straw.getMidPoint();
      CLHEP::Hep3Vector hpos = mid + wiredir*ihit->hitLen();
      tshinfo._poca = Geom::toXYZVec(hpos);

      // count correlations with other TSH
      for(std::vector<TrkStrawHitSeed>::const_iterator jhit=kseed.hits().begin(); jhit != ihit; ++jhit) {
	if(tshinfo._plane ==  jhit->strawId().plane() &&
	   tshinfo._panel == jhit->strawId().panel() ){
	  tshinfo._dhit = true;
	  if (jhit->flag().hasAllProperties(active)) {
	    tshinfo._dactive = true;
	    break;
	  }
	}
      }

      tshinfos.push_back(tshinfo);
    }
  }

  void TrkTools::fillMatInfo(const KalSeed& kseed, std::vector<TrkStrawMatInfo>& tminfos ) {
    tminfos.clear();
    // loop over sites, pick out the materials
      
    for(const auto& i_straw : kseed.straws()) {
      TrkStrawMatInfo tminfo;

      tminfo._plane = i_straw.straw().getPlane();
      tminfo._panel = i_straw.straw().getPanel();
      tminfo._layer = i_straw.straw().getLayer();
      tminfo._straw = i_straw.straw().getStraw();

      tminfo._active = i_straw.active();
      tminfo._dp = i_straw.pfrac();
      tminfo._radlen = i_straw.radLen();
      tminfo._doca = i_straw.doca();
      tminfo._tlen = i_straw.trkLen();

      tminfos.push_back(tminfo);
    }
  }

  void TrkTools::fillCaloHitInfo(const KalSeed& kseed, TrkCaloHitInfo& tchinfo) {
    if (kseed.hasCaloCluster()) {
      auto const& tch = kseed.caloHit();
      auto const& cc = tch.caloCluster();

      tchinfo._active = tch.flag().hasAllProperties(StrawHitFlag::active);
      tchinfo._did = cc->diskId();
      tchinfo._trklen = tch.trkLen();
      tchinfo._clen = tch.hitLen();
    
      if(tch.flag().hasAllProperties(StrawHitFlag::doca)) {
	tchinfo._doca = tch.clusterAxisDOCA();
      }
      else {
	tchinfo._doca = -100.0;
      }
      // add the propagation time offsetA
      tchinfo._t0 = tch.t0().t0();
      tchinfo._t0err = tch.t0().t0Err();
      tchinfo._ct = tch.time(); // time used to constrain T0 by this hit: includes the 'propagation time' offset
      tchinfo._cterr = tch.timeErr();
      tchinfo._edep = cc->energyDep();
      // transform cog to tracker coordinates; requires 2 steps.  This is at the front
      // of the disk
      mu2e::GeomHandle<mu2e::Calorimeter> calo;
      XYZVec cpos = Geom::toXYZVec(calo->geomUtil().mu2eToTracker(calo->geomUtil().diskToMu2e(cc->diskId(),cc->cog3Vector())));
      // move to the front face and 
      // add the cluster length (relative to the front face).  crystal size should come from geom FIXME!
      cpos.SetZ(cpos.z() -200.0 + tch.hitLen());
      tchinfo._poca = cpos;
      // find the nearest segment
      auto ikseg = kseed.nearestSegment(tch.trkLen());
      if(ikseg != kseed.segments().end()){
	ikseg->mom(ikseg->localFlt(tch.trkLen()),tchinfo._mom);
      }
    }
  }

  void TrkTools::fillTrkQualInfo(const TrkQual& tqual, TrkQualInfo& trkqualInfo) {
    int n_trkqual_vars = TrkQual::n_vars;
    for (int i_trkqual_var = 0; i_trkqual_var < n_trkqual_vars; ++i_trkqual_var) {
      TrkQual::MVA_varindex i_index = TrkQual::MVA_varindex(i_trkqual_var);
      trkqualInfo._trkqualvars[i_trkqual_var] = (double) tqual[i_index];
    }
    trkqualInfo._trkqual = tqual.MVAOutput();
  }

  void TrkTools::fillHelixInfo(const KalSeed& kseed, HelixInfo& hinfo) {
    // navigate down to the HelixSeed
    auto hhh = kseed.helix();
    if(hhh.isNull() && kseed.kalSeed().isNonnull())
      hhh = kseed.kalSeed()->helix();
    if(hhh.isNonnull()){
      // count hits, active and not
      for(size_t ihit=0;ihit < hhh->hits().size(); ihit++){
	auto const& hh = hhh->hits()[ihit];
	hinfo._nch++;
	hinfo._nsh += hh.nStrawHits();
	if(!hh.flag().hasAnyProperty(StrawHitFlag::outlier)){
	  hinfo._ncha++;
	  hinfo._nsha += hh.nStrawHits();
	}
	if( hhh->status().hasAllProperties(TrkFitFlag::TPRHelix))
	  hinfo._flag = 1;
	else if( hhh->status().hasAllProperties(TrkFitFlag::CPRHelix))
	  hinfo._flag = 2;
	hinfo._t0err = hhh->t0().t0Err();
	hinfo._mom = 0.299792*hhh->helix().momentum()*_bz0; //FIXME!
	hinfo._chi2xy = hhh->helix().chi2dXY();
	hinfo._chi2fz = hhh->helix().chi2dZPhi();
	if(hhh->caloCluster().isNonnull())
	  hinfo._ecalo  = hhh->caloCluster()->energyDep();
      }
    }
  }
}
