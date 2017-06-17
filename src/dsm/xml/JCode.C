#include <iostream>
  using std::cout;
  using std::cerr;
  using std::endl;
#include <fstream>
  using std::ifstream;
#include <algorithm>
  using std::find;
  using std::less;

#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMNode.hpp>
#include <xercesc/dom/DOMElement.hpp>
#include <xercesc/dom/DOMNamedNodeMap.hpp>
#include <xercesc/dom/DOMNodeList.hpp>
#include <xercesc/util/XMLString.hpp>
using namespace xercesc;

#include "util/Ecce.H"
#include "util/ErrMsg.H"
#include "util/TypedFile.H"
#include "util/SFile.H"
#include "util/EcceMap.H"

#include "tdat/TTheory.H"
#include "tdat/TRunType.H"
#include "tdat/TGBSAngFunc.H"
#include "tdat/GeomConstraintRules.H"

#include "dsm/BasicDOMParser.H"
#include "dsm/JCode.H"
#include "dsm/SummaryIterator.H"
#include "dsm/ParseErrorReporter.H"
#include "dsm/ResourceType.H"


// typedef for theory map iterator
typedef map<TTheory, vector<TRunType>, lttheory >::const_iterator TRIterator;

// typedef for STL pair class for theory map 
typedef map<TTheory, vector<TRunType>, lttheory >::value_type TRPair;

typedef map<const char *, const JCode *, ltstr >::value_type CodePair;



/**
 * Protected Constructor.
 */
JCode::JCode(const string& code_name) 
{

  p_theories = 0; // used to keep theory order as defined in files
  p_theoryRuntypes = 0; // we cache theories once they've been asked for 
  p_doc = 0;

  p_name = code_name;

  string file = Ecce::ecceDataPath();
  file += "/client/cap/";

  // Lookup EDML file name from resource descriptor
  // GDB 6/4/12 (previously assumed it was <code_name>.edml)
  string descFile = "";
  ResourceDescriptor rd = ResourceDescriptor::getResourceDescriptor();
  ResourceType *resType = rd.getResourceType("virtual_document",
                                            "ecceCalculation", code_name);
  if (resType != (ResourceType*)0) {
    descFile = resType->getDescriptorFile();
  }

  if (descFile != "") {
    file += descFile;
  } else {
    // fall back to <code_name>.edml
    file += code_name;
    file += ".edml";
  }

  BasicDOMParser parser;
  try {
    p_doc = parser.parse(SFile(file));
  } catch (const EcceException& toCatch) {
    EE_RT_ASSERT(false, EE_FATAL, (file + ": " + toCatch.what()));
  }

  // Maybe we should be throwing an exception here...
  EE_RT_ASSERT( (p_doc != 0 && p_doc->hasChildNodes()), 
      EE_FATAL, (code_name + ": unparsable"));

  cacheTheories();

}

/**
 * Destructor.
 */
JCode::~JCode(void) 
{
  if (p_doc != 0) p_doc->release();
  p_doc = 0;
}

/**
 * Equality check - identity.
 */
int JCode::operator==(const JCode& code) const
{
  return (this == &code);
}

bool JCode::exists(const string& key) const
{
  bool ret = false;
  XMLCh *tmpStr = XMLString::transcode(key.c_str());
  DOMNodeList *nodes = p_doc->getElementsByTagName(tmpStr);
  delete [] tmpStr;

  int numNodes = 0;
  if (nodes != 0) numNodes = nodes->getLength();

  if (numNodes > 0) {
    ret = true;
  }
  return ret;
}
/**
 * Get 1st value for key.  Strip white space.
 */
string JCode::getValue(const string& key) const
{
  string ret;

  XMLCh *tmpStr = XMLString::transcode(key.c_str());
  DOMNodeList *nodes = p_doc->getElementsByTagName(tmpStr);
  delete [] tmpStr;

  int numNodes = 0;
  if (nodes != 0) numNodes = nodes->getLength();
  
  if (numNodes > 0) {
    DOMElement* ele = (DOMElement *) nodes->item(0);
    if (ele->hasChildNodes()) {
      DOMNode *child = ele->getFirstChild();
      char *value = XMLString::transcode(child->getNodeValue());
      ret = value;
      delete [] value;
    }
  }

  if (!ret.empty()) {
    int start = ret.find_first_not_of(" \t");
    int end = ret.find_last_not_of(" \t");
    if (start != 0 || end != 0)
      ret = ret.substr(start,end-start+1);
  }
  return ret;
}


bool JCode::get_bool(const string& key, bool& value) const 
{
  bool retval = false;
  string tmp = getValue(key);
  if (!tmp.empty()) {

    retval = true;  // found value

    value = (stricmp(tmp.c_str(),"true") == 0);
  }

  return retval;
}

