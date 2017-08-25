/**
 * @file
 *
 *
 */

#include <fstream>
  using std::ifstream;

#include <strstream>
  using std::istrstream;
  using std::ostrstream;
  using std::ends;

#include <string>
  using std::string;
#include <vector>
  using std::vector;


#include "util/TDateTime.H"
#include "util/EcceMap.H"
#include "util/Serializer.H"
#include "util/StringConverter.H"
#include "tdat/Fragment.H"
#include "tdat/GeomConstraint.H"
#include "tdat/MetaPotential.H"
#include "tdat/QMMMParams.H"
#include "dsm/EDSIFactory.H"
#include "dsm/ESPConstraintModelXMLizer.H"
#include "dsm/GeomConstraintModelXMLizer.H"
#include "dsm/MetaPotentialModelXMLizer.H"
#include "dsm/QMMMModelXMLizer.H"
#include "dsm/Session.H"
#include "dsm/VDoc.H"
#include "dsm/ChemistryTask.H"

/**
 * Constructor.
 */
ChemistryTask::ChemistryTask()
  : TaskJob()
  , p_atomTypes()
{
}


/**
 * Destructor.
 */
ChemistryTask::~ChemistryTask()
{
}


/** 
 * Writes the indicated fragment to file on the dav server.
 * If frag is NULL, then it erases the fragment from the dav
 * server.
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
bool ChemistryTask::fragment(Fragment* frag)
{
  bool ret = true;
  p_msgStack->clear();

  // set url to molecule
  string moleculeName = getVDoc()->getMoleculeName();
  EcceURL moleculeUrl;

  Resource *parent = EDSIFactory::getResource(getURL().getParent());

  if (frag && frag->numAtoms()>0) { // save fragment to file
    // STTR logic first
    if (parent->getApplicationType() == ResourceDescriptor::AT_REACTION_STUDY) {
      if (getName() == "Transition-State") {
        // for the transition state, do atom typing to uniquely
        // identify each atom
        frag->setReactionIndices();
      } else {
        // for reactants and products, only do atom typing if no atom
        // types are previously set.  This allows the user to import
        // a fragment that maps directly on the transition state, but
        // is very dangerous otherwise
        frag->setReactionIndicesIfUnset();
      }
      // initialize atom type vector for reaction studies
      p_atomTypes.clear();
      vector<TAtm*> *atoms = frag->atoms();
      for (size_t it=0; it<frag->numAtoms(); it++) {
        p_atomTypes.push_back((*atoms)[it]->atomType());
      }
    }

    // Next create the file:
    ostrstream os;
    frag->dumpMVM(os, true);  // dump all.  We needed to do this to save
                              // our display style attributes
    os << ends;
    string moleculeData = os.str();

    // Save fragment
    try {
      istrstream is(moleculeData.c_str());
      moleculeUrl = getVDoc()->addConfiguration(moleculeName, &is);
      getEDSI()->setURL(moleculeUrl);

      vector<MetaDataResult> results(7);

      // empiricalFormula property
      results[0].name = VDoc::getEcceNamespace() + ":empiricalFormula";
      frag->generateEmpiricalFormula();
      results[0].value = frag->formula();

      // charge property
      results[1].name = VDoc::getEcceNamespace() + ":charge";
      char buf[24];
      sprintf(buf,"%d",frag->charge());
      results[1].value = buf;

      // symmetrygroup property
      results[2].name = VDoc::getEcceNamespace() + ":symmetrygroup";
      results[2].value = frag->pointGroup();

      // useSymmetry property
      results[3].name = VDoc::getEcceNamespace() + ":useSymmetry";
      results[3].value = frag->useSymmetry()? "true": "false";

      // numAtoms property
      results[4].name = VDoc::getEcceNamespace() + ":numAtoms";
      results[4].value = StringConverter::toString(frag->numAtoms());

      // numElectrons property
      results[5].name = VDoc::getEcceNamespace() + ":numElectrons";
      results[5].value = StringConverter::toString(frag->numElectrons());

      // fragmentName property
      results[6].name = VDoc::getEcceNamespace() + ":fragmentName";
      results[6].value = frag->name();

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

  if (ret) {
    if (parent->getApplicationType()!=ResourceDescriptor::AT_REACTION_STUDY &&
        parent->getApplicationType()!=ResourceDescriptor::AT_CONDENSED_REACTION_STUDY) {
      setState(ResourceDescriptor::STATE_CREATED);
    } else {
      if (getApplicationType()!=ResourceDescriptor::AT_DRDVTST &&
          getApplicationType()!=ResourceDescriptor::AT_POLYRATE &&
          getApplicationType()!=ResourceDescriptor::AT_SOLVATE &&
          getApplicationType()!=ResourceDescriptor::AT_METADYNAMICS) {
        setState(ResourceDescriptor::STATE_CREATED);
      }
      resetReactionTasks();
    }
  }

  return ret;
}


bool ChemistryTask::hasFragment()
{
  bool ret = false;

  EcceURL url = getVDoc()->getConfiguration(getVDoc()->getMoleculeName());
  getEDSI()->setURL(url);

  if (getEDSI()->getDataSetSize() > 0) {
    ret = true;
  }

  return ret;
}


/**
 * Returns false if the molecule file does not exist, there is a
 * problem with the URL on the Dav server,  or if the
 * molecule type is not "mvm".  If some properties do not exist for
 * the molecule, they will be left with their default value.
 * 
 * @param frag Fragment reference
 *
 * @return Returns the Fragment via the frag reference.
 */
