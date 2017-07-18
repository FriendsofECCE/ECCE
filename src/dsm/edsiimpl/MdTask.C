/**
* @file
*
*
*/

#include <strstream>
  using std::istrstream;
  using std::ostrstream;
  using std::ends;

#include "util/DavException.H"
#include "util/EcceURL.H"
#include "util/InvalidException.H"
#include "util/ResourceUtils.H"
#include "util/StringConverter.H"

#include "dsm/Job.H"
#include "dsm/NWChemMDModel.H"
#include "dsm/JCode.H"
#include "dsm/MdTask.H"
#include "dsm/ResourceType.H"
#include "dsm/Session.H"
#include "dsm/VDoc.H"



/** 
 * Constructor.  Only EDSIFactory can create an MdTask.  The supplied
 * Url identifies the storage location of the new MdTask.
 *
 * @param eurl Url address of new MdTask
 *
 * @throw InvalidException An exception is thrown if either the connection
 *        to the Dav server could not be established or the MdTask's 
 *        virtual document could not be created.
 */
MdTask::MdTask(const EcceURL& eurl) throw(InvalidException)  
{
  p_url = eurl;
  p_edsi = 0;
  p_vdoc = 0;
}


/**
 * Returns false if a fragment file does not exist, or there is a
 * problem with the URL on the Dav server. Starting
 * with the current task and traversing to each previous task,
 * each task is searched until the fragment is found or no more tasks 
 * remain to be searched.
 * 
 * 
 * @param frag Fragment reference
 *
 * @return Returns the Fragment via the frag reference.
 */
bool MdTask::getFragment(Fragment& frag)
{
  bool ret = false; 

  Session *session = getSession();
  if (session != 0) {
    MdTask *searchTask = this;
    while ( (!ret) && (searchTask != 0) ) {
      ret = searchTask->getTaskFragment(frag);
      
      // Check if found fragment, if not then set
      // search task to previous task.
      if (!ret) { 
        MdTask *newSearchTask = dynamic_cast<MdTask*>(searchTask->getInputProvider(session));
        searchTask = newSearchTask;
      }
    }
  }
  return ret;
}

/**
 * Returns false if a fragment file does not exist, or there is a
 * problem with the URL on the Dav server. Works in a similar manner
 * to get Fragment.
 * 
 * @param fragSum FragmentSummary reference
 *
 * @return Returns the FragmentSummary via the fragSum reference.
 */
bool MdTask::getFragmentSummary(FragmentSummary& fragSum)
{
  bool ret = false; 

  Session *session = getSession();
  if (session != 0) {
    MdTask *searchTask = this;
    while ( (!ret) && (searchTask != 0) ) {
      
      ret = searchTask->getTaskFragmentSummary(fragSum);
      
      // Check if found fragment, if not then set
      // search task to previous task.
      if (!ret) { 
        MdTask *newSearchTask = dynamic_cast<MdTask*>(searchTask->getInputProvider(session));
        searchTask = newSearchTask;
      }
    }
  }
  return ret;
}


/**
 * Remove the output fragment if it exists.
 * Quietly does nothing if the resource doesn't exist.
 */
void MdTask::removeOutputFragment()
{
  p_msgStack->clear();
  EcceURL url = getVDoc()->getOutput(getVDoc()->getOutputMoleculeName());
  getEDSI()->setURL(url);
  if (getEDSI()->exists()) {
     getEDSI()->removeResource();
  }
}


/**
 * Remove the input fragment if it exists.
 * Quietly does nothing if the resource doesn't exist.
 */
void MdTask::removeInputFragment()
{
  p_msgStack->clear();
  EcceURL url = getVDoc()->getConfiguration(getVDoc()->getInputMoleculeName());
  getEDSI()->setURL(url);
  if (getEDSI()->exists()) {
     getEDSI()->removeResource();
  }
}


/**
 * Remove the original fragment if it exists.
 * Quietly does nothing if the resource doesn't exist.
 */
void MdTask::removeOriginalFragment()
{
  p_msgStack->clear();
  EcceURL url = getVDoc()->getConfiguration(getVDoc()->getOriginalMoleculeName());
  getEDSI()->setURL(url);
  if (getEDSI()->exists()) {
     getEDSI()->removeResource();
  }
}


