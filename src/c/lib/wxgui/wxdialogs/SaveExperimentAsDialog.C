#include <fstream>
  using std::ofstream;
 
#include <wx/bmpcbox.h> // for wxBitmapComboBox

#include "dsm/ChemistryTask.H"
#include "dsm/DirDyVTSTTask.H"
#include "dsm/EDSIFactory.H"
#include "dsm/Resource.H"
#include "dsm/ResourceDescriptor.H"
#include "dsm/ResourceType.H"
#include "dsm/Session.H"

#include "tdat/FragUtil.H"

#include "util/JMSPublisher.H"
#include "util/ResourceUtils.H"
#include "util/StringTokenizer.H"
#include "util/TempStorage.H"

#include "wxgui/ewxBitmap.H"
#include "wxgui/ewxConfig.H"
#include "wxgui/ewxWindowUtils.H"

#include "wxgui/SaveExperimentAsDialog.H"


long ID_SAVE_AS_FILTER_CHOICE = wxNewId();


IMPLEMENT_CLASS(SaveExperimentAsDialog, ewxGenericFileDialog)


BEGIN_EVENT_TABLE(SaveExperimentAsDialog, ewxGenericFileDialog)
  EVT_COMBOBOX(ID_SAVE_AS_FILTER_CHOICE,
               SaveExperimentAsDialog::OnChoiceSaveAsFilter)
END_EVENT_TABLE()


SaveExperimentAsDialog::SaveExperimentAsDialog()
  : ewxGenericFileDialog(),
    p_bitmapCombo(NULL),
    p_imageIndex(-1),
    p_chemFileIndex(-1),
    p_nameToType()
{ }


SaveExperimentAsDialog::SaveExperimentAsDialog(wxWindow *parent)
  : ewxGenericFileDialog(),
    p_bitmapCombo(NULL),
    p_imageIndex(-1),
    p_chemFileIndex(-1),
    p_nameToType()
{
  Create(parent);
  restoreSettings();
}


SaveExperimentAsDialog::~SaveExperimentAsDialog()
{
}