bool ChemistryTask::getFragment(Fragment& frag, const bool& taskCheck) 
{
  p_msgStack->clear();
  bool ret = true;

 
  ///////
  // set url to molecule
  ///////

  EcceURL url = getVDoc()->getConfiguration(getVDoc()->getMoleculeName());
  getEDSI()->setURL(url);

  ///////
  //TODO:  Need to augment the Fragment class to put
  //       the state information requested in the Fragment class.
  //       Need logic to get all Molecule formats...
  ///////
  istream *is = getEDSI()->getDataSet();
  if (is)
    frag.restoreMVM(*is,1.0,false);
  else if (taskCheck) { // getDataSet failed
    // If this is a session based task, try to retrieve a chemical system from
    // a previous task
    ret = getTaskFragment(frag);

    if (!ret) {
      p_msgStack->add(getEDSI()->m_msgStack);
    }
  } else {
    p_msgStack->add(getEDSI()->m_msgStack);
    ret = false;
  }


  // initialize symmetry flag because it isn't part of the v2.0 schema
  // and we want this upgrade to be handled transparently
  frag.useSymmetry(false);

  string chargeStr = getProp(VDoc::getEcceNamespace() + ":charge");
  string symGrpStr = getProp(VDoc::getEcceNamespace() + ":symmetrygroup");
  string useSymStr = getProp(VDoc::getEcceNamespace() + ":useSymmetry");

  if (! chargeStr.empty()) {
    int f;
    sscanf(chargeStr.c_str(),"%d",&f);
    frag.charge(f);
  }

  if (! symGrpStr.empty()) {
    frag.pointGroup(symGrpStr.c_str());
  }

  if (! useSymStr.empty()) {
    frag.useSymmetry(useSymStr == "true");
  }

  // initialize atom type vector for reaction studies
  p_atomTypes.clear();
  vector<TAtm*> *atoms = frag.atoms();
  for (size_t it=0; it<frag.numAtoms(); it++) {
    p_atomTypes.push_back((*atoms)[it]->atomType());
  }


  return ret;
}


bool ChemistryTask::getTaskFragment(Fragment& frag) 
{
  bool ret = false;

  Session *session = getSession();
  if (session != 0) {
    ChemistryTask *searchTask = this;
    while ( (!ret) && (searchTask != 0) ) {
      ret = searchTask->getFragment(frag, false);

      // Check if found fragment, if not then set
      // search task to previous task.
      if (!ret) {
        ChemistryTask *newSearchTask = dynamic_cast<ChemistryTask*>(searchTask->getInputProvider(session));
        searchTask = newSearchTask;
      }
    }
  }

  return ret;
}