vector<string> MdTask::propertyNames()
{
  vector<string> ret = PropertyTask::propertyNames();
  
  // Only add the TRJ prop key if certain conditions apply...
  ResourceDescriptor::RUNSTATE state = getState();
  if (state >= ResourceDescriptor::STATE_RUNNING) {
    ResourceDescriptor::CONTENTTYPE type = getContentType();
    if (type == ResourceDescriptor::CT_MDOPTIMIZE ||
        type == ResourceDescriptor::CT_MDDYNAMICS) {
      // The logic to populate the model correctly is pretty tricky
      // Panels that are to be referenced must be "pushed" so the
      // model fields are populated.
      vector<NWChemMDModel::GUIPanel> panels;
      panels.push_back(NWChemMDModel::FILES);
      NWChemMDModel taskModel(panels);
      try {
         getTaskModel(taskModel);
         FilesModel *fmodel = taskModel.getFilesModel();
         if (fmodel && fmodel->getWriteTrajectory()) {
           ret.push_back("TRJ");
         }
      } catch (...) {
      }
    }
  }
 
  return ret;
}


/** 
 * Writes the indicated fragment to file on the dav server.
 * If frag is NULL, then it erases the fragment from the dav
 * server.
 *
 * For MD task, files are saved as PDB and if there is not yet an
 * original fragment, then the original fragment is set.  Otherwise the input
 * fragment is set.
 *
 * @param frag Fragment pointer
 *
 * @return Returns true if the fragment was successfully stored, 
 *         otherwise false is returned.
 * 
 */
bool MdTask::fragment(Fragment* frag)
{
  bool ret = true;
  p_msgStack->clear();

  // set url to original fragment
  string moleculeName = getVDoc()->getOriginalMoleculeName();
  // IF we have an original frag already, then we want to save edit frag
  if (hasOriginalFragment()) {
     moleculeName = getInputFragmentName();
  }
  EcceURL moleculeUrl;

  if (frag && frag->numAtoms()>0) { // save fragment to file
    // First create the file:
    ostrstream os;
    frag->dumpPDB(os, false);
    os << ends;
    string moleculeData = os.str();

    // Save fragment
    try {
      istrstream is(moleculeData.c_str());
      moleculeUrl = getVDoc()->addConfiguration(moleculeName, &is);
      getEDSI()->setURL(moleculeUrl);

      // Save some metadata for MD fragments
      vector<MetaDataResult> results(7);

      int n = 0;
      // numAtoms property
      results[n].name = VDoc::getEcceNamespace() + ":numAtoms";
      results[n].value = StringConverter::toString(frag->numAtoms());
      n++;

      // numElectrons property
      results[n].name = VDoc::getEcceNamespace() + ":numElectrons";
      results[n].value = StringConverter::toString(frag->numElectrons());
      n++;

      // numResdiues property
      results[n].name = VDoc::getEcceNamespace() + ":numResidues";
      results[n].value = StringConverter::toString(frag->numResidues());
      n++;

      //?? Should we only do this for original frag??
      results[n].name = VDoc::getEcceNamespace() + ":selectedModel";
      results[n].value = "1";
      n++;

      results[n].name = VDoc::getEcceNamespace() + ":alternateLocation";
      results[n].value = " ";
      n++;

      results[n].name = VDoc::getEcceNamespace() + ":chainId";
      results[n].value = " ";
      n++;

      // fragmentName property
      results[n].name = VDoc::getEcceNamespace() + ":fragmentName";
      results[n].value = frag->name();
      n++;


      if (!getEDSI()->putMetaData(results)) {
        p_msgStack->add(getEDSI()->m_msgStack);
        ret = false;
      }

      // Need to update properties for this
      // calc to keep the cache in sync.  This
      // is necessary because the properties are
      // being stored on the molecule file and
      // not on the calc.  If the proeprties were
      // being stored on the calc then would of
      // just used an addProps() call, which would
      // of updated the cache for us.
      updateProps();

    }
    catch (DavException& davException) {
      p_msgStack->add("UNABLE_TO_COMPLETE_REQUEST",
                      davException.what());
      ret = false;
    }
  }
  return ret;
}