bool SaveExperimentAsDialog::Create(wxWindow *parent)
{
  if (!ewxGenericFileDialog::Create(parent, "Save Chemical System As...",
            wxEmptyString, wxEmptyString, wxEmptyString, wxFD_SAVE)) {
    return false;
  }

  SetTitle("Save Chemical System As...");
  SetName("SaveChemicalSystemAsDialog");

  SetExtraStyle(wxWS_EX_VALIDATE_RECURSIVELY);

  p_bitmapCombo = new wxBitmapComboBox(this, ID_SAVE_AS_FILTER_CHOICE, "",
          wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY);

  // HACK - (standalone) ebuilder sets this env var
  if (getenv("ECCE_NO_MESSAGING") == NULL) {
    // create the bitmap combo for our calc types
    ResourceType *type = ResourceDescriptor::getResourceDescriptor().
            getResourceType(ResourceDescriptor::RT_COLLECTION,
                            ResourceDescriptor::CT_PROJECT,
                            ResourceDescriptor::AT_UNDEFINED);
    vector<ResourceType*> typeVec = type->getContains();
    vector<ResourceType*>::iterator typeIter;
    // for each "contained"
    // add to menu, including ResourceType*, but skip the Project type
    for (typeIter = typeVec.begin(); typeIter != typeVec.end(); typeIter++) {
      if ((*typeIter)->getMenuItem() == "New Project...") continue;
      wxString name((*typeIter)->getMenuItem());
      name = name.AfterFirst(' ').BeforeFirst('.'); // remove "New " and "..."
      p_nameToType[(*typeIter)->getName()] = (*typeIter);
      p_bitmapCombo->Append(name, ewxBitmap((*typeIter)->getIcon()),
                            new wxStringClientData((*typeIter)->getName()));
    }
  }

  // image format
  p_imageIndex = p_bitmapCombo->GetCount();
  p_bitmapCombo->Append("POV-Ray (*.pov)", ewxBitmap("camera16.xpm"),
                        new wxStringClientData("pov"));
  if (!getenv("ECCE_NO_VIZIMAGES")) {
    p_bitmapCombo->Append("JPEG (*.jpg,*.jpeg)", ewxBitmap("camera16.xpm"),
                          new wxStringClientData("jpg|jpeg"));
    p_bitmapCombo->Append("PNG (*.png)", ewxBitmap("camera16.xpm"),
                          new wxStringClientData("png"));
    p_bitmapCombo->Append("GIF (*.gif)", ewxBitmap("camera16.xpm"),
                          new wxStringClientData("gif"));
    p_bitmapCombo->Append("TIF (*.tif,*.tiff)", ewxBitmap("camera16.xpm"),
                          new wxStringClientData("tif|tiff"));
  }
  p_bitmapCombo->Append("Postscript (*.ps)", ewxBitmap("camera16.xpm"),
                        new wxStringClientData("ps"));

  // chemical system formats
  p_chemFileIndex = p_bitmapCombo->GetCount();
  p_bitmapCombo->Append("CAR (*.car)", ewxBitmap("chem16.xpm"),
                        new wxStringClientData("car"));
  p_bitmapCombo->Append("ECCE Internal Type (*.mvm)",ewxBitmap("chem16.xpm"),
                        new wxStringClientData("mvm"));
  p_bitmapCombo->Append("NWChem Input (*.nw)", ewxBitmap("chem16.xpm"),
                        new wxStringClientData("nw"));
  p_bitmapCombo->Append("PDB (*.pdb)", ewxBitmap("chem16.xpm"),
                        new wxStringClientData("pdb"));
  p_bitmapCombo->Append("XYZ (*.xyz)", ewxBitmap("chem16.xpm"),
                        new wxStringClientData("xyz"));
  
  // replace the wildcard filter with our bitmap combo, destory old filter
  GetSizer()->Replace(m_choice, p_bitmapCombo, true);
  m_choice->Hide();

  return true;
}


/**
 * If this dialog natively supports saving, this will return true.
 * For example, image rendering is not supported.
 * If a chemical system format or ECCE type, this function will return true.
 */
bool SaveExperimentAsDialog::canSave() const
{
  int selection = p_bitmapCombo->GetSelection();
  return selection < p_imageIndex || selection >= p_chemFileIndex;
}


/**
 * Creates a file based on the acquired filename, directory, and type.
 * Assumes a valid filename.
 *
 * @param fragment to write
 * @param context will point to the created URL if a chem file or ECCE type.
 *        This parameter will be empty if the return value for this
 *        function is false.
 * @return true if successful
 */
bool SaveExperimentAsDialog::doSave(Fragment *fragment, string& context) const
{
  int selection = p_bitmapCombo->GetSelection();
  if (selection < p_imageIndex) {
    return doSaveAsChemistryTask(fragment, context);
  } else if (selection < p_chemFileIndex) {
    return false; // images not supported natively
  } else {
    return doSaveAsChemFileType(fragment, context);
  }

  return false;
}


wxString SaveExperimentAsDialog::getType() const
{
  int selection = p_bitmapCombo->GetSelection();
  if (selection < p_imageIndex) { // it's an ecce type
    return "";
  } else if (selection < p_chemFileIndex) { // it's an image format
    return p_bitmapCombo->GetStringSelection();
  } else { // it's a chem file type
    return p_bitmapCombo->GetStringSelection().BeforeFirst('(').BeforeLast(' ');
  }
  return "";
}


/**
 * Return the extension in the filename textctrl, if any.
 */
wxString SaveExperimentAsDialog::getExt() const
{
  return GetFilename().AfterLast('.');
}


/**
 * Get the extension for the wildcard choice box.
 * Note that it could return multiple valid extensions in the form
 * ext1|ext2|...|extn
 * or simply 
 * ext1
 */
