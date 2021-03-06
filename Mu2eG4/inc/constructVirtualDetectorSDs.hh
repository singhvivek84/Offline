#ifndef Mu2eG4_constructVirtualDetectorSDs_hh
#define Mu2eG4_constructVirtualDetectorSDs_hh
//
// Free function to make the virtual detectors sensitive
//
// constructVirtualDetectorSDs.cc
// Author: Lisa Goodenough
// 2018/03/12
//
//

namespace mu2e {

    class SimpleConfig;
    class Mu2eSensitiveDetector;

    void constructVirtualDetectorSDs(SimpleConfig const & _config,
                                     Mu2eSensitiveDetector* vdSD);

}

#endif /* Mu2eG4_constructVirtualDetectorSDs_hh */