bool MdTask::hasFragment()
{
  bool ret = false;
  EcceURL url;

  Session *session = getSession();
  if (session != 0) {
    MdTask *searchTask = this;
    while ( (!ret) && (searchTask != 0) ) {

      // Check for ouput fragment
      url = searchTask->getVDoc()->getOutput(getOutputFragmentName());
      getEDSI()->setURL(url); 
      ret = (getEDSI()->getDataSetSize() > 0);

      // Check for input fragment 
      if (!ret) {
        url = searchTask->getVDoc()->getConfiguration(getInputFragmentName());
        getEDSI()->setURL(url); 
        ret = (getEDSI()->getDataSetSize() > 0);
      }

      // Check for Original fragment 
      if (!ret) {
        url = searchTask->getVDoc()->getConfiguration(getOriginalFragmentName());
        getEDSI()->setURL(url); 
        ret = (getEDSI()->getDataSetSize() > 0);
      }

      // Check if found fragment, if not then set
      // search task to previous task.
      if (!ret) { 
        MdTask *newSearchTask = dynamic_cast<MdTask*>(searchTask->getInputProvider(session));
        searchTask = newSearchTask;
      }
    }
  }

  return ret;
}


/**
 * Gets the fragment for a task.
 * Returns false if a fragment file does not exist, or there is a
 * problem with the URL on the Dav server.  The fragment file is
 * searched for in the following order: (1) Output fragment, 
 * (2) Input/Modified fragment file, (3) Original fragment.
 * 
 * @param frag Fragment reference
 *
 * @return Returns the Fragment via the frag reference.
 */
bool MdTask::getTaskFragment(Fragment& frag)
{
  bool ret = false;
  istream *is = 0;

  // Get output fragment, if exists
  is = getDataFile(JCode::FRAGMENT_OUTPUT);
  if (is) {
    ret = getOutputFragment(is, frag);
  }
  // Output Fragment not found look for input
  else {
    EcceURL url = getVDoc()->getConfiguration(getInputFragmentName());
    getEDSI()->setURL(url);
    if (getEDSI()->exists()) {
       ret = getFragment(frag, url);
    }
    // Input Fragment not found look for original fragment
    else {
      ret = getOriginalFragment(frag);
    }
  } 
  return ret;
}

/**
 * Gets the fragment summary for a task. Similar to getTaskFragment.
 * 
 * @param fragSum FragmentSummary reference
 *
 * @return Returns the FragmentSummary via the fragSum reference.
 */
bool MdTask::getTaskFragmentSummary(FragmentSummary& fragSum)
{
  bool ret = false;

  // Get output fragment, if exists
  EcceURL url = getVDoc()->getConfiguration(getOutputFragmentName());
  getEDSI()->setURL(url);
  if (getEDSI()->exists()) {
    ret = getFragmentSummary(fragSum,url); 
  } else {
    EcceURL url = getVDoc()->getConfiguration(getInputFragmentName());
    getEDSI()->setURL(url);
    if (getEDSI()->exists()) {
      ret = getFragmentSummary(fragSum,url); 
    } else {
      // Set Url to Original fragment file
       url = getVDoc()->getConfiguration(getVDoc()->getOriginalMoleculeName());
       ret = getFragmentSummary(fragSum,url); 
    }
  } 
  return ret;
}


/**
 * Writes the indicated original source fragment to file on the dav server.
 * If frag is NULL, then it erases the fragment from the dav
 * server.  PDB format is assumed.
 *
 * @param frag Fragment pointer
 *
 * @return Returns true if the fragment was successfully stored,
 *         otherwise false is returned.
 *
 * @todo Need to augment the Fragment class to put
 *       the state information requested in the Fragment class.
 *       Need logic to get all Molecule formats...
 */