wxString SaveExperimentAsDialog::getExts() const
{
  int selection = p_bitmapCombo->GetSelection();
  if (selection < p_imageIndex) { // it's an ecce type
    return "";
  } else {
    wxStringClientData *data = dynamic_cast<wxStringClientData*>(
            p_bitmapCombo->GetClientObject(p_bitmapCombo->GetSelection()));
    if (data) {
      return data->GetData();
    }
  }
  return "";
}


void SaveExperimentAsDialog::EndModal(int retCode)
{
  if (retCode == wxID_OK) {
    saveSettings();
  }
  ewxGenericFileDialog::EndModal(retCode);
}


void SaveExperimentAsDialog::setSaveAsFilterIndex(int index)
{
  p_bitmapCombo->SetSelection(index);
  DoSetSaveAsFilterIndex(index);
}


void SaveExperimentAsDialog::OnChoiceSaveAsFilter(wxCommandEvent& event)
{
  DoSetSaveAsFilterIndex((int)event.GetInt());
}


bool SaveExperimentAsDialog::doSaveAsChemFileType(
        Fragment *fragment, string& context) const
{
  bool ret = true;

  // type is determined from chem combo item, ex) "PDB (*.pdb,*.ent)"
  wxString sel = p_bitmapCombo->GetStringSelection();
  wxString types = sel.AfterFirst('(').BeforeLast(')'); // *.pdb,*.ent
  StringTokenizer tokenizer(types.c_str());
  vector<string> tokens = tokenizer.tokenize(",");
  vector<string>::iterator token;
  wxString type = tokens[0];  // *.pdb
  type = type.AfterLast('.'); // pdb
  bool needExtension = true;

  // get filename and append proper extension if needed
  wxString filename = GetFilename();
  for (token = tokens.begin(); token != tokens.end(); ++token) {
    // ex) *.pdb
    wxString t = *token;
    if (filename.Lower().EndsWith(t.AfterLast('.').Lower())) {
      needExtension = false;
      break;
    }
  }
  if (needExtension) {
    filename << "." << type.Lower();
  }
  SFile *file = TempStorage::getTempFile();
  file->move(file->pathroot() + "/" + filename.c_str());

  type = type.MakeUpper();
  if (type.IsSameAs("XYZ")) {
    ofstream xyz(file->path().c_str());
    ret = fragment->dumpXYZ(xyz);
    xyz.close();
  } else if (type.IsSameAs("MVM")) {
    ofstream mvm(file->path().c_str());
    ret = fragment->dumpMVM(mvm);
    mvm.close();
  } else if (type.IsSameAs("CAR")) {
    ofstream car(file->path().c_str());
    ret = fragment->dumpCAR(car);
    car.close();
  } else if (type.IsSameAs("PDB")) {
    ofstream pdb(file->path().c_str());
    if (fragment->numResidues() > 0) {
      ret = fragment->dumpPDB(pdb);
    } else {
      Fragment *tmpfrag = fragment->clone();
      FragUtil fragutil(tmpfrag);
      fragutil.createResidues();
      ret = tmpfrag->dumpPDB(pdb);
      delete tmpfrag;
    }
    pdb.close();
  }

  // the fragment is in the temporary file, now put it where it belongs
  string parentUrlStr = GetDirectory().c_str();
  Resource *parent = 0;
  Resource *child = 0;
  try {
    parent = EDSIFactory::getResource(parentUrlStr);
    if (!parent) throw InvalidException("Null Resource", WHERE);

    child = parent->createChild(filename.c_str(), file);
    if (!child) throw InvalidException("Null Resource", WHERE);
  } catch (EcceException& ex) {
    ret = false;
    // the child is either a calculation or a session, in either case removing
    // it will take care of all subdirectories and child resources
    if (child) child->remove();
    child = 0;
  }

  // success!
  if (ret) {
    notifyCreate(child->getURL().toString());
    context = child->getURL().toString();
  }

  if (file) {
    file->remove();
    delete file;
  }

  return ret;
}


/**
 * Creates the new calculation with the acquired filename, directory, and type.
 * Assumes the filename is valid.
 */
