#include "util/EcceMap.H"
#include "util/EcceURL.H"  
#include "util/ErrMsg.H"
#include "util/SFile.H"

#include "tdat/Fragment.H"
#include "tdat/GUIValues.H"
#include "tdat/TRunType.H"
#include "tdat/TTheory.H"

#include "dsm/JCode.H"
#include "dsm/ResourceDescriptor.H"
#include "dsm/TCalculation.H"
#include "dsm/TGBSConfig.H"

using std::sort;
using std::includes;
 
/***********************************************************************
 CONSTRUCTORS
 TODO I added destructor but we need copy constructor too
 ***********************************************************************/
TCalculation::TCalculation() : ICalculation() {
  p_fragment = 0;
  p_gbsconfig = 0;
  p_guiparams = 0;
  p_runtype = new TRunType;

  p_theory = new TTheory;
  p_code = 0;
  p_state = ResourceDescriptor::STATE_CREATED;
}

TCalculation::~TCalculation()
{
  if (p_theory) delete p_theory;
  if (p_runtype) delete p_runtype;
  if (p_fragment) delete p_fragment;
  if (p_gbsconfig) delete p_gbsconfig;
  // Don't delete guiparams
  // Don't delete jcode
}

TCalculation::TCalculation(TCalculation* const calc) : ICalculation() {
  
  EE_ASSERT(calc,EE_FATAL,"Calculation is Null!");

  setName(         calc->p_name                         );
  owner(           calc->p_owner                        );
  annotations(     calc->p_annotations                  );
  isReviewed(      calc->p_isReviewed                   );
  setReviewedDate( calc->p_reviewedDate                 );
  createdWith(     calc->p_createdwith                  );
  currentVersion(  calc->p_currentversion               );
  startDate(       calc->p_startDate                    );
  completionDate(  calc->p_completionDate               );
  setState(        calc->p_state                        );
  launchdata(      calc->p_launchdata                   );
  openShells(      calc->p_openshells                   );
  spinMultiplicity(calc->p_spinmultiplicity             );
  gbsUseExpCoeff(  calc->p_gbsUseExpCoeff               );

  p_runtype = new TRunType;
  if (calc->p_runtype)
    runtype(*(calc->p_runtype));
     
  if (calc->p_theory)
    theory(new TTheory(*(calc->p_theory)) );
  else
    p_theory = (TTheory*) 0;

  // JCodes are shared const pointers
  p_code = calc->p_code;

  if (calc->p_gbsconfig)
    gbsConfig(new TGBSConfig(*(calc->p_gbsconfig)) );
  else
    p_gbsconfig = (TGBSConfig*) 0;
  if (calc->p_fragment)
  {
    Fragment* tmp = new Fragment;
    *tmp = *(calc->p_fragment);
    fragment(tmp);
    // Fragment copy constructor isn't working right
    //fragment(new Fragment(*(calc->p_fragment)));
  }
  else
    p_fragment = (Fragment*) 0;
  if (calc->p_guiparams)
    guiparams(new GUIValues(*(calc->p_guiparams))  );
  else
    p_guiparams = (GUIValues*) 0;
}

bool TCalculation::isValid() throw (CancelException,RetryException)
{
  return true;
}

/***********************************************************************
 MODIFIERS
 ***********************************************************************/

bool TCalculation::setURL(const EcceURL& url) {
  // does nothing
  return false;
}

bool TCalculation::move(EcceURL& target) {
  // does nothing
  return false;
}

bool TCalculation::remove(void) {
  // does nothing
  return false;
}

bool TCalculation::setName(const string& name) {
  p_name = name;
  return true;
}

bool TCalculation::owner(const string& owner) {
  p_owner = owner;
  return true;
}

bool TCalculation::openShells(unsigned long openshell) {
  p_openshells = openshell;
  return true;
}

bool TCalculation::spinMultiplicity(SpinMult::SpinMultEnum sm) {
  p_spinmultiplicity= sm;
  return true;
}

bool TCalculation::gbsUseExpCoeff(bool useExpCoeff) {
  p_gbsUseExpCoeff = useExpCoeff;
  return true;
}

bool TCalculation::annotations(const string& annotations) {
  p_annotations = annotations;
  return true;
}