bool JCode::get_int(const string& key, int& value) const 
{
  bool retval = false;
  string tmp = getValue(key);

  if (!tmp.empty()) {

    retval = true;
    value = atol(tmp.c_str());
  }

  return retval;
}

bool JCode::get_float(const string& key, float& value) const
{
  bool retval = false;
  string tmp = getValue(key);

  if (!tmp.empty()) {

    retval = true;
    value = strtol( tmp.c_str(), (char**)0, 10 );
  }
  return retval;
}

bool JCode::get_string(const string& key, string& value) const
{
  bool retval = false;
  string tmp = getValue(key);

  if (!tmp.empty()) {
    retval = true;
    value = tmp;
  }
  return retval;
}

bool JCode::get_string_list(const string& key, vector<string>& value) const
{
  bool retval = false;
  string tmp = getValue(key);

  if (!tmp.empty()) {

    retval = true;
    char *str = strdup(tmp.c_str());
    char* tok = strtok(str," \t");
    while (tok) {
      value.push_back(tok);
      tok = strtok((char*)0," \t");
    }
    free(str);
  }
  return retval;
}

bool JCode::get_float_list(const string& key, vector<float>& value) const
{
  bool retval = false;
  string tmp = getValue(key);

  if (!tmp.empty()) {

    retval = true;
    char *str = strdup(tmp.c_str());
    char* tok = strtok(str," \t");
    float f;
    while (tok) {
      f = strtol(tok,(char**)0,10);
      value.push_back(f);
      tok = strtok((char*)0," \t");
    }
    free(str);
  }
  return retval;
}

bool JCode::get_int_list(const string& key, vector<int>& value) const
{
  bool retval = false;
  string tmp = getValue(key);

  if (!tmp.empty()) {

    retval = true;
    char *str = strdup(tmp.c_str());
    char* tok = strtok(str," \t");
    while (tok) {
      value.push_back(atoi(tmp.c_str()));
      tok = strtok((char*)0," \t");
    }
    free(str);
  }

  return retval;
}

bool JCode::get_bool_list(const string& key, vector<bool>& value) const
{
  bool retval = false;
  string tmp = getValue(key);
  if (!tmp.empty()) {

    retval = true;
    char *str = strdup(tmp.c_str());
    char* tok = strtok(str," \t");
    while (tok) {
      if (stricmp(tok,"true") == 0) {
        value.push_back(true);
      } else {
        value.push_back(false);
      }
      tok = strtok((char*)0," \t");
    }
    free(str);
  }

  return retval;
}



string JCode::name(void) const 
{
  return p_name;
}

string JCode::getScript(const string& what) const
{
  string script = "";

  if (what == "GBSExport")
    get_string("BasisTranslationScript", script);
  else if (what == "CalcImport")
    get_string("Importer", script);

  EE_RT_ASSERT(script != "", EE_WARNING, "script not found")

  return script;
}

//////////////////////////////////////////////////////////////////////////
// Description
//   Return the parse desciptor file name for the specified code.
//////////////////////////////////////////////////////////////////////////
string JCode::parseDescriptor() const
{
  string script = "";

  get_string("ParseSpecification", script);

  EE_RT_ASSERT(script != "", EE_WARNING, "No parse descriptor file");

  return script;
}

//////////////////////////////////////////////////////////////////////////
// Description
//   Return the launch post processing script name for the specified
//   code.
//   This script is not required to exist so no error checking is done.
//////////////////////////////////////////////////////////////////////////
string JCode::launchPPScript() const
{
  string script = "";

  get_string("LaunchPreprocessor", script);

  EE_RT_ASSERT(script != "", EE_WARNING, "No launch post processing script");

  return script;
}

///////////////////////////////////////////////////////////////////////////////
///  man
//
//  Description
//   Return the name of the script (full path) that parses pKey.
//
//  Implementation
//
///////////////////////////////////////////////////////////////////////////////
string JCode::parseScript(const string& pKey) const
{
  static string loaded;
  static EcceMap s_dict;

  string ret = "";      //empty if not known

  string path = Ecce::ecceDataControllersPath();
  string tmp;
  get_string("ParseSpecification", tmp);
  EE_RT_ASSERT(!tmp.empty(), EE_WARNING, "No parse descriptor file specified");

  // If we haven't loaded the file for this code, do it now
  string code = this->name();
  if (loaded.find(code) == string::npos) {
    path += "/" + tmp;
    ifstream is(path.c_str());
    if (is) {
      string line, key, value;
      int startText, endText;
      char buf[1001];

      while (is.good()) {
        buf[0] = '\0';
        is.getline(buf, 1000);
        line = buf;
        if (line.find("[") == 0 && line.find("[END]") == string::npos) {
          // new section
          key = line.substr(1,line.find(']')-1);
          for (int i = 0; i < key.size(); i++) {
            key[i] = tolower(key[i]);
          }
	  
          while (is.good()) {
            buf[0] = '\0';
            is.getline(buf, 1000);
            line = buf;
            if(line.find("[END]") != string::npos) {
              break;
            }

            if (line.find("Script") == 0) {
              value = line.substr(line.find("=")+1, string::npos);
              startText = value.find_first_not_of(' ');
              endText= value.find_last_not_of(' ');
              value = value.substr(startText, endText-startText +1);
              key.insert(0, code);
              s_dict[key] = value;
              break;
            }
          }
        }
      }
      is.close();
    }
    loaded += code;
  }

  string key = code + pKey;
  string script;
  if (s_dict.findValue(key, script)) {
    ret = script;
  }

  return ret;
}

