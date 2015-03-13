//
// $Id: TrkExtrapolBis_module.cc,v 1.20 2014/09/20 18:04:22 murat Exp $
// $Author: murat $
// $Date: 2014/09/20 18:04:22 $
//
// Original author B. Echenard
//
// A generic algorithm would either scan the track along every section in sequence or check every section along the path 
// First option is optimal when sections are separated in z (e.g disks), second is ideal when sections are ovelapping in z (e.g. vanes)
// Since it is very likely we have disks, the first choice has been selected (implementing the second should be straightforward)

// a few notes

// Currently, we must extend the tracks to the calorimeter, but D. Brown said he would do that by default, so remove the corresponding 
// lines when this is ready

// There are a bunch of methods only available for HelixTraj, but not for TrajDifTraj. D. Brown will add them later
// and this code should be updated when they are available

// There are some optimizations for the disk that can be set by defaults when we get rid of the vanes

 
// Framework includes.
#include "art/Framework/Core/EDProducer.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Selector.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Services/Optional/TFileDirectory.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"
#include "art/Framework/Services/Optional/TFileService.h"
#include "art/Framework/Principal/Handle.h"

// From the art tool-chain
#include "fhiclcpp/ParameterSet.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

// Mu2e includes.
#include "GeometryService/inc/GeometryService.hh"
#include "GeometryService/inc/GeomHandle.hh"


//CLHEP includes
#include "CLHEP/Vector/TwoVector.h"
#include "CLHEP/Geometry/HepPoint.h"


// calorimeter, tracker  and data
#include "BaBar/Constants.hh"
#include "CalorimeterGeom/inc/Calorimeter.hh"
#include "CalorimeterGeom/inc/DiskCalorimeter.hh"
#include "KalmanTests/inc/KalRepPtrCollection.hh"
#include "KalmanTrack/KalRep.hh"
#include "KalmanTests/inc/KalFitMC.hh"
#include "TrkBase/HelixParams.hh"
#include "TrkBase/TrkRep.hh"
#include "RecoDataProducts/inc/TrkCaloIntersectCollection.hh"



// Other includes.
#include "cetlib/exception.h"
#include <cmath>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>


//root includes
#include "TFile.h"
#include "TDirectory.h"
#include "TNtuple.h"



  
namespace {

	struct TrkCaloInter 
	{
	   int    fSection;
	   double fSEntr;
	   double fSEntrErr;
	   double fSExit;
	};
}



namespace mu2e {


    class TrkExtrapolBis : public art::EDProducer {


       public:

	   explicit TrkExtrapolBis(fhicl::ParameterSet const& pset):
	     _trkterModuleLabel(pset.get<std::string>("fitterModuleLabel")),
	     _tpart((TrkParticle::type)(pset.get<int>("fitparticle"))),
	     _fdir((TrkFitDirection::FitDirection)(pset.get<int>("fitdirection"))),
	     _diagLevel(pset.get<int>("diagLevel",0)),
	     _pathStep(pset.get<double>("pathStep")),
	     _tolerance(pset.get<double>("tolerance")),
	     _checkExit(pset.get<bool>("checkExit")),
	     _outputNtup(pset.get<bool>("outputNtup")),
	     _trkdiag(0)
	   {

        	 _trkfitInstanceName  = _fdir.name() + _tpart.name();
	         _downstream          = (_fdir.dzdt() > 0 ) ? true : false;
		 
        	 produces<TrkCaloIntersectCollection>();
		 
	   }


	   virtual ~TrkExtrapolBis() {}

	   void beginJob();
	   void endJob() {}
	   void produce(art::Event & e );





       private:

	   void fillTrkNtup(int itrk, KalRep const* kalrep,  TrkDifTraj const& traj, std::vector<TrkCaloInter> const& intersec);
	   void doExtrapolation(TrkCaloIntersectCollection& extrapolatedTracks, KalRepPtrCollection const& trksPtrColl);
	   void findIntersectSection(Calorimeter const& cal, TrkDifTraj const& traj, 
	                             HelixTraj const& trkHel, unsigned int iSection, std::vector<TrkCaloInter>& intersect);
	   