bool SaveExperimentAsDialog::doSaveAsChemistryTask(
        Fragment *fragment, string& context) const
{
  bool ret = true;

  string name = GetFilename().c_str();
  string parentUrlStr = GetDirectory().c_str();
  wxStringClientData *data = dynamic_cast<wxStringClientData*>(
          p_bitmapCombo->GetClientObject(p_bitmapCombo->GetSelection()));
  wxString resourceName = data->GetData();
  map<wxString, ResourceType*>::const_iterator it =
          p_nameToType.find(resourceName);
  if (it == p_nameToType.end()) return false; // bail early
  ResourceType *type = it->second;

  Resource      *parent = 0;
  Resource      *child = 0;
  ChemistryTask *task = 0;

  try {
    parent = EDSIFactory::getResource(parentUrlStr);
    if (!parent) throw InvalidException("Null Resource", WHERE);

    child = parent->createChild(name, type);
    if (!child) throw InvalidException("Null Resource", WHERE);

    // Sessions (MD and Reaction) require extra Resource(s)
    if (child->getContentType() == ResourceDescriptor::CT_SESSION) {
      Session *session = dynamic_cast<Session*>(child);
      if (child->getApplicationType() == ResourceDescriptor::AT_REACTION_STUDY)
      {
        task = doSaveReactionStudy(session);
      }
      else if (child->getApplicationType() == ResourceDescriptor::AT_MDSTUDY)
      {
        task = doSaveMdStudy(session);
      }
    }
    else { // child is not a session
      task = dynamic_cast<ChemistryTask*>(child);
    }

    // now that the resource is created, add the fragment to it
    if (!task) throw InvalidException("Null cast to ChemistryTask", WHERE);
    if (!task->fragment(fragment)) 
      throw InvalidException("Could not store fragment", WHERE);

  } catch (EcceException& ex) {
    ret = false;
    // the child is either a calculation or a session, in either case removing
    // it will take care of all subdirectories and child resources
    if (child) child->remove();
    child = 0;
    task = 0;
  }

  // success!
  if (ret) {
    // we only need to notify the calc/session creation, since other apps will
    // discover session children upon inspection of the session
    notifyCreate(child->getURL().toString());

    // set the context to the correct calc or session child, so that the
    // calling app can reset their context if wanted
    context = task->getURL().toString();
  }

  return ret;
}


/**
 * Returns a pointer to the Prepare task where we will add the fragment.
 * Performs all necessary task creation and linking.
 */
ChemistryTask* SaveExperimentAsDialog::doSaveMdStudy(Session *session) const
{
  ResourceType  *prepareType = 0;
  Resource      *child = 0;
  ChemistryTask *task = 0;

  try {
    // find the prepare type, contained by the session we were passed
    vector<ResourceType*> types = session->getDescriptor()->getContains();
    vector<ResourceType*>::iterator typeIter;
    for (typeIter = types.begin(); typeIter != types.end(); typeIter++) {
      if ((*typeIter)->getContentType() == ResourceUtils::contentTypeToString(
                                           ResourceDescriptor::CT_MDPREPARE)) {
        prepareType = (*typeIter);
        break;
      }
    }
    if (!prepareType) throw InvalidException("Null ResourceType",WHERE);

    // create the prepare task
    child = session->createChild(prepareType->getLabel(), prepareType);
    if (!child) throw InvalidException("Null Resource", WHERE);

    // link the prepare task in the session
    session->addMemberAsTarget(child);
    
    // set our return pointer
    task = dynamic_cast<ChemistryTask*>(child);

  } catch (InvalidException& ex) {
    // if we failed at any step along the way, clean up all traces of activity
    if (task) { task->remove(); }
    if (session) { session->remove(); }
    task = 0;
    session = 0;
  }
 
  return task;
}


/**
 * Returns a pointer to the Transition-State task where we will add the
 * fragment.  Performs all necessary task creation and linking.
 */