/**
 * Big data member so returns a pointer to the fragment.
 * User is responsible for deleting when finished.
 * Returns null on error.  (Not all fragment data may get set
 * if some properties are missing.)
 *
 * @return Returns a Fragment pointer, user responsible for memory
 *         management.
 */
Fragment* ChemistryTask::fragment() const
{
  p_msgStack->clear();
  Fragment * fragment = new Fragment;
  if ((const_cast<ChemistryTask*>(this))->getFragment(*fragment)) {
    return fragment;
  } else {
    delete fragment;
  }

  return 0;
}


bool ChemistryTask::isFragmentReadOnly() const
{
  // MD tasks other than prepare should make the builder read-only
  ResourceDescriptor::CONTENTTYPE ctype = getContentType();
  bool ret = (ctype == ResourceDescriptor::CT_MDOPTIMIZE ||
              ctype == ResourceDescriptor::CT_MDENERGY ||
              ctype == ResourceDescriptor::CT_MDDYNAMICS);

  return ret;
}


/**
 * Determines, by comparing the :lastmodified times of the input
 * and molecule file, if the molecule file is new.  
 * 
 * @return Returns true if the :lastmodified time of the 
 *         molecule file is newer than the :lastmodified time
 *         of the input file.
 */
bool ChemistryTask::isFragmentNew() const
{
  bool ret = false;
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

  return ret;
}



/** 
 * Writes the indicated original source fragment to file on the dav server.
 * If frag is NULL, then it erases the fragment from the dav
 * server.
 *
 * @return Returns true if the fragment was successfully stored, 
 *         otherwise false is returned.
 * 
 * @todo Need to augment the Fragment class to put
 *       the state information requested in the Fragment class.
 *       Need logic to get all Molecule formats...
 */
bool ChemistryTask::setOriginalFragment(istream *is,
                                        int selectedModel,
                                        const string& alternatelocation,
                                        const string& chainId,
                                        const string& name)
{
  return false;
}


/**
 * Original source fragment is returned.
 * Big data member so returns a pointer to the fragment.
 * User is responsible for deleting when finished.
 * Returns null on error.  (Not all fragment data may get set
 * if some properties are missing.)
 *
 * @return Returns a Fragment pointer, user responsible for memory
 *         management.
 */
bool ChemistryTask::getOriginalFragment(Fragment& frag)
{
  return false;
}



/**
 * Determines if an original fragment exists.
 */
bool ChemistryTask::hasOriginalFragment()
{
  return false;
}


/**
 * Performs serialization of the ESP Model to the DAV 
 * server.  If the model pointer is null, then the 
 * ESP file is removed from the DAV server.
 *
 * @param model Pointer to an ESP Model
 *
 * @return Returns true if the ESP model was successfully
 *         serialized out to the DAV server, otherwise 
 *         false is returned.
 */
bool ChemistryTask::setESPModel(ESPConstraintModel* model)
{
   bool ret = true;
   p_msgStack->clear();

   // get ESP resource name
   string espName = getVDoc()->getEspConstraintsName();

   if (model && model->size() > 0) {
      Serializer *ser = new ESPConstraintModelXMLizer();
      string data;
      EcceMap props;
      try {
        ser->serialize(*model,data,props); 
        istrstream is(data.c_str());
        getVDoc()->addConfiguration(espName, &is);
      } catch (std::exception&) {
         ret = false;
      }
      delete ser;
   } else {
      // Remove the resource
      try {
        getVDoc()->removeConfiguration(espName);
      } 
      catch (DavException&) {
        ret = false;
      }
   }
   return ret;
}


/**
 * Performs deserialization of the ESP Model from the DAV 
 * server.  
 *
 * @param model ESP Model referenece, deserialized model stored here.
 *
 * @return Returns true if the ESP model was successfully
 *         deserialized from the DAV server, otherwise 
 *         false is returned.
 */
