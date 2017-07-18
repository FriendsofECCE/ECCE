/**
 * @file
 *
 *
 */

#include <iostream>
  using std::cout;
  using std::cerr;
  using std::endl;
  using std::ends;

#include <fstream>
  using std::ifstream;

#include <strstream>
  using std::istrstream;
  using std::ostrstream;

#include <string>
  using std::string;
#include <vector>
  using std::vector;

#include "tdat/Fragment.H"

#include "dsm/TGaussianBasisSet.H"
#include "dsm/EDSIMessage.H"
#include "dsm/EDSI.H"
#include "dsm/EDSIFactory.H"
#include "dsm/Session.H"
#include "dsm/DirDyVTSTTask.H"
#include "dsm/PolyrateTask.H"


/**
 * Constructor.
 */
PolyrateTask::PolyrateTask(const EcceURL& url):
  DavCalculation(url)
{}


/**
 * Destructor.
 */
PolyrateTask::~PolyrateTask()
{}


/**
 * Save the data and properties.
 * The url is presumed to have been set already.
 * @param data - the stream to fill up
 * @param props - set of properties to get; possibly none.  Retrieved only
 *   if the put succeeds.  values added to map
 */
bool PolyrateTask::load(ostream& data, EcceMap& props) const
{
   bool ret = true;
#if 000
   if (!getEDSI()->getDataSet(data) ||
         getEDSI()->m_msgStack.getMessage().find("404") != string::npos) {
      ret = false;
   } else {
      data << ends;
      // now load metdata data if any was requested.
      if (props.size() > 0) {
         // The properties we want to get
         vector<MetaDataRequest> requests;

         // A single request - we set the name from props
         MetaDataRequest request;

         // What we got
         vector<MetaDataResult> results;


         // Make our request list
         EcceMapIterator it = props.begin();
         while (it != props.end()) {
            request.name = (*it).first;
            it++;
            requests.push_back(request);
         }

         // Now put them out there
         if (getEDSI()->getMetaData(requests, results)) {
            // finally, update the callers hash
            for (int idx=0; idx<results.size(); idx++) {
               props[results[idx].name] = results[idx].value;
            }
         } else {
            ret = false;
         }
      }
   }
#endif
   return ret;
}

/**
 * Return a string that represents the arguments that should be
 * sent to data parsing scripts.
 * By default an empty string is returned.  Subclasses should
 * override this method if their parse scripts take arguments.
 */
string PolyrateTask::getParseScriptArgs() const
{
  string basis = "none";
  string theorycat = "none";
  string theoryname = "none";
  string xcfunc = "none";

  Session *session = getSession();
  if (session != 0) {
    DirDyVTSTTask *dirdy = dynamic_cast<DirDyVTSTTask*>(getInputProvider(session));
    if (dirdy != (DirDyVTSTTask*)0) {
      TTheory* sptheory = dirdy->theory("single");
      string useTheory;
      if (sptheory==(TTheory*)0 || sptheory->category()=="None") {
        useTheory = "general";
        TTheory* gentheory = dirdy->theory(useTheory);
        if (gentheory != (TTheory*)0) {
          theorycat = gentheory->category();
          theoryname = gentheory->name();
        }
      } else {
        useTheory = "single";
        theorycat = sptheory->category();
        theoryname = sptheory->name();
      }

      if (theorycat == "DFT") {
        GUIValues* guiparams = dirdy->guiparams(useTheory);
        if (guiparams != (GUIValues*)0) {
          GUIValue* guivalue = guiparams->get("ES.Theory.DFT.XCFunctionals");
          if (guivalue != (GUIValue*)0) {
            xcfunc = guivalue->getValueAsString();
          }
        }
      }

      TGBSConfig* gbsConfig = dirdy->gbsConfig(useTheory);
      if (gbsConfig != (TGBSConfig*)0) {
        basis = gbsConfig->name();
      }
    }
  }

  string args = " . \"" + basis + "\" \"" + theorycat + "\" \"" + theoryname + "\" \"" + xcfunc + "\"";

   return args;
}


bool PolyrateTask::canModifyInputProvider()
{ 
  return true;
}