bool TCalculation::citations(const string& citations) {
  p_citations = citations;
  return true;
} 

bool TCalculation::isReviewed(const bool& flag) {
  p_isReviewed = flag;
  return true;
}

bool TCalculation::setReviewedDate(const string& date) {
  p_reviewedDate = date;
  return true;
}

bool TCalculation::application(const JCode* code) {
  p_code = code;
  return true;
}

bool TCalculation::createdWith(const string& version) {
  p_createdwith = version;
  return true;
}

bool TCalculation::currentVersion(const string& version) {
  p_currentversion = version;
  return true;
}

bool TCalculation::theory(TTheory* theory, const string& theoryType) {
  p_theory = theory;
  return true;
}

bool TCalculation::runtype(const TRunType& name) {
  *p_runtype = name;
  return true;
}

bool TCalculation::guiparams(GUIValues *params, const string& theoryType) {
  //TODO delete the existing object first?
  p_guiparams = params;
  return true;
}


EcceMap *TCalculation::inputFileMimeTypes()  const
{
  EcceMap* ret = NULL;
  return ret;
}


EcceMap *TCalculation::outputFileMimeTypes() const
{
  EcceMap* ret = NULL;
  return ret;
}


bool TCalculation::removeInputFiles() {
  return true;
}


bool TCalculation::removeOutputFiles() {
  return true;
}


bool TCalculation::putAnyFile(const  string& name, istream* istr) {
  // does nothing
  return true;
}

bool TCalculation::putInputFile(const string& name, istream* istr) {
  // does nothing
  return true;
}

bool TCalculation::putOutputFile(const string& name, istream* istr) {
  // does nothing
  return true;
}

bool TCalculation::startDate(const string& date) {
  p_startDate = date;
  return true;
}

bool TCalculation::completionDate(const string& date) {
  p_completionDate = date;
  return true;
}

bool TCalculation::setState(const ResourceDescriptor::RUNSTATE& state) {
  p_state = state;
  return true;
}

bool TCalculation::jobdata(Jobdata data) {
  p_jobdata = data;
  return true;
}

bool TCalculation::gbsConfig(TGBSConfig* config, const string& theoryType) {
  p_gbsconfig = config;
  return true;
}

bool TCalculation::launchdata(const Launchdata& data) {
  p_launchdata = data;
  return true;
}

bool TCalculation::fragment(Fragment* frag) {
  p_fragment = frag;
  return true;
}

bool TCalculation::joblog(const char* log) 
{
  bool ret = false;
  return ret;
}


bool TCalculation::startJobLog(const char* log) 
{
  bool ret = false;
  return ret;
}

bool TCalculation::appendJobLog(const char* log) 
{
  bool ret = false;
  return ret;
}


bool TCalculation::endJobLog(const char* log) 
{
  bool ret = false;
  return ret;
}

bool TCalculation::removeJobLog(void)
{
  bool ret = false;
  return ret;
}



/***********************************************************************
 ACCESSORS
 ***********************************************************************/

EcceURL TCalculation::getURL(void) const {
  // does nothing
  EcceURL url;
  return url;
}

bool TCalculation::isInputFileNew(void) const {
  bool ret = false;
  return ret;
}

bool TCalculation::isFragmentNew(void) const {
  bool ret = false;
  return ret;
}

string TCalculation::joblog(void) const {
  string ret = "";
  return ret;
}

string  TCalculation::name(void) const {
  return p_name;
}

string TCalculation::messages(void) const {
  // does nothing
  string message;
  return message;
}

string TCalculation::owner(void) const {
  return p_owner;
}

unsigned long TCalculation::openShells(void) const {
  return p_openshells;
}

SpinMult::SpinMultEnum TCalculation::spinMultiplicity(void) const {
  return p_spinmultiplicity;
}

bool TCalculation::gbsUseExpCoeff(void) const {
  return p_gbsUseExpCoeff;
}

string TCalculation::annotations(void) const {
  return p_annotations;
}

string TCalculation::citations(void) const {
  return p_citations;
}

bool TCalculation::isReviewed(void) const {
  return p_isReviewed;
}

string TCalculation::getReviewedDate(void) const {
  return p_reviewedDate;
}

string TCalculation::createdWith(void) const {
  return p_createdwith;
}

