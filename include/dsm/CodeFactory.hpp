#ifndef CODEFACTORY_HH
#define CODEFACTORY_HH
/////////////////////////////////////////////////////////////////////////////
// Class: CodeFactory
//
// Synopsis: Construction and queries of registered code (JCode) objects.
//
// Description:
/////////////////////////////////////////////////////////////////////////////

#include <map>
#include <vector>
#include <string>
  using std::vector;
  using std::string;
  using std::map;

#include "util/CompareOps.H"

class JCode;

class CodeFactory 
{

  public:

  static const JCode*        lookup(const char* name);
  static vector<const JCode*> *getCodes(); // All of them


  static vector<const JCode*> *getBasisCodes();
  static vector<string> getImportCodes();
  static vector<string> getFullySupportedCodes();
  static vector<string> getFullySupportedCodeNames();

  protected:

    CodeFactory();
    virtual ~CodeFactory();

  private:

  static map<const char*, const JCode*, ltstr> p_cache;
};

#endif 
