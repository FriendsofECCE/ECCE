/**
* @file
*
*
*/
#ifndef JCODE_HH
#define JCODE_HH



#include <map>
#include <set>
#include <vector>
#include <string>
  using std::vector;
  using std::set;
  using std::string;
  using std::multimap;
  using std::map;

#include <xercesc/dom/DOMDocument.hpp>
  using namespace xercesc;

#include "util/CompareOps.H"

#include "tdat/TTheory.H"
#include "tdat/TRunType.H"
#include "tdat/TGBSAngFunc.H"
#include "tdat/GeomConstraintRules.H"

#include "dsm/TGaussianBasisSet.H"


class TypedFile;
class TTheory;
class TRunType;
class SummaryIterator;

struct lttheory {
  bool operator()(const TTheory& t1, const TTheory& t2) const {
    if (t1.category() == t2.category())
      return (t1.name() <= t2.name());
    else
      return (t1.category() < t2.category());
  }
};





/**
 * Interface to Applications Descriptor data.
 * Application descriptor files (formerly known as cap or CodeCap files
 * are now structured XMLi (.edml files).  There is currently no DTD.
 * To keep the interface as much the same as possible, the get_<type>
 * methods still work as long under the important assumption that the
 * first element of the specified name will be returned.
 * For more structured data, there are convenience methods or you can
 * ask for the Element by path.
 */
class JCode 
{

  public:

  ~JCode(void);

  int operator==(const JCode& code) const;

  // Get name of code
  string                     name(void) const;

  // Use these for getting basisset rules.  They are here for
  // compatability and return the first match (XML document order) for
  // the specified key
  bool exists(const string& key) const;
  bool get_bool(const string& key, bool& value) const;
  bool get_int(const string& key, int& value) const;
  bool get_float(const string& key, float& value) const;
  bool get_string(const string& key, string& value) const;
  bool get_string_list(const string& key,
                       vector<string>& value) const;
  bool get_float_list(const string& key,
                      vector<float>& value) const;
  bool get_bool_list(const string& key,
                     vector<bool>& value) const;
  bool get_int_list(const string& key,
                    vector<int>& value) const;


  string getCodeName() const;
  string parseDescriptor() const;
  string parseScript(const string& key) const;
  string launchPPScript() const;
  string getScript(const string& what) const;


  vector<TTheory>  *theories() const;
  vector<TRunType> *runTypes(const TTheory* theory=0) const;
  bool getTheoryRunTypeEditorNames(string& theory, string& runtype) const;
  bool getIrreducibleFragmentSupported(void) const;
  bool getBasisSetQuickListSupported(void) const;
  bool theoryNeedsBasis(const TTheory& theory) const;
  bool theoryCategoryNeedsBasis(const string& theorycat) const;

  SummaryIterator *getSummaryIterator(const string& kind) const;

  /**
   * Input/Output Code Application Data Files
   * These are the raw data files produced/consumed 
   * by/for a particular code.
   */
  enum CodeFileType
  {
    INPUT,
    PRIMARY_INPUT,
    AUXILIARY_INPUT,
    OUTPUT,
    PRIMARY_OUTPUT,
    PARSE_OUTPUT,
    AUXILIARY_OUTPUT,
    PROPERTY_OUTPUT,
    FRAGMENT_OUTPUT,
    RESTART_OUTPUT,
    TOPOLOGY_OUTPUT,
    ESP_OUTPUT,
    MD_PROPERTIES_OUTPUT,
    MD_OUTPUT
  };
  vector<TypedFile> getCodeFiles(JCode::CodeFileType type) const;
  TypedFile getCodeFile(JCode::CodeFileType type) const;
  static string codeFileTypeToStr(JCode::CodeFileType type);
  string getParseVerifyPattern() const;
  map<string, string> getSuffixMapping() const;


  // functions for MO computation
  TGBSAngFunc *getAngFunc(TGaussianBasisSet::CoordinateSystem type) const;


  // Geometry constraint rules
  GeomConstraintRules *getGeomConstraintRules() const;


  protected:

  JCode(const string& name);
  string getValue(const string& key) const;

  private:
  void cacheTheories();

  
  vector<TTheory>                             *p_theories; // preserve order
  map<TTheory,vector<TRunType>,lttheory>      *p_theoryRuntypes;
  string                                       p_name;
  DOMDocument                                 *p_doc;

  friend class CodeFactory;

};

#endif // JCODE_HH