bool MdTask::setOriginalFragment(istream* is,
                                 int selectedModel,
                                 const string& alternateLocation,
                                 const string& chainId,
                                 const string& name)
{
  bool ret = false;
  p_msgStack->clear();

  if (is != 0) {

    // set url to molecule
    string moleculeName = getVDoc()->getOriginalMoleculeName();
    EcceURL moleculeUrl;

    try {
      moleculeUrl = getVDoc()->addConfiguration(moleculeName, is);
      getEDSI()->setURL(moleculeUrl);

      vector<MetaDataResult> results(4);
      results[0].name = VDoc::getEcceNamespace() + ":selectedModel";
      char temp[10];
      sprintf(temp, "%d", selectedModel);
      results[0].value = temp;

      results[1].name = VDoc::getEcceNamespace() + ":alternateLocation";
      results[1].value = alternateLocation;

      results[2].name = VDoc::getEcceNamespace() + ":chainId";
      results[2].value = chainId;

      if (name != "") {
         results[3].name = VDoc::getEcceNamespace() + ":fragmentName";
         results[3].value = name;
      }

      if (!getEDSI()->putMetaData(results)) {
        p_msgStack->add(getEDSI()->m_msgStack);
      }
      else {
        ret = true;
      }
    }
    catch (DavException& davException) {
      p_msgStack->add("UNABLE_TO_COMPLETE_REQUEST",
                      davException.what());
    }

  }

  return ret;
}


/**
 * Determines if an original fragment exists.
 */
bool MdTask::hasOriginalFragment()
{
  p_msgStack->clear();
  bool ret = false;

  EcceURL url = getVDoc()->getConfiguration(getVDoc()->getOriginalMoleculeName());
  getEDSI()->setURL(url);

  ret = getEDSI()->exists();

  return ret;
}


/**
 * Determines if an input fragment exists.
 */
bool MdTask::hasInputFragment()
{
  p_msgStack->clear();
  bool ret = false;

  EcceURL url = getVDoc()->getConfiguration(getVDoc()->getInputMoleculeName());
  getEDSI()->setURL(url);

  ret = getEDSI()->exists();

  return ret;
}


/**
 * Original source fragment is saved to frag reference.
 * Returns false on error.  (Not all fragment data may get set
 * if some properties are missing.).  It is assumed that
 * the original fragment file is PDB format.
 *
 * @todo  Need to augment the Fragment class to put
 *        the state information requested in the Fragment class.
 *        Need logic to get all Molecule formats...
 *
 * @return Returns true if fragment is retrieved, otherwise
 *         false is returned.
 */
bool MdTask::getOriginalFragment(Fragment& frag)
{
   // Set Url to Original fragment file
   EcceURL url = getVDoc()->getConfiguration(getVDoc()->getOriginalMoleculeName());
   return getFragment(frag, url);

}

/**
 * This convenience method gets an MD fragment given the url.  It can
 * be used to get any of the input, output, or original fragment
 */
bool MdTask::getFragment(Fragment& frag, const EcceURL& url)
{
   p_msgStack->clear();

   bool ret = false;

   getEDSI()->setURL(url);

   if (getEDSI()->exists()) {

      vector<MetaDataRequest> requests(4);
      int n=0;
      requests[n++].name = VDoc::getEcceNamespace() + ":selectedModel";
      requests[n++].name = VDoc::getEcceNamespace() + ":alternateLocation";
      requests[n++].name = VDoc::getEcceNamespace() + ":chainId";
      requests[n++].name = VDoc::getEcceNamespace() + ":fragmentName";

      vector<MetaDataResult> results;
      if (!getEDSI()->getMetaData(requests, results)) {
         p_msgStack->add(getEDSI()->m_msgStack);
      }
      else {
         int selectedModel = 1;
         string alternateLocation = " ";
         string chainId = " ";
         string fragName;

         // Get metadata values
         vector<MetaDataResult>::const_iterator resultsIt = results.begin();
         while (resultsIt != results.end()) {
            if ((*resultsIt).name == VDoc::getEcceNamespace() + ":selectedModel") {
               if ( !(*resultsIt).value.empty() ) { 
                  sscanf((*resultsIt).value.c_str(),"%d",&selectedModel);
               }
            } 
            else if ((*resultsIt).name == VDoc::getEcceNamespace() + ":fragmentName") {
               if ( !(*resultsIt).value.empty() ) { 
                  fragName = (*resultsIt).value;
               }
            }
            else if ((*resultsIt).name == VDoc::getEcceNamespace() + ":alternateLocation") {
               if ( !(*resultsIt).value.empty() ) { 
                  alternateLocation = (*resultsIt).value;
               }
            }
            else if ((*resultsIt).name == VDoc::getEcceNamespace() + ":chainId") {
               if ( !(*resultsIt).value.empty() ) { 
                  chainId = (*resultsIt).value;
               }
            }
            resultsIt++;
         }

         istream *is = getEDSI()->getDataSet();
         if (is) {
            ret = frag.restorePDB(*is, 
                  1.0, 
                  true,
                  selectedModel,
                  alternateLocation, 
                  true, 
                  false, 
                  chainId);

            // initialize symmetry flag because it isn't part of the v2.0 schema
            // and we want this upgrade to be handled transparently
            frag.useSymmetry(false);
            if (fragName != "") frag.name(fragName);
         }
         else { 
            p_msgStack->add(getEDSI()->m_msgStack);
         }
      }
   }

   return ret;
}