           double scanIn(      Calorimeter const& cal, TrkDifTraj const& traj, HelixTraj const& trkHel, int iSection, double rangeStart, double rangeEnd);
           double scanOut(     Calorimeter const& cal, TrkDifTraj const& traj, HelixTraj const& trkHel, int iSection, double rangeStart, double rangeEnd);
           double scanOutDisk( Calorimeter const& cal, TrkDifTraj const& traj, HelixTraj const& trkHel, int iSection, double rangeStart, double rangeEnd);
           double scanBinary(  Calorimeter const& cal, TrkDifTraj const& traj, int iSection, double rangeIn, double rangeOut);

           double fastForwardDisk(Calorimeter const& cal, TrkDifTraj const& traj, HelixTraj const& trkHel, double rangeInit, CLHEP::Hep3Vector& trjVec);
           double extendToRadius(HelixTraj const& trkHel, TrkDifTraj const& traj, double range, double cylinderRad);
           void   updateTrjVec(Calorimeter const& cal, TrkDifTraj const& traj, double range, CLHEP::Hep3Vector& trjVec);
	   double radiusAtRange(TrkDifTraj const& traj, double range);
	   

	   std::string                   _trkterModuleLabel;
	   TrkParticle                   _tpart;
	   TrkFitDirection               _fdir;
	   std::string                   _trkfitInstanceName;
	   bool                          _downstream;
	   int                           _diagLevel;
	   double                        _pathStep;
	   double                        _tolerance;
	   bool                          _checkExit;
	   bool                          _outputNtup;



	   TTree* _trkdiag;
	   int    _trkid,_trkint;
	   float  _trksection[128],_trkpath[128],_trktof[128],_trkx[128],_trky[128],_trkz[128];
	   float  _trkmomx[128],_trkmomy[128],_trkmomz[128],_trkmom[128];
	   


    };






    void TrkExtrapolBis::beginJob() 
    {
	
	if (_outputNtup)
	{
	    art::ServiceHandle<art::TFileService> tfs;
	    _trkdiag  = tfs->make<TTree>("trk", "trk extrapolated info");
	    _trkdiag->Branch("trkid", &_trkid  ,"trkid/I");
	    _trkdiag->Branch("trkint", &_trkint  ,"trkint/I");
	    _trkdiag->Branch("trksection[trkint]", _trksection, "trksection[trkint]/F");
	    _trkdiag->Branch("trkpath[trkint]", _trkpath, "trkpath[trkint]/F");
	    _trkdiag->Branch("trktof[trkint]", _trktof, "trktof[trkint]/F");
	    _trkdiag->Branch("trkx[trkint]", _trkx, "trkx[trkint]/F");
	    _trkdiag->Branch("trky[trkint]", _trky, "trky[trkint]/F");
	    _trkdiag->Branch("trkz[trkint]", _trkz, "trkz[trkint]/F");
	    _trkdiag->Branch("trkmomx[trkint]", _trkmomx, "trkmomx[trkint]/F");
	    _trkdiag->Branch("trkmomy[trkint]", _trkmomy, "trkmomy[trkint]/F");
	    _trkdiag->Branch("trkmomz[trkint]", _trkmomz, "trkmomz[trkint]/F");
	    _trkdiag->Branch("trkmom[trkint]", _trkmom, "trkmom[trkint]/F");
	}    
    }

    //-----------------------------------------------------------------------------
    void TrkExtrapolBis::fillTrkNtup(int itrk, KalRep const* kalrep,  TrkDifTraj const& traj, std::vector<TrkCaloInter> const& intersec)
    {
	_trkid = itrk;
	_trkint = intersec.size();    
	
	for(unsigned int i=0; i<intersec.size(); ++i)
	{
	    double length  = intersec[i].fSEntr;
	    _trksection[i] = intersec[i].fSection;
	    _trkpath[i]    = length;
	    _trktof[i]     =  kalrep->arrivalTime(length);
	    _trkx[i]       = traj.position(length).x();
	    _trky[i]       = traj.position(length).y();
	    _trkz[i]       = traj.position(length).z();
	    _trkmomx[i]    = kalrep->momentum(length).x();
	    _trkmomy[i]    = kalrep->momentum(length).y();
	    _trkmomz[i]    = kalrep->momentum(length).z();
	    _trkmom[i]     = kalrep->momentum(length).mag();      
	}
	_trkdiag->Fill();
    }