SummaryIterator *JCode::getSummaryIterator(const string& kind) const
{
  XMLCh *tmpStr = XMLString::transcode(kind.c_str());
  return new SummaryIterator(*(p_doc->getElementsByTagName(tmpStr)));
  delete [] tmpStr;
}


/**
 * Return all theories for this specified code.
 *
 * We assume that the Theory elements are well formed.
 */
vector<TTheory>* JCode::theories() const
{
  return new vector<TTheory>(*p_theories);
}

void JCode::cacheTheories()
{
  if (p_theoryRuntypes == 0) {
    p_theoryRuntypes = new map<TTheory,vector<TRunType>,lttheory>;
    p_theories = new vector<TTheory>;

    XMLCh *tmpStr = XMLString::transcode("Theory");
    DOMNodeList *theoryNodes = p_doc->getElementsByTagName(tmpStr);
    delete [] tmpStr;

    int numTheories = 0;
    if (theoryNodes != 0) numTheories = theoryNodes->getLength();

    DOMElement *theoryEle = 0;
    int numAttr, adx;
    bool noSpherical;
    DOMNamedNodeMap *attributes = 0;
    DOMNode *attribute = 0;
    string tcat, tname, basisStr;
    bool basisFlag;
    char *attributeName = (char*)0;
    char *cptr = (char*)0;
    vector<TRunType> runtypes;
    for (int idx=0; idx<numTheories; idx++) {
      tcat = tname = "";
      runtypes.clear();
      basisFlag = true;
      theoryEle = (DOMElement *) theoryNodes->item(idx);
      attributes = theoryEle->getAttributes();
      numAttr = attributes->getLength();
      for (adx=0; adx<numAttr; adx++) {
        attribute = attributes->item(adx);
        attributeName = XMLString::transcode(attribute->getNodeName());
        if (strcmp(attributeName, "category") == 0) {
          cptr = XMLString::transcode(attribute->getNodeValue());
          tcat = cptr;
          delete [] cptr;
        }
        if (strcmp(attributeName, "name") == 0) {
          cptr = XMLString::transcode(attribute->getNodeValue());
          tname = cptr;
          delete [] cptr;
        }
        if (strcmp(attributeName, "needsBasis") == 0) {
          cptr = XMLString::transcode(attribute->getNodeValue());
          basisStr = cptr;
          if (basisStr=="false" || basisStr=="FALSE" || basisStr=="False")
            basisFlag = false;
          delete [] cptr;
        }
        delete [] attributeName;
      }

      if (!tcat.empty() && !tname.empty()) {
        TTheory theory(tcat, tname, basisFlag);

        // Now load up the runtypes....
        tmpStr = XMLString::transcode("runtype");
        DOMNodeList *runtypeNodes = theoryEle->getElementsByTagName(tmpStr);
        delete [] tmpStr;

        int numRunTypes = 0;
        if (runtypeNodes != 0) numRunTypes = runtypeNodes->getLength();

        DOMElement *runtypeEle = 0;
        DOMNode *child = 0;
        char *value;
        for (int rdx=0; rdx<numRunTypes; rdx++) {
          runtypeEle = (DOMElement *) runtypeNodes->item(rdx);
          noSpherical = false;
          if (runtypeEle->hasChildNodes()) {
            child = runtypeEle->getFirstChild();
            value = XMLString::transcode(child->getNodeValue());

            attributes = runtypeEle->getAttributes();
            numAttr = attributes->getLength();
            for (int adx=0; adx<numAttr; adx++) {
              attribute = attributes->item(adx);
              attributeName = XMLString::transcode(attribute->getNodeName());
              if (strcmp(attributeName, "noSpherical") == 0) {
                cptr = XMLString::transcode(attribute->getNodeValue());
                noSpherical = strcmp(cptr, "true")==0;
                delete [] cptr;
              }
            }

            runtypes.push_back(TRunType(value, noSpherical));
            delete [] value;
          }
        }

        p_theories->push_back(theory);
        p_theoryRuntypes->insert(TRPair(theory,runtypes));
      } else {
        cerr << "warn about bad theory entry" << endl;
      }
    }
  }
}

