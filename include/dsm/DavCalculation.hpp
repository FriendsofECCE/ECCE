///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: DavCalculation.H
//
///////////////////////////////////////////////////////////////////////////////

#ifndef DAVCALCULATION_H
#define DAVCALCULATION_H

#include <iostream>
  using std::istream;
#include <map>
  using std::map;
  using std::less;
#include <strstream>
  using std::ostrstream;

// xml stuff
#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMNode.hpp>
  using namespace xercesc;

#include "dsm/ICalculation.H"

#include "util/IOException.H"

class EDSI;
class MetaDataResult;
class TTheory;
class Fragment;
class TGBSConfig;
class TProperty;
class JCode;
class TRunType;
class TGaussianBasisSet;
class TGBSGroup;

/**
 * Implementation of the ICalculation abstract class for accessing
 * persistant calculation structures.
 * 
 * DavCalculation provids an implementation of access architecture
 * for applications requiring persistant EDSI Ecce calculation data
 * structure access.  This architecture is dependent upon the new
 * OO data model which allows a calculation to contain 1 or more tasks. 
 * Each task describes Chemistry code input parameters, launch data,
 * molecular data, gaussian basis sets, and calculation result
 * property containers.
 */
class DavCalculation : virtual public ICalculation 
{

  public:

    enum TheoryType
    {
      GENERAL,
      SINGLE,
      UNDEFINED_THEORY
    };

    friend class EDSIFactory; 

    // ICalculation virtuals
    bool theory(TTheory* theory, const string& theoryType="");
    TTheory* theory(const string& theoryType="") const;
    bool runtype(const TRunType& runtype);
    TRunType runtype(void) const;
    bool gbsConfig(TGBSConfig* config, const string& theoryType=""); 
    TGBSConfig* gbsConfig(const string& theoryType="") const;
    bool gbsUseExpCoeff(bool useExpCoeff);
    bool gbsUseExpCoeff(void) const;
    bool guiparams(GUIValues *params, const string& theoryType="");
    GUIValues *guiparams(const string& theoryType="") const;
    bool openShells(unsigned long openshell);
    unsigned long openShells(void) const;
    bool spinMultiplicity(SpinMult::SpinMultEnum sm);
    SpinMult::SpinMultEnum spinMultiplicity(void) const;

    // ChemistryTask virtuals
    virtual bool fragment(Fragment* frag);
    virtual Fragment* fragment() const;

    // Resource virtuals
    virtual bool remove();
    virtual bool copy(EcceURL& target);
    virtual bool move(EcceURL& target);
    virtual bool canModifyInputProvider();

    static string theoryTypeToString(DavCalculation::TheoryType theory);
    static DavCalculation::TheoryType stringToTheoryType(const string& str);

  private:

    bool save(istream &data, const EcceMap& props) const;
  
    string diffuseMap(TGBSGroup* group);
    string tightMap(TGBSGroup* group);

    string appendedFunctionMap(const FunctionMap& map);
    string contractionInfoMap(TGBSGroup* group);
    void writeConfigMapElement(TGBSConfig* config, ostrstream& output); 
    void writeConfigDataElement(TGBSConfig* config, ostrstream& output);
    bool getConfigMetaData(TGBSConfig* config) const;

    void parseGbsData(TGaussianBasisSet* gbs, istream* is ) const;
    void parseEcpData(TGaussianBasisSet* ecp, istream* is ) const;
    void parseConfigMapElement(const DOMDocument& doc, TGBSConfig* config,
            map<string, TGaussianBasisSet*, less<string> >& gbsCache) const;
    void parseGroupOrbitalMap(const DOMNode& groupNode, 
                              TGBSGroup* group, string groupName) const;
    void parseGroupTightMap(const DOMNode& groupNode, TGBSGroup* group) const;
    void parseGroupDiffuseMap(const DOMNode& groupNode, TGBSGroup* group) const;
    void parseGroupFunctionMap(const DOMNode& groupNode, FunctionMap& fMap,
                               const string& nodeID) const;
    void parseGroupContractionInfoMap(const DOMNode& groupNode, 
                                      TGBSGroup* group) const;
    void parseConfigDataElement(const DOMDocument& doc, TGBSConfig* config,
            map<string, TGaussianBasisSet*, less<string> >& gbsCache) const;
    void parseGroupGBSOrderedList(const DOMNode& groupNode, TGBSGroup* group,
            map<string, TGaussianBasisSet*, less<string> >& gbsCache,
            map<string, TGaussianBasisSet*, less<string> >& usedBasisSets) const;

   virtual string getParseScriptArgs() const;
   virtual string import( const string& dir, const string& file) 
        throw(EcceException);

  protected:

    virtual void processImportParameters(const Preferences& params)
      throw(IOException);

    DavCalculation(const EcceURL& url) throw(InvalidException);

    string getXMLElementValue(const DOMDocument& doc, const string& name) const;

    const DOMDocument* getXMLDocument(const string& docTail) const;

};

#endif 