    //-----------------------------------------------------------------------------
    void TrkExtrapolBis::produce(art::Event & evt )
    {

	//get tracks
	art::Handle<KalRepPtrCollection> trksHandle;
	evt.getByLabel(_trkterModuleLabel,_trkfitInstanceName,trksHandle);
	KalRepPtrCollection const& trksPtrColl = *trksHandle.product();

	//output of Extrapolated tracks
	std::unique_ptr<TrkCaloIntersectCollection> extrapolatedTracks(new TrkCaloIntersectCollection);

	if (_diagLevel) std::cout<<"Event Number : "<< evt.event()<<"\nStart TrkExtrapolBis  with ntrk = "<<trksPtrColl.size()<<std::endl;

	doExtrapolation(*extrapolatedTracks, trksPtrColl);   
	evt.put(std::move(extrapolatedTracks));

    } 


    //-----------------------------------------------------------------------------
    void TrkExtrapolBis::doExtrapolation(TrkCaloIntersectCollection& extrapolatedTracks, KalRepPtrCollection const& trksPtrColl)
    {
	 Calorimeter const&  cal = *(GeomHandle<Calorimeter>());
	 CLHEP::Hep3Vector   endCalTracker = cal.toTrackerFrame( CLHEP::Hep3Vector(cal.origin().x(),cal.origin().y(),cal.caloGeomInfo().enveloppeZ1()) );
	 
	 
	 for (unsigned int itrk=0; itrk< trksPtrColl.size(); ++itrk )
	 {
	      std::vector<TrkCaloInter> intersectVec;

	      KalRepPtr trk  = trksPtrColl.at(itrk);
	      KalRep*   krep = const_cast<KalRep*>( &(*trk) ); //exception: cast away const to extend track - ok with D.LBL Brown
	      if ( !krep ) continue;

	      HelixTraj trkHel(krep->helix(krep->endFoundRange()).params(),krep->helix(krep->endFoundRange()).covariance());
	      double endrange = trkHel.zFlight(endCalTracker.z()); 

	      TrkErrCode rc = krep->extendThrough(endrange);
	      if (rc.success() != 1 && rc.success() !=13)
	      {
        	if (_diagLevel) std::cout<<"TrackExtrapol ERROR: could not extend to range = "<<endrange<<", rc = "<<rc.success()<<"  for zend="<<endCalTracker.z()<<std::endl;
        	continue;
	      }

	      if (_diagLevel>2)
	      {
		  double angle         = Constants::pi*0.5 + trkHel.phi0();
		  double circleRadius  = 1.0/trkHel.omega();
		  double centerCircleX = (trkHel.d0() + circleRadius)*cos(angle);
		  double centerCircleY = (trkHel.d0() + circleRadius)*sin(angle);
		  printf("Helix traj circle: R = %10.3f X0 = %10.3f  Y0 = %10.3f phi0 = %10.3f d0 = %10.3f\n",circleRadius,centerCircleX,centerCircleY,angle,trkHel.d0());		  
	      }

	      TrkDifTraj const& traj = krep->traj();
              
	      for(unsigned int iSec=0; iSec<cal.nSection(); ++iSec ) findIntersectSection(cal,traj,trkHel,iSec, intersectVec);  
	      if (_downstream) std::sort(intersectVec.begin(),intersectVec.end(),[](const TrkCaloInter& a, const TrkCaloInter& b ){ return a.fSEntr < b.fSEntr;});
	      else             std::sort(intersectVec.begin(),intersectVec.end(),[](const TrkCaloInter& a, const TrkCaloInter& b ){ return a.fSEntr > b.fSEntr;});
	      


	      for (auto inter : intersectVec ) extrapolatedTracks.push_back( TrkCaloIntersect(inter.fSection, trk, itrk, inter.fSEntr,inter.fSEntrErr, inter.fSExit) );
	      
	      if (_diagLevel) std::cout<<"Found "<<intersectVec.size()<<" intersections "<<std::endl;
	      if (_diagLevel) for (auto inter : intersectVec ) std::cout<<"Final "<<inter.fSection<<" "<<inter.fSEntr<<"  "<<inter.fSExit<<std::endl;
	      

	      if (_outputNtup) fillTrkNtup(itrk, krep, traj, intersectVec);
	 }

     }