/**
 * Returns a pointer to the input fragment.
 * The input fragment will be used if it exists, else the original fragment.
 * The input fragment SHOULD exist though.
 * 
 * @todo traverse tasks to obtain source fragment file input
 *       stream.
 */
istream *MdTask::getInputFragmentAsFile()
{
  istream *ret = 0;

  EcceURL url = getVDoc()->getConfiguration(getInputFragmentName());
  getEDSI()->setURL(url);
  if (getEDSI()->exists()) {
    ret = getEDSI()->getDataSet();
  }

  return ret;

}


/**
 * Original source fragment is saved to fragment summary reference.
 * Returns false on error. It is assumed that
 * the original fragment file is PDB format.
 * @param fragSum - object to be filled from pdb file
 * @param url - url to pdb file; could be any of orig, input, output frags
 *
 * @todo should this include the fragment name?
 * @return Returns true if fragment summmary is retrieved, otherwise
 *         false is returned.
 */
bool MdTask::getFragmentSummary(FragmentSummary& fragSum, const EcceURL& url)
{
   p_msgStack->clear();
   bool ret = false;

   getEDSI()->setURL(url);

   if (getEDSI()->exists()) {

      // Construct fragment summary from PDB file.
      istream *is = getEDSI()->getDataSet();
      if (is) {
         ret = fragSum.scanPDB(*is);
         if (ret) {
            ret = getOriginalFragmentProperties(fragSum);
         }
      }
      else { 
         p_msgStack->add(getEDSI()->m_msgStack);
      }
   }

   return ret;
}

