//
// A module to extract leading edge times, pulse heights, integrals and number of photons from the CRV waveforms
//
// $Id: $
// $Author: ehrlich $
// $Date: 2014/08/07 01:33:40 $
// 
// Original Author: Ralf Ehrlich

#include "CRVResponse/inc/MakeCrvRecoPulses.hh"

#include "CosmicRayShieldGeom/inc/CosmicRayShield.hh"
#include "DataProducts/inc/CRSScintillatorBarIndex.hh"

#include "ConditionsService/inc/AcceleratorParams.hh"
#include "ConditionsService/inc/ConditionsHandle.hh"
#include "GeometryService/inc/DetectorSystem.hh"
#include "GeometryService/inc/GeomHandle.hh"
#include "GeometryService/inc/GeometryService.hh"
#include "MCDataProducts/inc/CrvWaveformsCollection.hh"
#include "RecoDataProducts/inc/CrvRecoPulsesCollection.hh"

#include "art/Framework/Services/Optional/TFileDirectory.h"
#include "art/Framework/Services/Optional/TFileService.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"
#include <TH1D.h>

#include "canvas/Persistency/Common/Ptr.h"
#include "art/Framework/Core/EDProducer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "fhiclcpp/ParameterSet.h"
#include "CLHEP/Units/GlobalSystemOfUnits.h"

#include <string>

#include <TMath.h>

namespace mu2e 
{
  class CrvRecoPulsesFinder : public art::EDProducer 
  {

    public:
    explicit CrvRecoPulsesFinder(fhicl::ParameterSet const& pset);
    void produce(art::Event& e);
    void beginJob();
    void beginRun(art::Run &run);
    void endJob();

    private:
    boost::shared_ptr<mu2eCrv::MakeCrvRecoPulses> _makeCrvRecoPulses;

    std::string _crvWaveformsModuleLabel;
    double      _scale, _offset;
    bool        _useFittedPulseHeight, _useFittedPulseTime, _doLEfit;
    int         _minPEs;
    double      _microBunchPeriod;

//    TH1F        *_hRecoPulses;
  };

  CrvRecoPulsesFinder::CrvRecoPulsesFinder(fhicl::ParameterSet const& pset) :
    _crvWaveformsModuleLabel(pset.get<std::string>("crvWaveformsModuleLabel")),
    _scale(pset.get<double>("scale")),   //0.0056 V/PE
    _offset(pset.get<double>("offset")),   //0 V
    _useFittedPulseHeight(pset.get<bool>("useFittedPulseHeight")),   //false, since the test beam analysis didn't use it either
    _useFittedPulseTime(pset.get<bool>("useFittedPulseTime")),   //true
    _doLEfit(pset.get<bool>("doLEfit")),   //true
    _minPEs(pset.get<int>("minPEs"))     //6 PEs
  {
    produces<CrvRecoPulsesCollection>();
    _makeCrvRecoPulses = boost::shared_ptr<mu2eCrv::MakeCrvRecoPulses>(new mu2eCrv::MakeCrvRecoPulses(_scale, _offset, _useFittedPulseHeight, _useFittedPulseTime, _doLEfit));
  }

  void CrvRecoPulsesFinder::beginJob()
  {
  }

  void CrvRecoPulsesFinder::endJob()
  {
  }

  void CrvRecoPulsesFinder::beginRun(art::Run &run)
  {
    mu2e::ConditionsHandle<mu2e::AcceleratorParams> accPar("ignored");
    _microBunchPeriod = accPar->deBuncherPeriod;
  }

  void CrvRecoPulsesFinder::produce(art::Event& event) 
  {
    std::unique_ptr<CrvRecoPulsesCollection> crvRecoPulsesCollection(new CrvRecoPulsesCollection);

    art::Handle<CrvWaveformsCollection> crvWaveformsCollection;
    event.getByLabel(_crvWaveformsModuleLabel,"",crvWaveformsCollection);

    for(CrvWaveformsCollection::const_iterator iter=crvWaveformsCollection->begin(); 
        iter!=crvWaveformsCollection->end(); iter++)
    {
      const CRSScintillatorBarIndex &barIndex = iter->first;
      const CrvWaveforms &crvWaveforms = iter->second;
      double digitizationPrecision = crvWaveforms.GetDigitizationPrecision(); //ns

      CrvRecoPulses &crvRecoPulses = (*crvRecoPulsesCollection)[barIndex];
      for(int SiPM=0; SiPM<4; SiPM++)
      {
        //merge single waveforms together if there is no time gap between them
        const std::vector<CrvWaveforms::CrvSingleWaveform> &singleWaveforms = crvWaveforms.GetSingleWaveforms(SiPM);
        std::vector<std::vector<double> > allVoltages;
        std::vector<double> allStartTimes;
        for(size_t i=0; i<singleWaveforms.size(); i++)
        {
          bool appendWaveform=false;
          if(!allVoltages.empty())
          {
            //difference between the time of the last digitization point and the next start time
            double lastTime = allStartTimes.back()+(allVoltages.back().size()-1)*digitizationPrecision;
            double timeDiff = singleWaveforms[i]._startTime - lastTime;
            if(timeDiff<digitizationPrecision*1.1) appendWaveform=true;   //the next start time seems to be just 
                                                                          //one digitization point (12.5ns) away
                                                                          //so that one can assume that the following 
                                                                          //single waveform is just a continuation
                                                                          //and can be appended.
                                                                          //the factor of 1.1 takes the limited precision 
                                                                          //of the floating point numbers into consideration.
          }

          if(appendWaveform) allVoltages.back().insert(allVoltages.back().end(),
                                                       singleWaveforms[i]._voltages.begin(),
                                                       singleWaveforms[i]._voltages.end());
          else
          {
            allVoltages.push_back(singleWaveforms[i]._voltages);
            allStartTimes.push_back(singleWaveforms[i]._startTime); 
          }
        }

        for(size_t i=0; i<allVoltages.size(); i++)
        {
          _makeCrvRecoPulses->SetWaveform(allVoltages[i], allStartTimes[i], digitizationPrecision);

          unsigned int n = _makeCrvRecoPulses->GetNPulses();
          for(unsigned int i=0; i<n; i++)
          {
            double pulseTime   = _makeCrvRecoPulses->GetPulseTime(i);
            int    PEs         = _makeCrvRecoPulses->GetPEs(i);
            double pulseHeight = _makeCrvRecoPulses->GetPulseHeight(i); 
            double pulseWidth  = _makeCrvRecoPulses->GetPulseWidth(i);
            double pulseFitChi2= _makeCrvRecoPulses->GetPulseFitChi2(i);
            double LEtime      = _makeCrvRecoPulses->GetLEtime(i);
            double LEfitChi2   = _makeCrvRecoPulses->GetLEfitChi2(i);
            if(pulseTime<0) continue;
            if(pulseTime>_microBunchPeriod) continue;
            if(PEs<_minPEs) continue; 
            crvRecoPulses.GetRecoPulses(SiPM).emplace_back(PEs, pulseTime, pulseHeight, pulseWidth, pulseFitChi2, LEtime, LEfitChi2);
          }
        }

      }
    }

    event.put(std::move(crvRecoPulsesCollection));
  } // end produce

} // end namespace mu2e

using mu2e::CrvRecoPulsesFinder;
DEFINE_ART_MODULE(CrvRecoPulsesFinder)
