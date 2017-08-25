#include <xercesc/dom/DOMDocument.hpp>
using namespace xercesc;

#include <iostream>
  using std::cout;
  using std::endl;

#include "util/Ecce.H"
#include "util/ErrMsg.H"
#include "util/SDirectory.H"
#include "util/StringTokenizer.H"

#include "dsm/JCode.H"
#include "dsm/CodeFactory.H"

#include <vector>

  using std::find;
  using std::vector;

map<const char*, const JCode*, ltstr> CodeFactory::p_cache;

typedef map<const char *, const JCode *, ltstr >::value_type CodePair;


/**
 * Protected Constructor.
 */
CodeFactory::CodeFactory() 
{
}

/**
 * Destructor.
 */
CodeFactory::~CodeFactory() 
{
}

const JCode* CodeFactory::lookup(const char* code_name) 
{
  EE_RT_ASSERT(code_name!=0,EE_FATAL, "CodeFactory: null code");
  EE_RT_ASSERT(strlen(code_name)!=0,EE_FATAL, "CodeFactory: empty code");

  map<const char*, const JCode*, ltstr>::const_iterator it =
                                         p_cache.find(code_name);
  if (it == p_cache.end()) {
    JCode *code = new JCode(code_name);
    p_cache.insert(CodePair((const char*)strdup(code_name),(const JCode*)code));
    return code;

  } else {
    return (*it).second;
  }
}

vector<const JCode*> *CodeFactory::getCodes()
{
  vector<const JCode*> *ret = new vector<const JCode*>();

  string tmp = Ecce::ecceDataPath();
  SDirectory capdir((tmp+"/client/cap").c_str());

  vector<SFile> capFiles;
  capdir.get_files(capFiles,false);

  string name;
  for (int idx=0; idx<capFiles.size(); idx++) {
    if (capFiles[idx].extension() == "edml") {
      name = capFiles[idx].pathtail();
      const JCode* jcode = CodeFactory::lookup( name.c_str() );
      ret->push_back( jcode );
    }
  }

  return ret;
}

///////////////////////////////////////////////////////////////////////////
// Description
//   Returns list of codes that have a ".expt" file in the controllers
//   directory.  Its possible that this could exist and other necessary
//   files like parse scripts not exist but not likely.
///////////////////////////////////////////////////////////////////////////
vector<string> CodeFactory::getImportCodes()
{
  vector<string> ret;

  /* TODO - wouldn't hurt to make sure it exists too.
  SDirectory sciptdir(Ecce::ecceDataControllersPath());
  vector<SFile> scriptFiles;
  sciptdir.get_files(scriptFiles,FALSE);
  */

  vector<const JCode*> *caps = CodeFactory::getCodes();
  vector<const JCode*>::iterator it;
  string script;
  for (it=caps->begin(); it!=caps->end(); it++) {

    script = "";
    (*it)->get_string("Importer",script);
    if (!script.empty()) {
      if (!((*it)->getParseVerifyPattern().empty()))  {
        ret.push_back((*it)->name());
      }
    }
  }
  delete caps;

  return ret;
}

///////////////////////////////////////////////////////////////////////////
// Description
//   Return list of codes that are fully supported which means:
//     . it has an entry in Code.tbl
//     . it has the ouput parsing scripts in place
//     . it has input file generator scripts
//     . it has a CodeCap file.
//  Each of the above bullets may have multiple files associated with it
//  but this method does the cheapest check that is going to be right most
//  of the time. 
//
//  To generate this list all we look for is:
//     . CodeCap file with Parsers file entries
//  If these exist, the rest is currently assumed.  WE can improve this
//  it it turns out not to be true.
///////////////////////////////////////////////////////////////////////////
vector<string> CodeFactory::getFullySupportedCodes()
{
  string firstCodes = getenv("ECCE_CODE_ORDER")? getenv("ECCE_CODE_ORDER"): "";
  StringTokenizer tok(firstCodes);
  vector<string> ret = tok.tokenize(" \t");
  
  vector<const JCode*> *caps = CodeFactory::getCodes();
  string parser, tmpl;
  int ir;
  for (int idx=caps->size()-1; idx>=0; idx--) {
    parser = tmpl = "";
    (*caps)[idx]->get_string("InputGenerator",parser);
    (*caps)[idx]->get_string("Template",tmpl);
    if (!parser.empty() && !tmpl.empty()) {
      for (ir=0; ir<ret.size() && ret[ir]!=(*caps)[idx]->name(); ir++);
      if (ir == ret.size())
        ret.push_back((*caps)[idx]->name().c_str());
    }
  }
  delete caps;

  return ret;
}

vector<string> CodeFactory::getFullySupportedCodeNames()
{
  string firstCodes = getenv("ECCE_CODE_ORDER")? getenv("ECCE_CODE_ORDER"): "";
  StringTokenizer tok(firstCodes);
  vector<string> ret = tok.tokenize(" \t");
  
  vector<const JCode*> *caps = CodeFactory::getCodes();
  string parser, tmpl;
  int ir;
  for (int idx=caps->size()-1; idx>=0; idx--) {
    parser = tmpl = "";
    (*caps)[idx]->get_string("InputGenerator",parser);
    (*caps)[idx]->get_string("Template",tmpl);
    if (!parser.empty() && !tmpl.empty()) {
      for (ir=0; ir<ret.size() && ret[ir]!=(*caps)[idx]->getCodeName(); ir++);
      if (ir == ret.size())
        ret.push_back((*caps)[idx]->getCodeName().c_str());
    }
  }
  delete caps;

  return ret;
}

vector<const JCode*> *CodeFactory::getBasisCodes()
{
  vector<const JCode*> *caps = CodeFactory::getCodes();
  bool rules;
  string script;
  vector<const JCode*>::iterator it;
  for (it=caps->begin(); it!=caps->end();) {
    script = "";
    rules = (*it)->exists("GaussianBasisSetRules");
    (*it)->get_string("BasisTranslationScript",script);
    if (!rules || script.empty()) {
      caps->erase(it);
    } else {
      // only advance iterator if the current entry is not removed
      // otherwise a nasty bug will be introduced and it will skip
      // over the next one without checking it and spurious entries result
      it++;
    }
  }

  return caps;
}