bool MdTask::getOriginalFragmentProperties(FragmentSummary& fragSum)
{
   bool ret = true;

   EcceURL url = getVDoc()->getConfiguration(getVDoc()->getOriginalMoleculeName());
   getEDSI()->setURL(url);

   vector<MetaDataRequest> requests(4);
   int n=0;
   string selectedModelName = VDoc::getEcceNamespace() + ":selectedModel";
   string fragmentName = VDoc::getEcceNamespace() + ":fragmentName";
   string alternateLocationName = VDoc::getEcceNamespace() + ":alternateLocation";
   string chainIdName = VDoc::getEcceNamespace() + ":chainId";
   requests[n++].name = selectedModelName;
   requests[n++].name = fragmentName;
   requests[n++].name = alternateLocationName;
   requests[n++].name = chainIdName;
   vector<MetaDataResult> results;
   if (!getEDSI()->getMetaData(requests, results)) {
      p_msgStack->add(getEDSI()->m_msgStack);
   } else {
      int selectedModel = 1;
      string alternateLocation = " ";
      string chain = " ";
      string fragName = "";

      // Get metadata values
      vector<MetaDataResult>::const_iterator resultsIt = results.begin();
      while (resultsIt != results.end()) {
         if ((*resultsIt).name == VDoc::getEcceNamespace() + ":selectedModel") {
            if ( !(*resultsIt).value.empty() ) { 
               sscanf((*resultsIt).value.c_str(),"%d",&selectedModel);
            }
         } 
         else if ((*resultsIt).name == VDoc::getEcceNamespace() + ":fragmentName") {
            if ( !(*resultsIt).value.empty() ) { 
               fragName = (*resultsIt).value;
            }
         }
         else if ((*resultsIt).name == VDoc::getEcceNamespace() + ":alternateLocation") {
            if ( !(*resultsIt).value.empty() ) { 
               alternateLocation = (*resultsIt).value;
            }
         }
         else if ((*resultsIt).name == VDoc::getEcceNamespace() + ":chainId") {
            if ( !(*resultsIt).value.empty() ) { 
               chain = (*resultsIt).value;
            }
         }
         resultsIt++;
      }

      fragSum.setSelectedModel(selectedModel);
      fragSum.setAltLocation(alternateLocation);
      fragSum.setChain(chain);
      fragSum.setName(fragName);
   }

   // If fragmentName exists on input fragment - then use it
   vector<MetaDataRequest> namerequest(1);
   n=0;
   fragmentName = VDoc::getEcceNamespace() + ":fragmentName";
   namerequest[n++].name = fragmentName;
   vector<MetaDataResult> nameresults;
   url = getVDoc()->getConfiguration(getInputFragmentName());
   getEDSI()->setURL(url);
   if (getEDSI()->exists()) {
     if (!getEDSI()->getMetaData(namerequest, nameresults)) {
        p_msgStack->add(getEDSI()->m_msgStack);
     } else {
        string fragName = "";

        // Get metadata values
        vector<MetaDataResult>::const_iterator resultsIt = nameresults.begin();
        while (resultsIt != nameresults.end()) {
           if ((*resultsIt).name == VDoc::getEcceNamespace() + ":fragmentName") {
              if ( !(*resultsIt).value.empty() ) { 
                 fragName = (*resultsIt).value;
              }
           }
           resultsIt++;
        }

        // override the orginal name if the user has updated it
        if (fragName != "")
          fragSum.setName(fragName);
     }
   }

   return ret;
}


/**
 * Get system name.  Used as the value for the system in nwchem's 
 * input file.  The name should only be used for this purpose.
 *
 * @return returns the system name of an MdTask for use in NWChem's
 *         input file.
 *         
 */
string MdTask::getSystemName() const throw(InvalidException, DavException)
{
  return ResourceUtils::contentTypeToString(ResourceDescriptor::CT_SESSION);
}


/**
 * Get calc name.  Used as the value for calc in nwchem's 
 * input file. The name should only be used for this purpose.
 * 
 * @return returns the calc name of an MdTask for use in NWChem's
 *         input file.
 */
string MdTask::getCalcName() const
{
  return getDescriptor()->getContentType();
}


/**
 * Get original fragment file name.
 *
 * @return returns the name of the original fragment file for an MdTask.
 */
string MdTask::getOriginalFragmentName() const
{
  return getVDoc()->getOriginalMoleculeName();
}


/**
 * Get Output fragment file name.
 *
 * @return returns the name of the output fragment file generated
 *         by the MdTask.
 */
string MdTask::getOutputFragmentName() const
{
  return getVDoc()->getOutputMoleculeName();
}

/**
 * Get Input fragment file name.
 *
 * @return returns the name of the output fragment file generated
 *         by the MdTask.
 */
string MdTask::getInputFragmentName() const
{
  return getVDoc()->getInputMoleculeName();
}


/**
 * Get the output restart file name.  There are two types of restart 
 * files that may be output by an MdTask,
 * (1) an rst file and (2) a qrs file which are 
 * only generated by an energy minimization (optimization) task.
 * If the MdTask is an optimize task then the qrs name is returned
 * otherwise the rst name is returned.
 *
 * @throw InvalidException the getSystemName call may throw this exception
 * @throw DavException the getSystemName call may throw this exception
 *
 * @return returns the name of the output restart file name
 */
string MdTask::getRestartName() const throw(InvalidException, DavException)
{
  string restartName = getSystemName() + "_" + getCalcName();

  ResourceDescriptor::CONTENTTYPE ct = getContentType();
  if (ct == ResourceDescriptor::CT_MDOPTIMIZE) {
    restartName += ".qrs";
  }
  else {
    restartName += ".rst";
  }

  return restartName;
}