string PolyrateTask::import( const string& dir, const string& file)
  throw(EcceException)
{
  return "";
}


/**
 * Removes a resource and all subdirectories, if any, from the
 * Dav server.
 *
 * @return Returns true if resource was removed, otherwise false.
 */
bool PolyrateTask::remove(void)
{
  bool ret = false;
  p_msgStack->clear();
  Session *session = getSession();

  if (session != 0) {
    session->removeMember(this);
    session->saveLinkbase();
    if (Resource::remove()) {
      ret = true;
    }
  }

  return ret;
}

/**
 * Disable copy because it means nothing to copy over just 
 * Polyrate task
 */
bool PolyrateTask::copy(EcceURL & target)
{
  bool ret = false;
  p_msgStack->clear();
  p_msgStack->add("UNABLE_TO_COPY",
                  "\nCopy of a POLYRATE Task is not supported!");
  return ret;
}


/**
 * For Polyrate tasks, only moving the resource within the session is
 * allowed (ie rename).
 *
 * @param target New location of resource.
 * @return Returns true if resource was moved, otherwise fals.
 */
bool PolyrateTask::move(EcceURL& target)
{
  bool ret = false;
  p_msgStack->clear();

  // Detect whether it is a rename
  if (target.getParent() != getURL().getParent()) {
    p_msgStack->add("UNABLE_TO_MOVE",
                    "\nMove of a POLYRATE Task is not supported!");
  }
  else {

    // Get session and task model
    Session * session = getSession();

    if (session != 0) {
      if (Resource::move(target)) {
        session->saveLinkbase();
        setURL(target);
        ret = true;
      }
      else {
        ret = Resource::move(target);
      }
    }
    else {
      ret = Resource::move(target);
    }
  }

  return ret;
}


/**
 * Returns string value for a component type.  If the component type
 * is not recognized, an empty string is returned.
 *
 * @param comp the component type 
 * 
 * @return string value of component type.
 */
string PolyrateTask::componentTypeToString(DirDyVTSTTask::Component comp)
{
  string compStr = "";

  if (comp == DirDyVTSTTask::TRANSITION) {
    compStr = "Transition-State";
  }
  if (comp == DirDyVTSTTask::REACT1) {
    compStr = "Reactant1";
  }
  if (comp == DirDyVTSTTask::REACT2) {
    compStr = "Reactant2";
  }
  if (comp == DirDyVTSTTask::PROD1) {
    compStr = "Product1";
  }
  if (comp == DirDyVTSTTask::PROD2) {
    compStr = "Product2";
  }

  return compStr;
}


/**
 * Returns the type of component for a string value.  If the string value
 * does not map to a component type then the UNDEFINED component
 * type is returned.
 * 
 * @param str string value
 *
 * @return returns the type of component for a string value.
 */
DirDyVTSTTask::Component PolyrateTask::stringToComponentType(const string& str)
{
  DirDyVTSTTask::Component
    comp = DirDyVTSTTask::UNDEFINED_COMPONENT;

  if (str == "Transition-State") {
    comp = DirDyVTSTTask::TRANSITION;
  }
  if (str == "Reactant1") {
    comp = DirDyVTSTTask::REACT1;
  }
  if (str == "Reactant2") {
    comp = DirDyVTSTTask::REACT2;
  }
  if (str == "Product1") {
    comp = DirDyVTSTTask::PROD1;
  }
  if (str == "Product2") {
    comp = DirDyVTSTTask::PROD2;
  }
  else {
    comp = DirDyVTSTTask::UNDEFINED_COMPONENT;
  }

  return comp;
}


/**
 * Retrieves the DirDyVTST Task associated with this Polyrate task.
 * 
 * @return returns a zero pointer if the DirDyVTSTTask does
 *         not exist (this shouldn't happen).
 *         User is responsible for memory management.
 */
DirDyVTSTTask *PolyrateTask::getDirdyTask() 
{
  DirDyVTSTTask *ret = 0;

  Session *session = getSession();

  if (session != 0) {
    ret = dynamic_cast<DirDyVTSTTask*>(getInputProvider(session));
  }
  return ret;
}


