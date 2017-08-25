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



class ICalcUtils
{
                
  public:

     static bool makeConsistent(Fragment* frag,
				TGBSConfig*& config,
				const JCode* code,
				TTheory* theory);

     static TGBSConfig* importConfig(istream& infile);


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