/**
 * Returns all runtypes for the specified theory or all types if the
 * passed parameter is a 0 pointer.
 * return all runtypes in this case.  It seems strange but there are
 * some pratical reasons:
 *
 * From Gary regarding calced:
 * All runtypes is preferable.  That way I create the buttons once and
 * only sensitize/desensitize as needed.  Otherwise I have to
 * rebuild the menu (deleting old object) each time they select a
 * different theory.  I also think it is better for them to see the
 * "extent" of the runtypes and they can figure out what theory will
 * support it if they really want a certain kind of property (I have
 * no idea if a chemist would ever really want this behavior but it
 * does seem more flexible than always showing the minimal list).
 * Finally, as you may be aware there is a memory leak associated
 * with deleting TeleUSE objects from menus--we investigated this a
 * couple years ago but didn't find an easy solution (it is some
 * internal TeleUSE structures that can't handle the deletions).  We
 * can't call XtDestroyWidget because it causes a core so really we
 * are just adding to memory each time we have code that acts like
 * it is deleting menu items.
 */
vector<TRunType> *JCode::runTypes(const TTheory* theory) const
{
  // Use vector to maintain order
  vector<TRunType>* result = new vector<TRunType>;
  if (theory == 0) {
    TRIterator trit;
    vector<TRunType>::const_iterator runit;
    vector<TRunType> runtypes;
    for (trit=p_theoryRuntypes->begin();trit!=p_theoryRuntypes->end();trit++) {
      runtypes = (*trit).second;
      for (runit=runtypes.begin(); runit!=runtypes.end(); runit++) {
        // gotta toss duplicates
        if (find(result->begin(),result->end(),*runit) == result->end()) {
          result->push_back(*runit);
        }
      }
    }

  } else {
    TRIterator trit;
    vector<TRunType>::const_iterator runit;
    vector<TRunType> runtypes;
    for (trit=p_theoryRuntypes->begin();trit!=p_theoryRuntypes->end();trit++) {
      if ((*trit).first == *theory) {
        runtypes = (*trit).second;
        for (runit=runtypes.begin(); runit!=runtypes.end(); runit++) {
          result->push_back(*runit);
        }
      }
    }
  }

  return result;
}

bool JCode::getTheoryRunTypeEditorNames(string& theory, string& runtype) const
{
  theory = "";
  runtype = "";

  XMLCh *tmpStr = XMLString::transcode("Editor");
  DOMNodeList *editorNodes = p_doc->getElementsByTagName(tmpStr);
  delete [] tmpStr;

  EE_ASSERT((editorNodes != 0) && (editorNodes->getLength()>0),EE_WARNING,"No Editor tag found");

  // Now dig out the editor names
  char* cptr = 0;
  DOMElement *editorElem = (DOMElement *) editorNodes->item(0);
  DOMNamedNodeMap *attributes = editorElem->getAttributes();

  tmpStr = XMLString::transcode("theorydialog");
  DOMNode *attribute = attributes->getNamedItem(tmpStr);
  delete [] tmpStr;

  if (attribute != 0) {
    cptr = XMLString::transcode(attribute->getNodeValue());
    theory = cptr;
    string pyfile = Ecce::ecceHome();

    pyfile += "/scripts/codereg/";
    pyfile += theory;
    SFile sfile(pyfile);

    // Trickiness to invoke wx enabled python.  Need to pass $LD_LIBRARY_PATH
    // to find the wxWidgets shared libraries plus any system libraries
    // in case the local version isn't compatible (e.g. 32-bit distributions)
    theory = "LD_LIBRARY_PATH=../3rdparty/wxwidgets/lib:../3rdparty/system/lib"
             ":../3rdparty/local/lib python " + pyfile;
 
    // Note that this will return just the theorydialog value if the
    // script does not exist in codereg which is the proper fallback if the
    // detail dialog is an executable in the platform/*/bin directory
    delete [] cptr;
  }

  tmpStr = XMLString::transcode("runtypedialog");
  attribute = attributes->getNamedItem(tmpStr);
  delete [] tmpStr;

  if (attribute != 0) {
    cptr = XMLString::transcode(attribute->getNodeValue());
    runtype = cptr;
    string pyfile = Ecce::ecceHome();

    pyfile += "/scripts/codereg/";
    pyfile += runtype;
    SFile sfile(pyfile);

    runtype = "LD_LIBRARY_PATH=../3rdparty/wxwidgets/lib:../3rdparty/system/lib"
              ":../3rdparty/local/lib python " + pyfile;
    
    // Note that this will return just the runtypedialog value if the
    // script does not exist in codereg which is the proper fallback if the
    // detail dialog is an executable in the platform/*/bin directory
    delete [] cptr;
  }

  return (theory!="" || runtype!="");
}