string TCalculation::currentVersion(void) const {
  return p_currentversion;
}

GUIValues *TCalculation::guiparams(const string& theoryType) const {
  return p_guiparams;
}

vector<EcceURL> TCalculation::getDataFiles(JCode::CodeFileType type) const {
  vector<EcceURL> ret;
  return ret;
}

vector<string> TCalculation::getDataFileNames(JCode::CodeFileType type) const {
  return p_datafiles;
}

vector<string> TCalculation::getAllFileNames(void) const {
  return p_datafiles;
}

istream* TCalculation::getAnyFile(const  string& name) const {
  // does nothing
  return 0;
}
istream* TCalculation::getAnyFile(const  EcceURL& url) const {
  // does nothing
  return 0;
}
SFile*   TCalculation::getAnyFile(const string& name, const SFile *dest) const
{
   return (SFile*)dest;
}
SFile*   TCalculation::getAnyFile(const EcceURL& url, const SFile *dest) const
{
   return (SFile*)dest;
}

void TCalculation::getDataFile(JCode::CodeFileType type, TypedFile& dataFile) const {
  // does nothing
}

istream* TCalculation::getDataFile(JCode::CodeFileType type) const {
  // does nothing
  return 0;
}

SFile *TCalculation::getDataFile(JCode::CodeFileType type, const SFile *dest) const
{
   return (SFile*)dest;
}


string TCalculation::startDate(void) const {
  return p_startDate;
}

string TCalculation::completionDate(void) const {
  return p_completionDate;
}

ResourceDescriptor::RUNSTATE TCalculation::getState() const {
  return p_state;
}

Jobdata TCalculation::jobdata(void) const {
  return p_jobdata;
}

TGBSConfig* TCalculation::gbsConfig(const string& theoryType) const {
  return p_gbsconfig;
}

Launchdata TCalculation::launchdata(void) const {
  return p_launchdata;
}

TRunType TCalculation::runtype(void) const {
  return *p_runtype;
}

const JCode* TCalculation::application(void) const {
  return p_code;
}

TTheory* TCalculation::theory(const string& theoryType) const {
  return p_theory;
}

Fragment* TCalculation::fragment(void) const {
  return p_fragment;
}

bool TCalculation::getFragment(Fragment& frag) {
  bool ret = false;
  if (p_fragment) {
    frag = *p_fragment;
    ret = true;
  }
  return ret;
}

bool TCalculation::getFragmentStep(Fragment& frag, int step) {
  bool ret = false;
  if (p_fragment) {
    frag = *p_fragment;
    ret = true;
  }
  return ret;
}

long TCalculation::getFragmentModifiedDate() const {
  return 0;
}

/**
 * A user supplied description of the Calculation Task.  
 * An empty string is returned if the dav property does
 * not exist or there is an error connecting to the
 * dav server.
 * @return string containing user supplied description
 *         of the Calculation task
 */
string TCalculation::getTaskDescription() const
{
  string taskDescription = "";
  return taskDescription;
}



/**
 * A default title supplied for the task.  This
 * is the runtype for a Calculation task.
 * 
 * @return string containing the system supplied title of
 *         the calculation task.
 */
string TCalculation::getTaskTitle() const
{
  string taskTitle = "";
  return taskTitle;
}



/**
 * Modifies task description; a user supplied description
 * of the task.  
 * 
 * @param taskDescription string containing description of task
 * @return returns a bool value, true if taskDescription was 
 *         successfully saved, false otherwise
 */
bool TCalculation::setTaskDescription(const string& taskDescription)
{
  return true;
}



/**
 * Indicates if the Calculation's input provider (i.e. other Tasks 
 * within a Session) can be modified/removed.  For a Calculation task this 
 * is determined by the ResourceDescriptor::RUNSTATE - if it has not been SUBMITTED 
 * then it may be altered.
 * 
 * @return bool indicates that the Task may be altered within a Session
 */
bool TCalculation::canModifyInputProvider()
{
  return (getState() < ResourceDescriptor::STATE_SUBMITTED);
}



/**
 * No need for notification for TCalculations
 */
void TCalculation::notifyProperty(const string &name, const string &value)
{
}


void TCalculation::notifyState(const string &state)
{
}