bool ChemistryTask::getESPModel(ESPConstraintModel& model) const
{
   // Note - nothing done with props
   bool ret = true;
   p_msgStack->clear();

   // set url 
   string espName = getVDoc()->getEspConstraintsName();
   getEDSI()->setURL(getVDoc()->getConfiguration(espName));

   EcceMap props;
   ostrstream os;

   if (load(os,props)) {
      char *cptr = os.str();
      string data = cptr;
      delete [] cptr;
      Serializer *ser = new ESPConstraintModelXMLizer();
      try {
         ser->deserialize(data, model, props); 
      } catch (ParseException& ex) {
         p_msgStack->add(ex.what());
         ret = false;
      }
      delete ser;
   } else {
      ret = false;
   }

   return ret;
}


/**
 * Stores geometry constraints created for the current calculation.
 * 
 * @param model Pointer to a GeomConstraintModel object, containing
 *              the list of constraints.
 *
 * @return returns a boolean.  True is returned if the constraints 
 *         were successfully stored.
 */
bool ChemistryTask::setGeomConstraintModel(GeomConstraintModel* model) 
{
   bool ret = true;
   p_msgStack->clear();

   // get Geometry resource name
   string geometryName = getVDoc()->getGeometryConstraintsName();

   if (model && model->size() > 0) {
      Serializer *ser = new GeomConstraintModelXMLizer();
      string data;
      EcceMap props;
      try {
        ser->serialize(*model,data,props); 
        istrstream is(data.c_str());
        getVDoc()->addConfiguration(geometryName, &is);
      } catch (std::exception&) {
         ret = false;
      }
      delete ser;
   } else {
      // Remove the resource
      try {
        getVDoc()->removeConfiguration(geometryName);
      } 
      catch (DavException&) {
        ret = false;
      }
   }
   return ret;
}


/**
 * Retrieves geometry constraints and rules for the current calculation.
 * 
 * @param model Reference to a GeomConstraintModel object, containing
 *              the list of constraints.
 *
 * @return returns a boolean.  True is returned if the constraints and rules
 *         were successfully loaded.
 */
bool ChemistryTask::getGeomConstraintModel(GeomConstraintModel& model) const 
{
   bool ret = true;
   const JCode *codeCap = application();
   p_msgStack->clear();

   // Get and set constraint rules for current code
   // This needs to be done before the actual deserialization,
   // to ensure that the correct rules are checked when 
   // adding constraints to the model.
   if (codeCap) {
     GeomConstraintRules *rules = codeCap->getGeomConstraintRules();
     model.setRules(rules);
   }

   // set url 
   string geometryName = getVDoc()->getGeometryConstraintsName();
   getEDSI()->setURL(getVDoc()->getConfiguration(geometryName));

   // Perform deserialization
   EcceMap props;
   ostrstream os;
   if (load(os,props)) {
      char *cptr = os.str();
      string data = cptr;
      delete [] cptr;
      Serializer *ser = new GeomConstraintModelXMLizer();
      try {
         ser->deserialize(data, model, props); 
      } 
      catch (ParseException& ex) {
         p_msgStack->add(ex.what());
         ret = false;
      }
      delete ser;
   } else {
      ret = false;
   }

   return ret;
}


/**
 * Stores meta-potentials created for the current calculation.
 * 
 * @param model Pointer to a MetaPotentialModel object, containing
 *              the list of potentials.
 *
 * @return returns a boolean.  True is returned if the potentials 
 *         were successfully stored.
 */
bool ChemistryTask::setMetaPotentialModel(MetaPotentialModel* model) 
{
   bool ret = true;
   p_msgStack->clear();

   // get metat-potential resource name
   string potentialName = getVDoc()->getMetaPotentialsName();

   if (model && model->size() > 0) {
      Serializer *ser = new MetaPotentialModelXMLizer();
      string data;
      EcceMap props;
      try {
        ser->serialize(*model,data,props); 
        istrstream is(data.c_str());
        getVDoc()->addConfiguration(potentialName, &is);
      } catch (std::exception&) {
         ret = false;
      }
      delete ser;
   } else {
      // Remove the resource
      try {
        getVDoc()->removeConfiguration(potentialName);
      } 
      catch (DavException&) {
        ret = false;
      }
   }
   return ret;
}


/**
 * Retrieves meta-potentials for the current calculation.
 * 
 * @param model Reference to a MetaPotentialModel object, containing
 *              the list of potentials.
 *
 * @return returns a boolean.  True is returned if the potentials
 *         were successfully loaded.
 */