bool JCode::getIrreducibleFragmentSupported(void) const
{
  bool supportedFlag = false;

  XMLCh *tmpStr = XMLString::transcode("Editor");
  DOMNodeList *editorNodes = p_doc->getElementsByTagName(tmpStr);
  delete [] tmpStr;

  EE_ASSERT((editorNodes != 0) && (editorNodes->getLength()>0),EE_WARNING,"No Editor tag found");

  char* cptr = 0;
  DOMElement *editorElem = (DOMElement *) editorNodes->item(0);
  DOMNamedNodeMap *attributes = editorElem->getAttributes();

  tmpStr = XMLString::transcode("irreducibleFragmentSupported");
  DOMNode *attribute = attributes->getNamedItem(tmpStr);
  delete [] tmpStr;

  if (attribute != 0) {
    cptr = XMLString::transcode(attribute->getNodeValue());
    string str = cptr;
    if (str=="true" || str=="TRUE" || str=="True")
      supportedFlag = true;
    delete [] cptr;
  }

  return supportedFlag;
}

bool JCode::getBasisSetQuickListSupported(void) const
{
  bool supportedFlag = true;

  XMLCh *tmpStr = XMLString::transcode("Editor");
  DOMNodeList *editorNodes = p_doc->getElementsByTagName(tmpStr);
  delete [] tmpStr;

  EE_ASSERT((editorNodes != 0) && (editorNodes->getLength()>0),EE_WARNING,"No Editor tag found");

  DOMElement *editorElem = (DOMElement *) editorNodes->item(0);
  DOMNamedNodeMap *attributes = editorElem->getAttributes();

  tmpStr = XMLString::transcode("basisSetQuickListSupported");
  DOMNode *attribute = attributes->getNamedItem(tmpStr);
  delete [] tmpStr;

  char* cptr = 0;
  if (attribute != 0) {
    cptr = XMLString::transcode(attribute->getNodeValue());
    string str = cptr;
    if (str=="false" || str=="FALSE" || str=="False")
      supportedFlag = false;
    delete [] cptr;
  }

  return supportedFlag;
}

bool JCode::theoryNeedsBasis(const TTheory& theory) const
{
  bool ret = true;

  for (int it=0; it < p_theories->size(); it++) {
    if ((*p_theories)[it] == theory) {
      ret = (*p_theories)[it].needsBasis();
      break;
    }
  }

  return ret;
}

bool JCode::theoryCategoryNeedsBasis(const string& theorycat) const
{
  bool ret = true;

  for (int it=0; it < p_theories->size(); it++) {
    if ((*p_theories)[it].category() == theorycat) {
      ret = (*p_theories)[it].needsBasis();
      break;
    }
  }

  return ret;
}

string JCode::getCodeName(void) const
{
  string codeName = name();

  XMLCh *tmpStr = XMLString::transcode("appdescriptor");
  DOMNodeList *appNodes = p_doc->getElementsByTagName(tmpStr);
  delete [] tmpStr;

  EE_ASSERT((appNodes != 0) && (appNodes->getLength()>0), EE_WARNING,
            "No appdescriptor tag found");

  DOMElement *appElem = (DOMElement *) appNodes->item(0);
  DOMNamedNodeMap *attributes = appElem->getAttributes();

  tmpStr = XMLString::transcode("codeName");
  DOMNode *attribute = attributes->getNamedItem(tmpStr);
  delete [] tmpStr;

  char* cptr = 0;
  if (attribute != 0) {
    cptr = XMLString::transcode(attribute->getNodeValue());
    codeName = cptr;
    delete [] cptr;
  }

  return codeName;
}