/**
 * Get Toplogy file name. Cannot set directly because it is
 * derived from the system name.  The topology file name
 * is the system name with a ".top" extension.
 *
 * @throw InvalidException the getSystemName call may throw this exception
 * @throw DavException the getSystemName call may throw this exception
 *
 * @return returns the topology file name that is generated
 *         by an MD prepare task.
 */
string MdTask::getTopologyName() const throw(InvalidException, DavException)
{
  return getSystemName() + ".top";
}


/**
 * Get MD output file name.  This is the name of the output
 * file generated by an MdTask.
 *
 * @return returns the name of the MD output file generated
 *         by the MdTask.
 */
string MdTask::getMdOutputName() const
{
  return (getSystemName() + "_" + getCalcName() + ".out");
}


/**
 * Gets MdTask that provides the Pdb (output fragment) as
 * input to the current task.  If there is no PDB provider
 * task then a 0 pointer will be returned.  Caller is
 * responsible for memory management.
 *
 * @return returns a pointer to the MD task that provides
 *         the Pdb (output fragment) as input to the 
 *         current task.
 */
MdTask *MdTask::getPdbProvider()
{
  MdTask *pdbProvider = 0;
  MdTask *searchTask = 0;
  Session *session = getSession();
  if (session != 0) {
    bool finishedSearch = false;
    searchTask = this; 
    while (!finishedSearch && searchTask != 0) {
      pdbProvider = dynamic_cast<MdTask*>(searchTask->getInputProvider(session));
      if (pdbProvider == 0) {
        finishedSearch = true;
      }
      else {
        if (pdbProvider->getDataFiles(JCode::FRAGMENT_OUTPUT).size() > 0) {
          finishedSearch = true;
        }
        else {
          searchTask = pdbProvider;
          pdbProvider = 0;
        }
      }
    }
  }

  return pdbProvider;
}


/**
 * Indicates if the Task's input provider (i.e. other Tasks 
 * within a Session) can be modified/removed. 
 * 
 * @return bool indicates that the Task's input providers may 
 *         be altered within a Session
 *
 * @todo Need to implement - How will this be determined?
 */
bool MdTask::canModifyInputProvider()
{

  return true;
}


/**
 * Indicates if the Task supports PMF calculation options.
 * Searches through self and input provider properties.
 *
 * @return bool indicates PMF is supported
 */
bool MdTask::hasPmf() const
{
  return getProp(VDoc::getEcceNamespace() + ":hasPmf", true) != "";
}



/**
 * Restores the output fragment - assumption is that the istream
 * is a pdb file.
 * 
 * @param is input stream
 * @param frag fragment reference 
 *
 * @return Returns true if the PDB restore was successful
 * 
 * @todo Need to augment the Fragment class to put
 *       the state information requested in the Fragment class.
 *       Need logic to get all Molecule formats...
 */
bool MdTask::getOutputFragment(istream *is, Fragment& frag) const
{
  bool ret = frag.restorePDB(*is);

  // initialize symmetry flag because it isn't part of the v2.0 schema
  // and we want this upgrade to be handled transparently
  frag.useSymmetry(false);

  return ret;
}


/**
 * Restores the output fragment summary - assumption is that the istream
 * is a pdb file.
 * The fragment summary from an output will NOT have import selections like
 * model, chain, altloc.
 * 
 * @param is input stream
 * @param fragSum fragment summary reference 
 *
 * @return Returns true if the PDB restore was successful
 */
bool MdTask::getOutputFragmentSummary(istream *is, FragmentSummary& fragSum) const
{
  bool ret = fragSum.scanPDB(*is);

  return ret;
}


/**
 * Recursively remove this task and all the the targets of this task.
 */
bool MdTask::remove()
{
  p_msgStack->clear();
  bool ret = true;

  // Get session
  Session * session = getSession();

  if (session != 0) {
    // Recursively delete all the targets of this task and itself
    ret = remove(session, true);
  }
  else {
    ret = Resource::remove();
  }

  return ret;
}


/**
 * Remove current task from task model and server, if deleteTarget is true,
 * also delete current task's targets from both task model and server.
 * Please make sure current task is contained in the taskModel.
 */