bool ChemistryTask::getMetaPotentialModel(MetaPotentialModel& model) const 
{
   bool ret = true;
   p_msgStack->clear();

   // set url 
   string potentialName = getVDoc()->getMetaPotentialsName();
   getEDSI()->setURL(getVDoc()->getConfiguration(potentialName));

   // Perform deserialization
   EcceMap props;
   ostrstream os;
   if (load(os,props)) {
      char *cptr = os.str();
      string data = cptr;
      delete [] cptr;
      Serializer *ser = new MetaPotentialModelXMLizer();
      try {
         ser->deserialize(data, model, props); 
      } 
      catch (ParseException& ex) {
         p_msgStack->add(ex.what());
         ret = false;
      }
      delete ser;
   } else {
      ret = false;
   }

   return ret;
}


/**
 * Stores QMMM terms created for the current calculation.
 * 
 * @param model Pointer to a QMMMModel object, containing
 *              the list of potentials.
 *
 * @return returns a boolean.  True is returned if the QMMM terms
 *         were successfully stored.
 */
bool ChemistryTask::setQMMMModel(QMMMModel* model) 
{
   bool ret = true;
   p_msgStack->clear();

   // get QMMM resource name
   string QMMMName = getVDoc()->getQMMMTermsName();

   if (model && model->size() > 0) {
      Serializer *ser = new QMMMModelXMLizer();
      string data;
      EcceMap props;
      try {
        ser->serialize(*model,data,props); 
        istrstream is(data.c_str());
        getVDoc()->addConfiguration(QMMMName, &is);
      } catch (std::exception&) {
         ret = false;
      }
      delete ser;
   } else {
      // Remove the resource
      try {
        getVDoc()->removeConfiguration(QMMMName);
      } 
      catch (DavException&) {
        ret = false;
      }
   }
   return ret;
}


/**
 * Retrieves QMMM terms for the current calculation.
 * 
 * @param model Reference to a QMMMModel object, containing
 *              the list of QMMM terms.
 *
 * @return returns a boolean.  True is returned if the QMMM terms
 *         were successfully loaded.
 */
bool ChemistryTask::getQMMMModel(QMMMModel& model) const 
{
   bool ret = true;
   p_msgStack->clear();

   // set url 
   string QMMMName = getVDoc()->getQMMMTermsName();
   getEDSI()->setURL(getVDoc()->getConfiguration(QMMMName));

   // Perform deserialization
   EcceMap props;
   ostrstream os;
   if (load(os,props)) {
      char *cptr = os.str();
      string data = cptr;
      delete [] cptr;
      Serializer *ser = new QMMMModelXMLizer();
      try {
         ser->deserialize(data, model, props); 
      } 
      catch (ParseException& ex) {
         p_msgStack->add(ex.what());
         ret = false;
      }
      delete ser;
   } else {
      ret = false;
   }

   return ret;
}


/**
 * Stores thumbnail created for the current calculation.
 * 
 * @param file Pointer to an SFile object, containing the thumbnail.
 *
 * @return returns a boolean.  True is returned if the thumbnail was 
 *         successfully stored.
 */
bool ChemistryTask::setThumbnail(SFile* file)
{
  bool ret = false;

  string thumbnailName = getVDoc()->getThumbnailName();
  EcceURL thumbnailUrl;

  if (file != 0) {
    ifstream ifs(file->path().c_str());
    if (ifs.is_open()) {
      try {
        getVDoc()->addConfiguration(thumbnailName, &ifs);
      }
      catch (DavException& davException) {
        p_msgStack->add("UNABLE_TO_COMPLETE_REQUEST", davException.what());
      }
      ret = true;
    }
    ifs.close();
  }

  return ret;
}


/**
 * Retrieves the thumbnail for the current calculation.
 * If the task is session based, we need to get the thumbnail from the root
 * task.
 *
 * @param file file object representation of the thumbnail
 *
 * @return returns a boolean.  True is returned if the thumbnail was 
 *         successfully loaded.
 */