vector<TypedFile> JCode::getCodeFiles(JCode::CodeFileType type) const
{
  vector<TypedFile> ret;
  vector<JCode::CodeFileType> types;
  bool isValidCodeFileType = true;
  string elementVal;
  string attributeVal;
  int numTypes;

  switch (type) {
    case INPUT:
      types.push_back(PRIMARY_INPUT);
      types.push_back(AUXILIARY_INPUT);
      elementVal = codeFileTypeToStr(INPUT);
      break;
    case PRIMARY_INPUT:
    case AUXILIARY_INPUT:
      types.push_back(type);
      elementVal = codeFileTypeToStr(INPUT);
      break;
    case OUTPUT:
      types.push_back(PRIMARY_OUTPUT);
      types.push_back(PARSE_OUTPUT);
      types.push_back(AUXILIARY_OUTPUT);
      types.push_back(PROPERTY_OUTPUT);
      types.push_back(FRAGMENT_OUTPUT);
      types.push_back(RESTART_OUTPUT);
      types.push_back(TOPOLOGY_OUTPUT);
      types.push_back(ESP_OUTPUT);
      types.push_back(MD_PROPERTIES_OUTPUT);
      types.push_back(MD_OUTPUT);
      elementVal = codeFileTypeToStr(OUTPUT);
      break;
    case PRIMARY_OUTPUT:
    case PARSE_OUTPUT:
    case AUXILIARY_OUTPUT:
    case PROPERTY_OUTPUT:
    case FRAGMENT_OUTPUT:
    case RESTART_OUTPUT:
    case TOPOLOGY_OUTPUT:
    case ESP_OUTPUT:
    case MD_PROPERTIES_OUTPUT:
    case MD_OUTPUT:
      types.push_back(type);
      elementVal = codeFileTypeToStr(OUTPUT);
      break;
    default:
      isValidCodeFileType = false;
      break;
  }

  if (isValidCodeFileType) {
    numTypes = types.size();
    for (int i=0; i<numTypes; i++) {
      attributeVal = codeFileTypeToStr(types[i]);
      XMLCh *tmpStr = XMLString::transcode(elementVal.c_str());
      DOMNodeList *files = p_doc->getElementsByTagName(tmpStr);
      delete [] tmpStr;
      int numFiles = files->getLength();
      EE_ASSERT(numFiles>0,EE_WARNING,"No tags found");
      DOMElement *ele = 0;
      DOMNamedNodeMap *attributes = 0;
      DOMNode *attribute = 0;
      char *cptr = (char*)0;
      DOMNode *child = 0;
      for (int j=0; j<numFiles; j++) {
        string fileName = ""; 
        string fileMimeType;
        string fileType;
        ele = (DOMElement *) files->item(j);
        attributes = ele->getAttributes();
        tmpStr = XMLString::transcode("type");
        attribute = attributes->getNamedItem(tmpStr);
        delete [] tmpStr;
        EE_ASSERT(attribute!=0,EE_WARNING,"No data file type specfied");
        if (attribute != 0) {
          cptr = XMLString::transcode(attribute->getNodeValue());
          fileType = cptr;
          delete [] cptr;
        }
        if (fileType == attributeVal) {

          // Get MIME Type
          tmpStr = XMLString::transcode("mimetype");
          attribute = attributes->getNamedItem(tmpStr);
          delete [] tmpStr;

          EE_ASSERT(attribute!=0,EE_WARNING,"No mimetype specified");
          if (attribute != 0) {
            cptr = XMLString::transcode(attribute->getNodeValue());
            fileMimeType = cptr;
            delete [] cptr;
          }

          //EE_ASSERT(ele->hasChildNodes(),EE_WARNING,"No Input file specified");
          // Only Mime type is required
          // Get File Name
          if (ele->hasChildNodes()) {
            child = ele->getFirstChild();
            cptr = XMLString::transcode(child->getNodeValue());
            fileName = cptr;
            delete [] cptr;
          }
          ret.push_back(TypedFile(fileName, fileMimeType));
        }
      }
    }
  }
  return ret;

}


TypedFile JCode::getCodeFile(JCode::CodeFileType type) const {

  TypedFile ret;

  vector<TypedFile> codeFiles = getCodeFiles(type);
  if (codeFiles.size() > 0) {
    ret = codeFiles[0];
  }

  return ret;
}



string JCode::codeFileTypeToStr(JCode::CodeFileType type) {

  string ret = "";

  switch (type) {
    case JCode::INPUT: 
      ret = "Input";
      break;
    case JCode::PRIMARY_INPUT: 
      ret = "primary";
      break;
    case JCode::AUXILIARY_INPUT:
      ret = "auxiliary";
      break;
    case JCode::OUTPUT:
      ret = "Output";
      break;
    case JCode::PRIMARY_OUTPUT:
      ret = "primary";
      break;
    case JCode::PARSE_OUTPUT:
      ret = "parse";
      break;
    case JCode::AUXILIARY_OUTPUT:
      ret = "auxiliary";
      break;
    case JCode::PROPERTY_OUTPUT:
      ret = "property";
      break;
    case JCode::FRAGMENT_OUTPUT:
      ret = "fragment";
      break;
    case JCode::RESTART_OUTPUT:
      ret = "restart";
      break;
    case JCode::TOPOLOGY_OUTPUT:
      ret = "topology";
      break;
    case JCode::ESP_OUTPUT:
      ret = "esp";
      break;
    case JCode::MD_PROPERTIES_OUTPUT:
      ret = "mdproperties";
      break;
    case JCode::MD_OUTPUT:
      ret = "mdoutput";
      break;
  }
  return ret;
}