bool MdTask::remove(Session * session, bool deleteTarget)
{
  // Go through each target and call remove function on them
  if (deleteTarget) {
    vector<Resource *> targets = session->getTargets(this);
    vector<Resource *>::iterator target = targets.begin();
    for(; target != targets.end(); target++) {
      if (!(dynamic_cast<MdTask *>(*target)->remove(session, deleteTarget))){
        break;
      }
    }
    if (target != targets.end())
      return false;
  }

  // Remove current task from taskmodel
  session->removeMember(this);
  session->saveLinkbase();

  // Remove current task from server
  return Resource::remove();
}


/**
 * Moves a resource to a different location on the Dav server.
 * For now only allow move inside same directory which is just a rename.
 *
 * @param target New location of resource.
 * @return Returns true if resource was moved, otherwise fals.
 */
bool MdTask::move(EcceURL& target)
{
  bool ret = false;
  p_msgStack->clear();

  // Detect whether it is a rename
  if (target.getParent() != getURL().getParent()) {
    p_msgStack->add("UNABLE_TO_MOVE",
                    "\nMove of a MD Task is not supported!");
  }
  else {

    // Get session
    Session * session = getSession();

    ret = Resource::move(target);

    // Save linkbase
    if (session != 0) {
      session->saveLinkbase();
    }
  }

  return ret;

}


/**
 * Disable copy because it means nothing to copy over just a MdTask
 */
bool MdTask::copy(EcceURL & target)
{
  p_msgStack->clear();
  p_msgStack->add("UNABLE_TO_COPY",
                  "\nCopy of a MD Task is not supported!");
  return false;
}


void MdTask::getDeleteMessage(bool & canDelete, string & message)
{
  vector<Resource *> targets = 
    getSession()->getTargets(this, Session::INPUT_PROVIDER);
  vector<Resource *>::iterator targetItor = targets.begin();
  bool hasFollower = false;
  for (; targetItor != targets.end(); targetItor++) {
    hasFollower = true;
    (*targetItor)->getDeleteMessage(canDelete, message);
    if (!canDelete) {
      message = "At least one subsequent task in the workflow is in"
        " the submitted or running state.\nDelete operation is not allowed.";
      return;
    }
  }

  TaskJob::getDeleteMessage(canDelete, message);
  if (canDelete) {
    if (hasFollower)
      message = "Deleting task \"" + getName() +
        "\" will also delete any subsequent tasks in the workflow."
        "\n\nAre you sure you want to delete " + getName() + 
        " and any subsequent tasks?";
    else
      message = "Are you sure you want to delete task \"" + getName()
        + "\"?";
  }
}


bool MdTask::canChangeState(string & message) const
{
  message = "";
  if (!TaskJob::canChangeState(message)) {
    message = "Changing the state of a submitted or running task is not allowed.";
    return false;
  }

  vector<Resource *> targets = 
          getSession()->getTargets(this, Session::INPUT_PROVIDER);
  vector<Resource *>::iterator targetItor = targets.begin();
  for (; targetItor != targets.end(); targetItor++) {
    Job *job = dynamic_cast<Job*>(*targetItor);
    if (job && !job->canChangeState(message)) {
      message = "Changing the state of the current task is not allowed since a"
                " subsequent task is still in the submitted or running state.";
      return false;
    }
  }
  return true;
}


bool MdTask::resetForRerun(const ResourceDescriptor::RUNSTATE& tostate)
{
  bool ret = PropertyTask::resetForRerun(tostate);
  vector<Resource *> targets = 
          getSession()->getTargets(this, Session::INPUT_PROVIDER);
  vector<Resource *>::iterator targetItor = targets.begin();
  for (; targetItor != targets.end(); targetItor++) {
    Job *job = dynamic_cast<Job*>(*targetItor);
    if (job)
      ret = job->resetForRerun(tostate) && ret;
  }
  return ret;
}


bool MdTask::resetForRestart()
{
  bool ret = TaskJob::resetForRestart();
  vector<Resource *> targets = 
          getSession()->getTargets(this, Session::INPUT_PROVIDER);
  vector<Resource *>::iterator targetItor = targets.begin();
  for (; targetItor != targets.end(); targetItor++) {
    Job *job = dynamic_cast<Job*>(*targetItor);
    if (job)
      ret = job->resetForRerun() && ret;
  }
  return ret;
}