bool ChemistryTask::getThumbnail(SFile* file) const
{
  bool ret = false;

  Session * session = 0;
  session = getSession();
  if (session != 0) {
    vector<Resource *> roots = session->getRootMembers(Session::INPUT_PROVIDER);
    ChemistryTask * chemTask = 0;
    chemTask = dynamic_cast<ChemistryTask *>(roots[0]);
    if (chemTask != 0 && this != chemTask) {
      ret = chemTask->getThumbnail(file);
    }
  }

  EcceURL url = getVDoc()->getConfiguration(getVDoc()->getThumbnailName());

  Resource *thumbnailResource = 0;
  thumbnailResource = EDSIFactory::getResource(url);
  if (thumbnailResource != 0) {
    file = thumbnailResource->getDocument(file);
    if (file != 0) {
      ret = true;
    }
  }

  return ret;
}


/**
 * Save the data and properties.
 * The url is presumed to have been set already.
 * @param data - the stream to fill up
 * @param props - set of properties to get; possibly none.  Retrieved only
 *   if the put succeeds.  values added to map
 */
bool ChemistryTask::load(ostream& data, EcceMap& props) const
{
   bool ret = true;
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
            for (size_t idx=0; idx<results.size(); idx++) {
               props[results[idx].name] = results[idx].value;
            }
         } else {
            ret = false;
         }
      }
   }
   return ret;
}




/**
 * Imports a .frag file and also invokes the processImportParameters
 * method.  Most subclasses will want to override this method.
 */
string ChemistryTask::import( const string& dir, const string& parseFileName) 
   throw(EcceException)
{
   string message;

   string fullFileRoot = dir + "/" + parseFileName;
   size_t index = fullFileRoot.find_last_of('.');
   if (index != string::npos) fullFileRoot = fullFileRoot.substr(0, index);

   // define the frag, basis, and param file names here so they are
   // scoped for the rm command at the end
   string fragFileName = fullFileRoot + ".frag";
   string paramFileName = fullFileRoot + ".param";

   // load .param file
   Preferences param(paramFileName, true);
   if (!param.isValid()) {
      throw EcceException("Setup parse script command "  
         " did not create parameter file.", WHERE);
   }

   importFragment(fragFileName, param);

   // Subclasses may have other stuff in the parameter file.
   processImportParameters(param);

   /* remove temparay files .param .frag files */
   string cmd = "/bin/rm -f ";
   cmd += paramFileName;
   cmd += " ";
   cmd += fragFileName;
   system(cmd.c_str());

   return message;
}



/**
 * As part of import process, load fragment from .frag file.
 * Title and charge taken from param file.
 */
void ChemistryTask::importFragment(const string& fragFileName, 
                                   const Preferences& params)
{
   // import .frag file
   SFile ffile(fragFileName);
   if (!ffile.exists()) {
      throw EcceException( "Setup parse script command "  
         " did not create chemical system file.", WHERE);
   }

   ifstream fragStream(fragFileName.c_str());
   Fragment* frag = new Fragment();
   if (frag!=(Fragment*)0 && frag->restoreMVM(fragStream)) {
      frag->addCovalentBonds();
      frag->name(frag->formula());
   } else {
      throw EcceException(
         "Unable to import chemical system for calculation.", WHERE);
   }

   fragStream.close();

   // Get the fragment title, charge
   string title;
   if (params.getString("title", title)) {
      string::size_type quote;
      while ((quote = title.find('"')) != string::npos)
         title.erase(quote, 1);

      frag->name(title);
   }
   int number;
   if (params.getInt("charge", number)) {
      frag->charge(number);
   }

   this->fragment(frag);

   delete frag;

}



/**
 * Return a string that represents the arguments that should be
 * sent to data parsing scripts.
 * By default an empty string is returned.  Subclasses should
 * override this method if their parse scripts take arguments.
 */
string ChemistryTask::getParseScriptArgs() const
{
   return " .";
}


/**
 * Currently just calls TaskJob virtual method since fragment related
 * parameters are handled directly in the import method.
 */
void   ChemistryTask::processImportParameters(const Preferences& params)
  throw (IOException)
{
   TaskJob::processImportParameters(params);
}