string JCode::getParseVerifyPattern() const
{
  string ret;
  string type;
  XMLCh *tmpStr = XMLString::transcode(codeFileTypeToStr(OUTPUT).c_str());
  DOMNodeList *files = p_doc->getElementsByTagName(tmpStr);
  delete [] tmpStr;

  int numFiles = 0;
  if (files != 0) numFiles = files->getLength();

  EE_ASSERT(numFiles>0,EE_WARNING,"No Output tags found");

  DOMElement *ele = 0;
  DOMNamedNodeMap *attributes = 0;
  DOMNode *attribute = 0;
  char *cptr = (char*)0;
  for (int idx=0; idx<numFiles; idx++) {
    ele = (DOMElement *) files->item(idx);
    // Find the primary output node
    type = "";
    attributes = ele->getAttributes();
    tmpStr = XMLString::transcode("type");
    attribute = attributes->getNamedItem(tmpStr);
    delete [] tmpStr;
    EE_ASSERT(attribute!=0,EE_WARNING,"No output file type specfied");
    if (attribute != 0) {
      cptr = XMLString::transcode(attribute->getNodeValue());
      type = cptr;
      delete [] cptr;
    }
    if (type == "parse") {
      XMLCh *tmpStr = XMLString::transcode("verifypattern");
      attribute = attributes->getNamedItem(tmpStr);
      delete [] tmpStr;

      if (attribute != 0) {
        cptr = XMLString::transcode(attribute->getNodeValue());
        ret = cptr;
        delete [] cptr;
        break;
      }
    }
  }

  return ret;
}


/**
 * Sometimes a code will generate files that are difficult
 * to uniquely mime-type on the data server, because their file suffix is 
 * ambiguous (e.g. .out).  In order to ensure that a code's generated
 * files can be uniquely mime-typed on the data server, the 
 * file is renamed with a new suffix.  The element <SuffixMap> 
 * contain's a code's list of suffix mappings.
 *
 * @return returns an stl map containg the code's suffix mappings. The
 *         key is the "from" suffix and the key's value is the "to"
 *         suffix.
 *
 */
map<string, string> JCode::getSuffixMapping() const
{
  map<string, string> ret;

  XMLCh *tmpStr = XMLString::transcode("SuffixMap");
  DOMNodeList *mappings = p_doc->getElementsByTagName(tmpStr);
  delete [] tmpStr;
  int numMappings = mappings->getLength();
  DOMElement *mapping = 0;
  DOMNamedNodeMap *attributes = 0;
  DOMNode *attribute = 0;
  char *cptr = (char*)0;

  for (int i=0; i<numMappings; i++) {

    string fromSuffix = ""; 
    string toSuffix = ""; 
    mapping = (DOMElement *)mappings->item(i);
    attributes = mapping->getAttributes();

    // Get FROM suffix
    tmpStr = XMLString::transcode("from");
    attribute = attributes->getNamedItem(tmpStr);
    delete [] tmpStr;
    if (attribute != 0) {
      cptr = XMLString::transcode(attribute->getNodeValue());
      fromSuffix = cptr;
      delete [] cptr;
    }

    // Get TO suffix
    tmpStr = XMLString::transcode("to");
    attribute = attributes->getNamedItem(tmpStr);
    delete [] tmpStr;
    if (attribute != 0) {
      cptr = XMLString::transcode(attribute->getNodeValue());
      toSuffix = cptr;
      delete [] cptr;
    }

    // Store in return map
    if (!fromSuffix.empty() && !toSuffix.empty()) {
      ret[fromSuffix] = toSuffix;
    }

  }
    
  return ret;

}


