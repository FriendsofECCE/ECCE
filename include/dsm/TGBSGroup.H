///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: TGBSGroup.H
//
//
// CLASS SYNOPSIS:
//    This class contains a set of gaussian basis sets.  This set includes
//    a base (i.e., orbital) GBS as well as augmenting GBSs, where
//    appropriate.  Each TGBSGroup is mapped to an atom, element, or
//    molecule (depending if the configuration is by atom, by element, or
//    simple, respectively).  This mapping occurs in the TGBSConfig class,
//    which defines the overall GBS configuration for the calculation.
// 
//
// SEE ALSO:
//    TGBSConfig.H, TGaussianBasisSet.H
//    
///////////////////////////////////////////////////////////////////////////////
#ifndef TGBSGROUP_H
#define TGBSGROUP_H

#include <map>
     using std::multimap;

#include "util/CompareOps.H"

#include "dsm/TGaussianBasisSet.H"


class TGBSConfig;
class ContractionInfo;
class GBSKey;

// Typedefs (lots of maps)
typedef map<string, vector<int>, less<string> > TightMap;
typedef map<string, vector<int>, less<string> > DiffuseMap;
typedef map<string, vector<int>, less<string> > FunctionMap;
typedef map<string, vector<string>, less<string> > OrbitalMap;
typedef map<string, int, less<string> > GBSCountMap;
typedef map<string, ContractionInfo, less<string> > GBSToContInfoMap;
typedef map<string, GBSToContInfoMap, less<string> > ElementToGBSInfoMap;


class TGBSGroup : public multimap<TGaussianBasisSet::GBSType,
		                  TGaussianBasisSet*, gtgbs>
{
                
  public:
  // Constructors:
     TGBSGroup();
     TGBSGroup(TGBSGroup& tgbsGroup);

  // Destructor:
     ~TGBSGroup(void);

  // Accessors:
     vector<const TGaussianBasisSet*>* getOrbitalGBSList(void) const;
     const OrbitalMap* getOrbitalGBSMappings(void) const;

     string name(void) const;
     const TGaussianBasisSet* ecp(void) const;

     // returns ordered list of all basis sets EXCEPT the ecp and dft fittings
     // DO NOT delete these pointers
     const vector<TGaussianBasisSet*>* getOrderedList(void);

     // Says if the given GBS is already in the group or not:
     bool contains(string gbsName, TGaussianBasisSet::GBSType gbsType) const;

  
     // Takes into account whether all the contractions have been deleted
     bool hasContractions(const string& tag) const;
     bool hasContractions(const string& element, 
			  const TGaussianBasisSet* gbs) const;

     vector<string>* getOrbitalGBSIdentifiers() const;
     vector<GBSKey>* getAuxGBSIdentifiers() const;
     vector<string>* getRemovedAuxGBSIdentifiers() const;

  // Modifiers 

     /*(ALWAYS USE THESE INSERT METHODS TO INSERT A NEW GBS TO MAKE SURE
       ORDER IS PRESERVED AND ECPs ARE INSERTED CORRECTLY) */
     bool insertGBS(TGaussianBasisSet* gbs, bool recompute=false);

     void insertOrbitalGBS(string bsName,  
			   vector<TGaussianBasisSet*>& gbsList,
			   bool recompute=false);

     // SAME GOES FOR THE REMOVE METHODS:
     bool removeGBS(TGaussianBasisSet::GBSType type);
     bool removeGBS(string gbsType, string gbsName);
     void removeOrbitalGBS(string bsName);   
     void removeAll(void); // removes all basis sets

     void setOrbitalGBSMappings(const OrbitalMap& map);
     void setGBSCount(const GBSCountMap& map);

  // functions dealing with adding custom tight and diffuse functions:
     void recomputeDiffuse();
     void recomputeTight();
     bool recomputeTight(const string& element_, const int& shell, 
			 const int& numTight);
     bool recomputeDiffuse(const string& element_, const int& shell, 
	  		   const int& numDiffuse);
     // what the ContractionEditor uses
     bool addTight(const string& tag, const int& shell, const
		   int& numToAdd);
     bool addDiffuse(const string& tag, const int& shell, const
		     int& numToAdd);

     FunctionMap& getTightMap();
     FunctionMap& getDiffuseMap();
     ElementToGBSInfoMap& getContractionInfoMap();

  private:
     double getDiffuseSingleExponentRatio(const string& element_,
					  const int& shell,
					  const double& shellExponent);

     void computeRatio(const string& element_, const int& shell, 
		       double& maxExponent,
		       double& ratio, const string& bound);

     void recomputeAppendedFuncs( const FunctionMap& map,
				  const TGaussianBasisSet::GBSType& type ); 

     bool recomputeAppendedFuncs (const string& element_, 
				  const int& shell, 
				  const int& numToAdd,
				  const FunctionMap& map,
                               const TGaussianBasisSet::GBSType& type);

     void insertOrderedGBS(TGaussianBasisSet* gbs);
     void removeOrderedGBS(TGaussianBasisSet* gbs);
     void removeMetaDataFor(string uniqueGBSKey);
     void cleanMetadataMaps(const string& element);

  // Data:
     TGaussianBasisSet* p_ecp;  // null unless ecp part is added

     string             p_name; // do we need a name?
  
     vector<TGaussianBasisSet*> p_gbsOrderedVector;  // ordered list of the
                                                     // unique basis sets in
                                                     // the group
  // Metadata maps (used for Amica enhancements)
     GBSCountMap        p_gbsCount;  // Gow many agg groups each gbs belongs to

     OrbitalMap         p_orbitals;  // Basis set mappings for all the
                                     // aggregate basis sets in the group

     TightMap p_tightFunctions;      // Counts of added tight functions
     DiffuseMap p_diffuseFunctions;
     ElementToGBSInfoMap p_contractionInfoMap;   // delete and uncontract flags

  friend class TGBSConfig;
};

// Contraction
class ContractionInfo {

public:
  vector<bool> deleteFlags;
  vector<bool> uncontractFlags;

  ContractionInfo();
  ContractionInfo(const ContractionInfo& c);
  ContractionInfo(const bool& deleteValue, const bool& uncontractValue);

  bool operator==(const ContractionInfo& contInfo) const;
  void resetVectors(const bool& deleteValue, const bool& uncontractValue);
};

#endif
