///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: GBSRules.H
//
//
// CLASS SYNOPSIS:
// 
//
// SEE ALSO:
//    
///////////////////////////////////////////////////////////////////////////////
#ifndef GBSRULES_H
#define GBSRULES_H

class TGBSConfig;
class Fragment;
class JCode;
class TTheory;

class GBSRules
{
                
  public:

  // Static rule checking functions:

     static bool checkECPCoverage(TGBSConfig* config);
  
     static bool checkOrbitalCoverage(const TGBSConfig* config);

     static bool isComplete (const Fragment* frag,
			     const TGBSConfig* config,
			     const JCode* code,
			     const TTheory* theory,
  			     bool verifyAllGroups = true);

     static bool isConsistent(const Fragment* frag,
			      TGBSConfig* config,
			      const JCode* code,
			      const TTheory* theory);
  
     static void autoOptimize(TGBSConfig* config,
	   		      const JCode* code,
			      const TTheory* theory);

     //  The code has spherical functions only (<SphericalOnly> in its
     //  EDML), so the recorded convention may not be Cartesian.
     static bool sphericalOnly(const JCode* code);

     //  Sets Spherical on a spherical-only code's config; true if changed.
     static bool enforceCodeCoordSys(TGBSConfig* config, const JCode* code);

};
#endif