TGBSAngFunc* JCode::getAngFunc(TGaussianBasisSet::CoordinateSystem type) const
{
  TGBSAngFunc *ret = new TGBSAngFunc;
  AngMomFunc AMfunc;
  int idx, adx, ldx;
  DOMElement *MOOrder = 0;
  DOMNode *MOShell = 0;
  DOMNamedNodeMap *attributes = 0;
  DOMNode *attribute = 0;
  XMLCh *tmpStr = XMLString::transcode("MOOrdering");
  DOMNodeList *MO_Nodes = p_doc->getElementsByTagName(tmpStr);
  delete [] tmpStr;

  char *attributeName = (char*)0;
  char *attributeValue = (char*)0;
  string attributeString;
  bool foundBasis = false;
  bool foundShell;
  int numAttr,lshell;

  // Create a string with appropriate basisType
  string typestring;
  if (type == TGaussianBasisSet::Cartesian) {
    typestring = "cartesian";
  } else if (type == TGaussianBasisSet::Spherical) {
    typestring = "spherical";
  } else {
    //cout << "Unknown basis type in getAngFunc argument" << endl;
  }

  // Scan through MOOrdering fields and look for the one with the
  // correct basis set type.
  for (idx = 0; (MO_Nodes != 0) && idx < MO_Nodes->getLength() && !foundBasis; idx++) {
    MOOrder = (DOMElement *) MO_Nodes->item(idx);
    attributes = MOOrder->getAttributes();
    numAttr = attributes->getLength();
    for (adx = 0; adx < numAttr && !foundBasis; adx++) {
      attribute = attributes->item(adx);
      attributeName = XMLString::transcode(attribute->getNodeName());
      if (strcmp(attributeName, "type") == 0) {
        attributeValue = XMLString::transcode(attribute->getNodeValue());
        if (strcmp(attributeValue,typestring.c_str()) == 0) {
          foundBasis = true;
        }
        delete [] attributeValue;
      }
      delete [] attributeName;
    }
    // Found the correct MOOrdering node. Now construct angular momentum
    // template.
    if (foundBasis) {
      if (typestring=="cartesian") {
        ret->basisType(TGBSAngFunc::Cartesian);
      } else {
        ret->basisType(TGBSAngFunc::Spherical);
      }
      tmpStr = XMLString::transcode("lshell");
      DOMNodeList *shells = MOOrder->getElementsByTagName(tmpStr);
      delete [] tmpStr;

      int nshell = 0;
      if (shells != 0) nshell = shells->getLength();
      for (ldx = 0; ldx < nshell; ldx++) {

        // Look for shell corresponding to value L=ldx
        foundShell = false;
        MOShell = shells->item(ldx);
        attributes = MOShell->getAttributes();
        numAttr = attributes->getLength();
        for (adx = 0; adx < numAttr && !foundShell; adx++) {
          attribute = attributes->item(adx);
          attributeName = XMLString::transcode(attribute->getNodeName());
          if (strcmp(attributeName,"lval") == 0) {
            attributeValue = XMLString::transcode(attribute->getNodeValue());
            lshell = atoi(attributeValue);
            if(lshell == ldx) {
              foundShell = true;
            }
            delete [] attributeValue;
          }
          delete [] attributeName;
        }

        // Scan shell for MO orders
        if (foundShell) {
          vector<string> funcString(numAttr);
          int numString = -1;
          int idx;
          for (adx = 0; adx < numAttr; adx++) {
            attribute = attributes->item(adx);
            attributeName = XMLString::transcode(attribute->getNodeName());
            // Check to see if attributeName begins with 'a'
            if (tolower(attributeName[0]) == 'a') {
              attributeValue = XMLString::transcode(attribute->getNodeValue());
              attributeString = attributeValue;
              //Find index associated with attribute name (this is the integer
              //appended to the character 'a')
              idx = atoi(attributeName+1)-1;
              if (idx < numAttr-1) {
                funcString[idx] = attributeString;
              } else {
                EE_ASSERT(0, EE_FATAL, "Illegal attribute found in MOOrdering field");
              }
              if (idx+1 > numString)
                numString = idx+1;
              delete [] attributeValue;
            }
            delete [] attributeName;
          }
          for (idx = 0; idx < numString; idx++) {
            AMfunc = ret->parseString(funcString[idx]);
            ret->addFunc(AMfunc);
          }
          ret->markShell();
        } else {
          //cout << "Angular momentum ordering is incomplete" << endl;
        }
      }
    }
  }
  return ret;
}


/**
 * Retrieves the current set of geometry constraint rules
 * for a particular application code.  Rules are extracted
 * from the <GeometryConstraintRule> element.
 *
 * @return Returns a pointer to a GeomConstraintRules 
 *         object - caller is responsible for memory 
 *         management.
 */
GeomConstraintRules* JCode::getGeomConstraintRules() const
{
  GeomConstraintRules *rules = new GeomConstraintRules();
  bool isSupported;

  if (get_bool("SupportsAtomConstraints", isSupported)) {
    rules->setSupportsAtomConstraints(isSupported);
  }

  if (get_bool("SupportsConstrainBonds", isSupported)) {
    rules->setSupportsConstrainBonds(isSupported);
  }

  if (get_bool("SupportsConstrainAngles", isSupported)) {
    rules->setSupportsConstrainAngles(isSupported);
  }

  if (get_bool("SupportsConstrainTorsions", isSupported)) {
    rules->setSupportsConstrainTorsions(isSupported);
  }

  if (get_bool("SupportsRestrainBonds", isSupported)) {
    rules->setSupportsRestrainBonds(isSupported);
  }

  if (get_bool("SupportsRestrainAngles", isSupported)) {
    rules->setSupportsRestrainAngles(isSupported);
  }

  if (get_bool("SupportsRestrainTorsions", isSupported)) {
    rules->setSupportsRestrainTorsions(isSupported);
  }

  if (get_bool("SupportsFixedAtomsAndConstraints", isSupported)) {
    rules->setSupportsFixedAtomsAndConstraints(isSupported);
  }

  if (get_bool("SupportsFixedAtomsAndRestraints", isSupported)) {
    rules->setSupportsFixedAtomsAndRestraints(isSupported);
  }

  if (get_bool("SupportsMixedConstraintsAndRestraints", isSupported)) {
    rules->setSupportsMixedConstraintsAndRestraints(isSupported);
  }

  return rules;
}