    //-----------------------------------------------------------------------------
    // Find entry / exit points with a pseudo binary search
    void TrkExtrapolBis::findIntersectSection(Calorimeter const& cal, TrkDifTraj const& traj,
                                           HelixTraj const& trkHel, unsigned int iSection, std::vector<TrkCaloInter>& intersect)
    {
	 	 	 
	 double zStart       = (_downstream) ? cal.section(iSection).zDownInTracker() : cal.section(iSection).zUpInTracker();
	 double zEnd         = (_downstream) ? cal.section(iSection).zUpInTracker()   : cal.section(iSection).zDownInTracker() ;	 	 
	 double rangeStart   = trkHel.zFlight(zStart);
	 double rangeEnd     = trkHel.zFlight(zEnd);
         

	 //apply first order correction to the helix model to match the trajDif model, add buffer to endRange to be on the safe side
	 //D. Brown will add a zFlight() method for TrajDifTraj in the future, rewrite code when available	 
	 double sinDip         = sqrt(1.0-trkHel.cosDip()*trkHel.cosDip());
	 double rangeStartCorr = (zStart-traj.position(rangeStart).z())/sinDip;
         double rangeEndCorr   = (zEnd-traj.position(rangeEnd).z())/sinDip +2;
	 	 
	 if (_downstream) {rangeStart += rangeStartCorr;rangeEnd += rangeEndCorr;} 
	 else             {rangeStart -= rangeStartCorr;rangeEnd -= rangeEndCorr;} 


         if (_diagLevel>1) std::cout<<"TrkExtrapolBis inter   rangeStart = "<<rangeStart<<"   rangeEnd="<<rangeEnd
	                            <<"  from z in Tracker="<<zStart<<" - "<<zEnd
				    <<"  Start at position "<< traj.position(rangeStart)<<std::endl;


	 double rangeIn = scanIn(cal,traj,trkHel,iSection,rangeStart, rangeEnd);

	 if (rangeIn > rangeEnd)
	 {
	     if (_diagLevel>1) std::cout<<"TrkExtrapolBis end search behind Section "<<iSection<<",range= "<<rangeIn<<", position is : "<<traj.position(rangeIn)<<std::endl;
	     return;
	 }  
	 

	 double rangeOut (-1);
         if (_checkExit)
	 {
	    if (cal.caloType()==Calorimeter::CaloType::disk) rangeOut = scanOutDisk(cal, traj, trkHel, iSection, rangeIn+1, rangeEnd);
	    else                                             rangeOut = scanOut(    cal, traj, trkHel, iSection, rangeIn+1, rangeEnd);
	 }  

	 
	 TrkCaloInter inter;
	 inter.fSection  = iSection;
	 inter.fSEntr    = rangeIn;
	 inter.fSEntrErr = _tolerance;
	 inter.fSExit    = rangeOut;
	 intersect.push_back(inter); 

    }


    //-----------------------------------------------------------------------------
    // find two starting points inside and outside of the calorimeter, either move out if we're in, or move in if we're out
    // if we are outside the calorimeter enveloppe, fast forward to the enveloppe
    double TrkExtrapolBis::scanIn(Calorimeter const& cal, TrkDifTraj const& traj, HelixTraj const& trkHel, int iSection, double rangeStart, double rangeEnd)
    {         

	 //check if we're inside the rdial enveloppe, if not, fast forward to it
	 double initRadius    = radiusAtRange(traj,rangeStart);
	 if (initRadius < cal.section(iSection).rInTracker())  rangeStart += extendToRadius(trkHel, traj, rangeStart, cal.section(iSection).rInTracker());	 
	 if (initRadius > cal.section(iSection).rOutTracker()) rangeStart += extendToRadius(trkHel, traj, rangeStart, cal.section(iSection).rOutTracker()); 

	 if (rangeStart > rangeEnd) return rangeEnd+1e-4;


	 //now find two starting points, one inside the boundary, the other outside
	 CLHEP::Hep3Vector trjVec;
	 updateTrjVec(cal,traj,rangeStart,trjVec);
	 
	 double rangeIn(rangeStart),rangeOut(rangeStart);
	 if ( cal.isInsideSection(iSection,trjVec))
	 {
	      while (cal.isInsideSection(iSection,trjVec))
	      {
		 rangeIn = rangeOut; 
		 rangeOut -= _pathStep;
		 updateTrjVec(cal,traj,rangeOut,trjVec);
		 if (_diagLevel>2) std::cout<<"TrackExtrpol position scan In down "<<trjVec<<"  for currentRange="<<rangeOut<<"   "<<"radius="<<radiusAtRange(traj,rangeOut)<<std::endl;
	      }	 

	 } else {
	 
	      while ( !cal.isInsideSection(iSection,trjVec) )
	      {
		 rangeOut = rangeIn; 
		 rangeIn += _pathStep;
        	 updateTrjVec(cal,traj,rangeIn,trjVec);
		 if (_diagLevel>2) std::cout<<"TrackExtrpol position scan In up "<<trjVec<<"  for currentRange="<<rangeIn<<"   "<<"radius="<<radiusAtRange(traj,rangeIn)<<std::endl;

		 if ( rangeIn > rangeEnd) return rangeIn;
	      }	    
	 }
	 
	 //finally, run the binary search for the intersection
	 return scanBinary(cal, traj, iSection, rangeIn, rangeOut);
    }