ChemistryTask* SaveExperimentAsDialog::doSaveReactionStudy(
        Session *session) const
{
  ResourceType  *dirdyType = 0;
  Resource      *dirdy = 0;
  ResourceType  *nwchemType = 0;
  Resource      *child = 0;
  ChemistryTask *task = 0;

  try {
    // find the dirdy type, contained by the child we just created
    vector<ResourceType*> types = session->getDescriptor()->getContains();
    vector<ResourceType*>::iterator typeIter;
    for (typeIter = types.begin(); typeIter != types.end(); typeIter++) {
      if ((*typeIter)->getApplicationType() ==
              ResourceUtils::applicationTypeToString(
              ResourceDescriptor::AT_DRDVTST)) {
        dirdyType = (*typeIter);
        break;
      }
    }
    if (!dirdyType) throw InvalidException("Null ResourceType",WHERE);
  
    // create the dirdy calc
    dirdy = session->createChild(dirdyType->getLabel(), dirdyType);
    if (!dirdy) throw InvalidException("Null Resource", WHERE);

    // link in the dirdy calc
    session->addMember(dirdy);
  
    // we need a Transition-State (NWChem ES) calc too
    string linkName = DirDyVTSTTask::componentTypeToString(
                      DirDyVTSTTask::TRANSITION);
    nwchemType = ResourceDescriptor::getResourceDescriptor().
            getResourceType(ResourceUtils::resourceTypeToString(
                            ResourceDescriptor::RT_VIRTUAL_DOCUMENT),
                            ResourceUtils::contentTypeToString(
                            ResourceDescriptor::CT_CALCULATION),
                            "NWChem");
    if (!nwchemType) throw InvalidException("Null ResourceType",WHERE);

    // create the NWChem calc
    child = session->createChild(linkName, nwchemType);
    if (!child) throw InvalidException("Null Resource", WHERE);

    // link in the NWChem calc
    session->addMember(child);
    session->addLink(child, dirdy, Session::INPUT_PROVIDER, linkName);

    // save model
    session->saveLinkbase();

    // set our return pointer
    task = dynamic_cast<ChemistryTask*>(child);

  } catch (InvalidException& ex) {
    // if we failed at any step along the way, clean up all traces of activity
    if (task) { task->remove(); }
    if (dirdy) { dirdy->remove(); }
    if (session) { session->remove(); }
    task = 0;
    dirdy = 0;
    session = 0;
  }

  return task;
}


void SaveExperimentAsDialog::DoSetSaveAsFilterIndex(int index)
{
  wxStringClientData *data = dynamic_cast<wxStringClientData*>(
          p_bitmapCombo->GetClientObject(index));
  wxString str = data->GetData();
  m_list->SetWild(str);
  if (str.Left(2) == wxT("*.")) {
    m_filterExtension = str.Mid(1);
    if (m_filterExtension == _T(".*"))
      m_filterExtension.clear();
  } else {
    m_filterExtension.clear();
  }
}


/**
 */
void SaveExperimentAsDialog::notifyCreate(const string& url) const
{
  JMSPublisher publisher(GetName().c_str());
  JMSMessage *msg = publisher.newMessage();
  msg->addProperty("url", url);
  publisher.publish("ecce_url_created", *msg);
  delete msg;
}


/**
 */
void SaveExperimentAsDialog::saveSettings()
{
  ewxGenericFileDialog::saveSettings();

  ewxConfig *config = ewxConfig::getConfig(string(GetName() + ".ini"));

  config->Write("TYPE", p_bitmapCombo->GetSelection());

  ewxConfig::closeConfigs();
}


/**
 */
void SaveExperimentAsDialog::restoreSettings()
{
  ewxGenericFileDialog::restoreSettings();

  ewxConfig *config = ewxConfig::getConfig(string(GetName() + ".ini"));

  int selection = 0;
  config->Read("TYPE", &selection);
  if (p_bitmapCombo && selection<p_bitmapCombo->GetCount())
    setSaveAsFilterIndex(selection);
  else 
    setSaveAsFilterIndex(0);

  ewxConfig::closeConfigs();

  setServerChoice(m_dir.c_str());
}

