///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: TGBSConfig.H
//
//
// CLASS SYNOPSIS:
//    This class describes the gaussian basis set configuration for a
//    calculation.  Specifically, it allows the user to assign a TGBSGroup
//    to the whole molecule, each element, or each atom (depending upon
//    if the configuration is simple, by element, or by atom, respectively).
//    Each TGBSGroup defines a base GBS and any augmenting GBSs, where
//    applicable.  The class DavCalculation is used to retrieve a TGBSConfig
//    from a calcultion.  DavCalculation calls EDSIGaussianBasisSetLibrary
//    function load(), which actually loads the data from the DAV server.
//
// SEE ALSO:
//    TGaussianBasisSet.H, TGBSGroup.H. EDSIGaussianBasisSetLibrary.H
//    DavCalculation.H
///////////////////////////////////////////////////////////////////////////////
#ifndef TGBSCONFIG_H
#define TGBSCONFIG_H

#include <strstream>
using std::ostrstream;

#include <vector>
#include <map>
#include <set>

using std::set;
using std::less;
using std::vector;
using std::map;


#include "tdat/Fragment.H"

#include "dsm/TGBSGroup.H"
    
   class TAtm;
   class TCodeCap;
   class JCode;
   class ICalculation;
   class TGaussianBasisSet;

// used for friend functions in os_to_dav
   class Experiment;
   class GaussianBasisSet;
   class GBSConfig;

class TGBSConfig : public map<string, TGBSGroup*, less<string> >
{
                
  public:

// Static conversion arrays

   // used for counting functions & primitives:
   static short sph_mult[]; 
   static short cart_mult[];

   // converts config type to its corresponding string:
   static const char* configType_formatter[];

  // Enumerated types:
     enum ConfigType {simple, element, atom, UnknownConfigType};
     enum CountType {NumFunctions, NumPrimitives, EffNumPrimitives,
                     NumEcpCoreElectrons};

  // Constructors:
     TGBSConfig();
     TGBSConfig(TGBSConfig& config);

  // Destructor:
      virtual ~TGBSConfig();

  // Static conversion routines:
     static TGBSConfig::ConfigType strToConfigType(string ct);

  // Modifiers
     void name(const string& name);
     void type(TGBSConfig::ConfigType type);
     void coordsys(TGaussianBasisSet::CoordinateSystem coordsys);
     void optimize(bool opt);

     // tag can be an atom name (i.e., "H1"), an element name (i.e., "H"),
     // or a set of the elements in the molecule (i.e, "H C") for simple
     // configurations
     bool insertGBSGroup(const string& tag, TGBSGroup* newGroup);
     bool insertGBSGroup(const vector<string>& tags, TGBSGroup* newGroup);
     void removeGBSGroup(string tag); // removes one group
     void remove(const char* elementName); 
     void removeAllGroups();
     void removeMultipleOrbitals();
     void resetAllGroups(); // makes them all null
     void resetAllGroups(TGBSConfig::ConfigType type, 
			 string tag,
			 TGBSGroup* group);
     void resetGroup(string tag, TGBSGroup* group);
     void clearType(TGaussianBasisSet::GBSType);

  // Accessor/Modifiers that search based on the element tag
     TGBSGroup* findGroup(string element_);
     vector<const TGaussianBasisSet*> getGBSList(string element_);
     GBSToContInfoMap* getContractionInfoMap(const string& tag);
     vector<int>* getDiffuseFunctionCount(const string& tag);
     vector<int>* getTightFunctionCount(const string& tag);
     void contractionInfo(vector<int>& totalFrom, vector<int>& totalTo,
			  const string& tag);
     void hasContractionInfo(vector<bool>& perShell, const string& tag);
      
  // Accessors
     string name(void) const;
     string groupTagOfHeaviestElement() const;
     string uniqueTagStr(void) const;
     set<string, less<string> > *elements() const;
     TGBSConfig::ConfigType type(void) const;
     TGaussianBasisSet::CoordinateSystem coordsys(void) const;
     bool optimize(void) const;
     bool isEqual(TGBSConfig* otherConfig) const;
     bool isEmpty(void) const;
     bool hasEmptyGroups(void) const;
     int  numUniqueGroups() const;