    //-----------------------------------------------------------------------------
    // generic algorithm: step along the track (coarse search) until you reach the outside of the calo section, then refine with binary search
    double TrkExtrapolBis::scanOut(Calorimeter const& cal, TrkDifTraj const& traj, HelixTraj const& trkHel, int iSection, double rangeStart, double rangeEnd)
    {         

	 double range(rangeStart);
	 CLHEP::Hep3Vector trjVec;
	 updateTrjVec(cal,traj,range,trjVec);

         while ( cal.isInsideSection(iSection,trjVec) )
	 {         	    
	    range += _pathStep;
	    updateTrjVec(cal,traj,range,trjVec);
	    if (_diagLevel>2) std::cout<<"TrackExtrpol position scan Out up "<<trjVec<<"  for currentRange="<<range<<"   "<<"radius="<<radiusAtRange(traj,range)<<std::endl;	 	 
	 }

	 return scanBinary(cal, traj, iSection, range - _pathStep, range);
    }
    
    //-----------------------------------------------------------------------------
    //same as ScanOut, but with an optimization for the disk geometry, forwarding close to the limits of the calorimeter enveloppe if we're inside the calorimeter volume
    double TrkExtrapolBis::scanOutDisk(Calorimeter const& cal, TrkDifTraj const& traj, HelixTraj const& trkHel, int iSection, double rangeStart, double rangeEnd)
    {         

	 double rangeForward(0);
	 double caloRadiusIn  = cal.section(iSection).rInTracker()  + 3*cal.caloGeomInfo().crystalHalfTrans();
	 double caloRadiusOut = cal.section(iSection).rOutTracker() - 3*cal.caloGeomInfo().crystalHalfTrans();

	 double range(rangeStart);

	 CLHEP::Hep3Vector trjVec;
	 updateTrjVec(cal,traj,range,trjVec);

         while ( cal.isInsideSection(iSection,trjVec) )
	 {         	    

	      double radius = radiusAtRange(traj,range);
	      if (radius > caloRadiusIn &&  radius < caloRadiusOut && fabs(range-rangeForward) > 10*_pathStep )
	      {
		  double lenInner = extendToRadius(trkHel, traj, range, caloRadiusIn );
		  double lenOuter = extendToRadius(trkHel, traj, range, caloRadiusOut );
		  double deltaLen = std::min(lenInner,lenOuter);

		  //the mismatch between traj and trkHelk causes forwarding to remain at the same point in some instances
		  if (deltaLen > 0 )
		  {
		      range += deltaLen;
		      if (range > rangeEnd) range = rangeEnd - _pathStep;
		      rangeForward = range;
		      if (_diagLevel>2) std::cout<<"Scan out fast forwarded to range="<<range<<"   "<<lenInner<<" "<<lenOuter<<std::endl;

		      updateTrjVec(cal,traj,range,trjVec);
		      while( !cal.isInsideSection(iSection,trjVec) ) {range -= _pathStep; updateTrjVec(cal,traj,range,trjVec);}	
		  }      
	      }
	      

	    range += _pathStep;
	    updateTrjVec(cal,traj,range,trjVec);
	    if (_diagLevel>2) std::cout<<"TrackExtrpol position scan Out up "<<trjVec<<"  for currentRange="<<range<<"   "<<"radius="<<radiusAtRange(traj,range)<<std::endl;	 	 
	 }
         

	 return scanBinary(cal, traj, iSection, range - _pathStep, range);
    }


