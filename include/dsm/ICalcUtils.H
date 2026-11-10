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
     /** Basis from five numbers per primitive (Z, l, exponent, coefficient
      *  for an unnormalised primitive, shell); null if unusable. */
     static TGBSConfig* gtoBasisConfig(const std::vector<double>& rows);

     static TGBSConfig* slaterBasisConfig(IPropCalculation* calc,
                                          bool* orthonormal = 0);

     /** ORCA's own copy of a named library basis can be the segmented
      *  form of the general contraction ECCE stores (cc-pVXZ): each
      *  contraction lacks the primitives that are also uncontracted
      *  functions of the same l, and the MO coefficients belong to that
      *  basis. Per element in the run's BASISPATTERN, where dropping those
      *  primitives gives the printed pattern exactly, their coefficients
      *  are zeroed in config. Anything else is left for the tr(PS) and
      *  C'SC checks to report. Returns true if config changed. */
     static bool applyCodeContraction(IPropCalculation* calc,
                                      TGBSConfig* config);
     /** The same from the BASISPATTERN text itself. */
     static bool applyContractionPattern(const std::string& patterns,
                                         TGBSConfig* config);


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