     void hasChargeFitting(bool& some,bool& all) const;
     void hasExchangeFitting(bool& some,bool& all) const;
     bool hasDFTs(void) const;
     bool hasECPs(void) const;
     bool hasSpherical(bool considerLValues = false) const;
     bool hasGeneralContractions(void) const;
     bool hasMultipleOrbitals(void) const;

     vector<string>* getOrbitalGBSIdentifiers(const string& groupName);
     vector<GBSKey>* getAuxGBSIdentifiers(const string& groupName);

  // Export
     const char* dump(const char* codeName, bool useNames = true);
     bool alwaysUseExp(const string& name) const;

  // Name generators
     string ecpName() const;
     string dftFittingName() const;
     string dftChargeFittingName() const;

  // Counting functions 
     int num_functions(TagCountMap tagCounts);
     int num_primitives(TagCountMap tagCounts);
     int num_ecpCoreElectrons(TagCountMap tagCounts);
     int eff_num_primitives(TagCountMap tagCounts, bool gc_code);

     int max_l_value(void) const;
     int max_l_value(const string& element_) const;
     vector<int>* max_primitives(void) const;

  private :

     string getName(const TGBSGroup* group) const;

     ostrstream* print_contraction( const TGaussianBasisSet* gbs,
				    const char* tag, 
				    const JCode* code);
     ostrstream* print_all_contractions(
             const TGaussianBasisSet* ecp,const char* tag);
     
     ostrstream* print_contractions_noop_gcNotSupported(
             const TGaussianBasisSet* ecp,const char* tag);

     ostrstream* print_contractions_optimize_gc(
             const TGaussianBasisSet* ecp,const char* tag);

     ostrstream* print_potential(
             const TGaussianBasisSet* ecp,const char* tag);

     void printNormalContraction(ostrstream* result, 
				 Contraction_* cont, 
				 const vector<bool>& shells,
				 const char* tag);

     void printUncontractedContraction(ostrstream* result, 
					      Contraction_* cont, 
					      const int& shell,
				       const char* tag);


     int countFunctions (const TGaussianBasisSet* gbs,
			 int elementCount,
			 string element_);
     int countEcpCoreElectrons (const TGaussianBasisSet* ecp,
			 int elementCount,
			 string element_);
     int countPrimitives (const TGaussianBasisSet* gbs,
			  int elementCount,
			  string element_);
     int countEffPrimitives(const TGaussianBasisSet* gbs,
			    int elementCount,
			    string element_,
			    bool gc_code);
     int count(TGBSConfig::CountType ct, 
	       TagCountMap& tagCounts,
	       bool gc_code=false); 


  // data 
     string                 p_name;      // name of the base gbs type
     TGBSConfig::ConfigType p_type;      // simple, by element, or by atom
     bool                   p_optimize;  
     TGaussianBasisSet::CoordinateSystem  p_coordSys;

  // friends 
     friend class DavCalculation;
     friend class ICalcUtils;

     // for os_to_dav conversion utility
     friend TGBSConfig* loadImportedGBS(const Experiment* experiment,
				       GaussianBasisSet* oldGBS);
     friend TGBSConfig* loadGBSConfig(ICalculation* calc, 
				     GBSConfig* osGBSConfig);
};

// class used to convert a tag string (which may contain multiple elements
// or atom labels) into a vector of individual element tags
class TGBSConfigTags : public vector<string>
{
  public:

  // constructor used to separate every unique element, removing any
  // atom identifiers (i.e., H1 would become H)
     TGBSConfigTags(const char*, bool sorted=false, bool elementOnly=true);

     bool findElement(string elementName);
};

class GBSKey 
{
  public:
     string name;
     TGaussianBasisSet::GBSType type;
};

#endif