    //-----------------------------------------------------------------------------
    double TrkExtrapolBis::scanBinary(Calorimeter const& cal, TrkDifTraj const& traj, int iSection, double rangeIn, double rangeOut)
    {         
         
	 CLHEP::Hep3Vector trjVec;
         
	 double range = 0.5*(rangeOut+rangeIn);
	 while ( fabs(rangeIn-rangeOut) > _tolerance)
	 {
	     updateTrjVec(cal,traj,range,trjVec);
	     if (_diagLevel>2) std::cout<<"TrackExtrpol position scan Binary "<<trjVec<<"  for currentRange="<<range<<std::endl;
	     
	     if (cal.isInsideSection(iSection,trjVec)) rangeIn=range;
	     else                                      rangeOut=range;
	     range = 0.5*(rangeOut+rangeIn);	     
	 }
	 
         return range;
    }


 




    //-----------------------------------------------------------------------------
    void TrkExtrapolBis::updateTrjVec(Calorimeter const& cal, TrkDifTraj const& traj, double range, CLHEP::Hep3Vector& trjVec)
    {
       HepPoint trjPoint = traj.position(range);
       trjVec.set(trjPoint.x(),trjPoint.y(),trjPoint.z());
       trjVec = cal.fromTrackerFrame(trjVec);    
    }


    //-----------------------------------------------------------------------------
    double TrkExtrapolBis::radiusAtRange(TrkDifTraj const& traj, double range)
    {
       HepPoint trjPoint = traj.position(range);
       return sqrt(trjPoint.x()*trjPoint.x()+trjPoint.y()*trjPoint.y());
    }

    
    //-----------------------------------------------------------------------------
    // see http://paulbourke.net/geometry/circlesphere  for notation
    double TrkExtrapolBis::extendToRadius(HelixTraj const& trkHel, TrkDifTraj const& traj, double range, double cylinderRad)
    {         

	 double radius    = 1.0/trkHel.omega();
	 double centerX   = -(trkHel.d0() + radius)*sin(trkHel.phi0());
	 double centerY   =  (trkHel.d0() + radius)*cos(trkHel.phi0());
	 double d         = fabs(trkHel.d0() + radius);
	 double a         = (cylinderRad*cylinderRad-radius*radius+d*d)/2.0/d;

	 //if a <cylinderRad, track never crosses cylinder, return overflow value
	 if (a>cylinderRad) return 1e6;

	 double h       = sqrt(cylinderRad*cylinderRad-a*a);	     
	 double xi1     = (a*centerX+h*centerY)/d;
	 double yi1     = (a*centerY-h*centerX)/d;
	 double xi2     = (a*centerX-h*centerY)/d;
	 double yi2     = (a*centerY+h*centerX)/d;

	 HepPoint trjPoint = traj.position(range); 
	 double phi_i      = (trjPoint.y()-centerY>0) ? atan2(trjPoint.y()-centerY,trjPoint.x()-centerX) : atan2(trjPoint.y()-centerY,trjPoint.x()-centerX) + 2*Constants::pi;
	 double phi_s1     = (yi1-centerY>0) ? atan2(yi1-centerY,xi1-centerX) : atan2(yi1-centerY,xi1-centerX) + 2*Constants::pi;
	 double phi_s2     = (yi2-centerY>0) ? atan2(yi2-centerY,xi2-centerX) : atan2(yi2-centerY,xi2-centerX) + 2*Constants::pi;

	 double dphi1      = (phi_s1 > phi_i) ? phi_s1-phi_i : phi_s1-phi_i + 2*Constants::pi;
	 double dphi2      = (phi_s2 > phi_i) ? phi_s2-phi_i : phi_s2-phi_i + 2*Constants::pi;

	 //for downstream tracks (need to check how upstream works)
	 double dphi       = (radius>0) ? std::min(dphi1,dphi2) : 2*Constants::pi - std::max(dphi1,dphi2);
	 double dlen       = fabs(radius)*dphi/trkHel.cosDip();

	 //calculate first order correction between the helix representation and the actual trajectory
	 double corr = (radiusAtRange(traj,range+dlen)-cylinderRad) / (radiusAtRange(traj,range+dlen) - radiusAtRange(traj,range+dlen+1) );
	 dlen += corr;

	 if (dlen < 0) dlen=0;

	 return dlen;
    }




}

using mu2e::TrkExtrapolBis;
DEFINE_ART_MODULE(TrkExtrapolBis);

