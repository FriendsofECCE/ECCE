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
#include "dsm/TGBSConfig.H"
#include "dsm/EDSIMessage.H"
#include "dsm/EDSI.H"
#include "dsm/EDSIFactory.H"
#include "dsm/Session.H"
#include "dsm/DirDyVTSTTask.H"
#include "dsm/CodeFactory.H"



/**
 * Constructor.
 */
DirDyVTSTTask::DirDyVTSTTask(const EcceURL& url):
  DavCalculation(url)
{}


/**
 * Destructor.
 */
DirDyVTSTTask::~DirDyVTSTTask()
{}


bool DirDyVTSTTask::fragment(Fragment *frag)
{
  return setFragment(frag, DirDyVTSTTask::TRANSITION);
}


bool DirDyVTSTTask::getFragment(Fragment &frag)
{
  Fragment *newfrag = fragment();
  if (newfrag) {
    frag = *newfrag;
    delete newfrag;
  }
  return true;
}


Fragment* DirDyVTSTTask::fragment() const
{
  return ((const_cast<DirDyVTSTTask*>(this))->getFragment(DirDyVTSTTask::TRANSITION));
}


/**
 * Gets the fragment for a component task.
 */
Fragment* DirDyVTSTTask::getFragment(DirDyVTSTTask::Component comp)
{
  Fragment *ret = 0;

  Session *session = getSession();

  if (session != 0) {

    // Get the component task and call its getFragment() method
    string linkName = DirDyVTSTTask::componentTypeToString(comp);
    DavCalculation *calc = 
      dynamic_cast<DavCalculation*>(getInputProvider(session, linkName));
    if (calc != 0) {
      // Get fragment and use last step of geomtrace, if any
      ret = calc->fragment();
      if (ret != 0) {
        calc->getFragmentStep(ret);
      }
    }
  }

  return ret;
}

string DirDyVTSTTask::getBasis(const string& theoryType) const
{
  string ret = "";
  TGBSConfig* const basis = gbsConfig(theoryType);
  if (basis != (TGBSConfig*)0) {
    // Produce code formatted basis set file
    const JCode* code = CodeFactory::lookup("NWChem");
    ostrstream stdStream;
    stdStream << basis->dump(code->name().c_str(), true) << ends;
    ret = stdStream.str();
  }
  return ret;
}


/**
 * Writes the indicated fragment for the component task.
 */
bool DirDyVTSTTask::setFragment(Fragment *fragment, 
                                DirDyVTSTTask::Component comp)
{
  bool ret = false;

  Session *session = getSession();

  if (session != 0) {

    // Get the component task and call its getFragment() method
    string linkName = DirDyVTSTTask::componentTypeToString(comp);
    ChemistryTask *calc = 
      dynamic_cast<ChemistryTask*>(getInputProvider(session, linkName));
    if (calc != 0) {
      ret = calc->fragment(fragment);
    }
  }

  return ret;

}


/**
 * Determines, by comparing the :lastmodified times of the input
 * and molecule file, if and of  the molecule file are new.  
 * 
 * @return Returns true if the :lastmodified time of any of the 
 *         molecule files is newer than the :lastmodified time
 *         of the input file.
 */
bool DirDyVTSTTask::anyFragmentsNew() const
{
  bool ret = false;
#if 000
  MetaDataRequest request;
  vector<MetaDataRequest> requests;
  vector<MetaDataResult> molecule_results;
  vector<MetaDataResult> infile_results;

  request.name = "DAV:getlastmodified";
  requests.push_back(request);

  getEDSI()->setURL(getVDoc()->getConfiguration(getVDoc()->getMoleculeName()));
  getEDSI()->getMetaData(requests, molecule_results);

  vector<EcceURL> inputfile = getDataFiles(JCode::INPUT);
  if (inputfile.size() > 0) {
    EcceURL iUrl = inputfile[0];
    getEDSI()->setURL(iUrl);
    getEDSI()->getMetaData(requests, infile_results);
  }

  if (molecule_results.size() > 0 &&
      infile_results.size() > 0) {
    long inSecs = TDateTime::toSeconds(infile_results[0].value.c_str());
    long molSecs = TDateTime::toSeconds(molecule_results[0].value.c_str());

    ret = molSecs>inSecs;
    if (ret)
      // DAV copy can result in input file being slightly older than molecule
      // and/or basis set.  However, they should all be very close in time so
      // check that the times are all within three seconds meaning that it
      // would be extremely unlikely if they weren't created by some kind of
      // copy operation.
      ret = molSecs-inSecs > 3;
  }

#endif
  return ret;
}

/**
 * Save the data and properties.
 * The url is presumed to have been set already.
 * @param data - the stream to fill up
 * @param props - set of properties to get; possibly none.  Retrieved only
 *   if the put succeeds.  values added to map
 */
bool DirDyVTSTTask::load(ostream& data, EcceMap& props) const
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
string DirDyVTSTTask::getParseScriptArgs() const
{
   return " .";
}


bool DirDyVTSTTask::canModifyInputProvider()
{ 
  return true;
}


string DirDyVTSTTask::import( const string& dir, const string& file)
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
bool DirDyVTSTTask::remove(void)
{
  bool ret = false;
  p_msgStack->clear();
  p_msgStack->add("UNABLE_TO_DELETE",
                  "\nDelete of a DirDyVTST Task is not supported!");
  return ret;
}

/**
 * Disable copy because it means nothing to copy over just 
 * DirDyVTST task
 */
bool DirDyVTSTTask::copy(EcceURL & target)
{
  bool ret = false;
  p_msgStack->clear();
  p_msgStack->add("UNABLE_TO_COPY",
                  "\nCopy of a DirDyVTST Task is not supported!");
  return ret;
}


/**
 * For DirDyVTST tasks, only moving the resource within the session is
 * allowed (ie rename).
 *
 * @param target New location of resource.
 * @return Returns true if resource was moved, otherwise fals.
 */
bool DirDyVTSTTask::move(EcceURL& target)
{
  bool ret = false;
  p_msgStack->clear();

  // Detect whether it is a rename
  if (target.getParent() != getURL().getParent()) {
    p_msgStack->add("UNABLE_TO_MOVE",
                    "\nMove of a DirDyVTST Task is not supported!");
  }
  else {
    Session * session = getSession();

    if (session != 0) {
      if (Resource::move(target)) {
        session->saveLinkbase();
        ret = true;
      }
      // Task is not in the session - go ahead and rename
      else {
        ret = Resource::move(target);
      }
    }
    // Task is not in a session - go ahead and rename
    else {
      ret = Resource::move(target);
    }
    setURL(target);
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
string DirDyVTSTTask::componentTypeToString(DirDyVTSTTask::Component comp)
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
DirDyVTSTTask::Component DirDyVTSTTask::stringToComponentType(const string& str)
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
 * Retrieves a component task providing input to this
 * DirDyVTST task.
 *
 * @return returns a zero pointer if the component task
 *         does not exist.
 *         User is responsible for memory management.
 */
DavCalculation *DirDyVTSTTask::getComponentTask(DirDyVTSTTask::Component comp)
{
  DavCalculation *ret = 0;

  Session *session = getSession();
  string linkName = componentTypeToString(comp);

  if (session != 0) {
    ret = dynamic_cast<DavCalculation*>(getInputProvider(session, linkName));
  }
  return ret;
}




