///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: ICalcUtils.H
//
//
// CLASS SYNOPSIS: Utility functions that require lookups from the
//                 EDSIGaussianBasisSetLibrary.  We put these here so tdat
//                 won't have dependencies to dsm.
// 
//
// SEE ALSO:
//    
///////////////////////////////////////////////////////////////////////////////
#ifndef ICALCUTILS_H
#define ICALCUTILS_H

#include <iostream>
  using std::istream;



#include "dsm/EDSIGaussianBasisSetLibrary.H"



class TGaussianBasisSet;
class TGBSConfig;
class Fragment;
class JCode;
class TTheory;
class TGBSGroup;



class IPropCalculation;

class ICalcUtils
{
                
  public:

     static bool makeConsistent(Fragment* frag,
				TGBSConfig*& config,
				const JCode* code,
				TTheory* theory);

     static TGBSConfig* importConfig(istream& infile);

     /**
      * The basis set implied by a semiempirical calculation.
      *
      * A semiempirical code never writes a basis set -- the valence
      * basis is implied by the Hamiltonian -- so gbsConfig() has
      * nothing to return and orbitals cannot be rendered.  Where the
      * code reported its Slater exponents (MOPAC's SLATERBASIS, from
      * the GRAPHF file), this rebuilds an equivalent contracted
      * Gaussian basis so the ordinary rendering path can evaluate them.
      *
      * @return a new config the caller owns, or null when the
      *         calculation has no such property or it cannot be used.
      *         Null means "do not render", never "render approximately".
      */
     static TGBSConfig* slaterBasisConfig(IPropCalculation* calc);


  private:

  // Methods:
     static TGBSGroup* completeGroup(TGBSGroup* oldGroup, string tag);
     static void importNumericalBasis(istream& infile, TGBSConfig* config);
     static void importNameBasis(istream& infile, TGBSConfig* config);
     static TGaussianBasisSet* getGbsTemplate(const string& line, 
                                              TGBSConfig* config);
     static void importGBSContractions(istream& infile, 
                                       TGaussianBasisSet* gbsTemplate,
                                       TGBSConfig* config);
     static void importECPContractions(istream& infile,
                                       TGBSConfig* config);

  // Data:
     // make this static so it caches gbs info:
     static EDSIGaussianBasisSetLibrary* p_gbsFactory;

};
#endif







