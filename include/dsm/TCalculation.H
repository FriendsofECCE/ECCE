///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: ICalculation.H
//
///////////////////////////////////////////////////////////////////////////////
#ifndef _TCALCULATION
#define _TCALCULATION

#include <string>
  using std::string;
#include <vector>
  using std::vector;

#include "dsm/ICalculation.H"
#include "dsm/ResourceDescriptor.H"

class EcceMap;
class GUIValues;
class TTheory;
class TRunType;

/**
 * Abstract interface definition accessing calculation state.
 *
 * ICalculation is an abstract class providing an access
 * architecture for applications requiring persistant Ecce calculation data
 * structure access or to be used by transient calculations.
 */
class TCalculation : virtual public ICalculation
{
  public:
    TCalculation();
    TCalculation(TCalculation* const icalc);
    virtual ~TCalculation();

  // Individual data element accessors:
    string              name(void)            const;
    string              owner(void)           const;
    string              annotations(void)     const;
    string              citations(void)       const;
    bool                isReviewed(void)      const;
    string              getReviewedDate(void) const;
    string              createdWith(void)     const;
    string              currentVersion(void)  const;
    EcceMap            *inputFileMimeTypes(void) const;
    EcceMap            *outputFileMimeTypes(void)    const;

  // Resource virtuals
    virtual EcceURL getURL(void) const;
    virtual bool isValid() throw (RetryException, CancelException);
    virtual bool remove(void);          // destroys an entire calculation
    virtual bool move(EcceURL& target); // moves an entire calculation
    virtual string messages(void) const;
    virtual bool canModifyInputProvider();
    virtual void notifyProperty(const string &name, const string &value);
    virtual void notifyState(const string &state);

  // TaskJob  virtuals
    virtual bool application(const JCode* code);
    virtual const JCode* application(void) const;
    virtual bool setState(const ResourceDescriptor::RUNSTATE& state);
    virtual ResourceDescriptor::RUNSTATE getState() const;
    virtual bool startDate(const string& sdate);
    virtual string startDate(void) const;
    virtual bool completionDate(const string& cdate);
    virtual string completionDate(void) const;
    virtual bool jobdata(Jobdata jdata);
    virtual Jobdata jobdata(void) const;
    virtual bool launchdata(const Launchdata& ldata);
    virtual Launchdata launchdata(void) const;
    virtual bool joblog(const char* log);
    virtual string joblog(void) const; 
    virtual bool startJobLog(const char* log);
    virtual bool appendJobLog(const char* log);
    virtual bool endJobLog(const char* log);
    virtual bool removeJobLog(void);
    virtual bool isInputFileNew(void) const;
    virtual vector<string> getDataFileNames(JCode::CodeFileType type) const;
    virtual vector<EcceURL> getDataFiles(JCode::CodeFileType type) const;
    virtual vector<string> getAllFileNames(void) const;
    virtual void getDataFile(JCode::CodeFileType type,
                             TypedFile& dataFile) const;
    virtual istream *getAnyFile(const  string& name)    const;
    virtual istream *getAnyFile(const  EcceURL& url)    const;
    virtual istream *getDataFile(JCode::CodeFileType type)  const;
    virtual SFile *getAnyFile(const string& name, const SFile *dest) const;
    virtual SFile *getAnyFile(const EcceURL& url, const SFile *dest) const;
    virtual SFile *getDataFile(JCode::CodeFileType type, const SFile *dest) const;
    virtual bool removeInputFiles();
    virtual bool removeOutputFiles();
    virtual bool putInputFile(const string& name, istream* istr);
    virtual bool putOutputFile(const string& name, istream* istr);
    virtual bool putAnyFile(const  string& name, istream* istr);

  // ChemistryTask virtuals
    virtual bool fragment(Fragment* frag);
    virtual Fragment* fragment(void) const;
    virtual bool getFragment(Fragment& frag);
    virtual bool isFragmentNew(void) const;
    virtual bool setESPModel(ESPConstraintModel* model) {return false;}
    virtual bool getESPModel(ESPConstraintModel& model) const { return 0; }
    virtual bool setGeomConstraintModel(GeomConstraintModel* model)
           {return false;}
    virtual bool getGeomConstraintModel(GeomConstraintModel& model) const
           { return 0; }

  // IPropCalculation/PropertyTask virtuals
    virtual bool getFragmentStep(Fragment& frag, int step);
    virtual long getFragmentModifiedDate() const;

  // ICalculation virtuals
    virtual bool theory(TTheory* theory, const string& theoryType="");
    virtual TTheory* theory(const string& theoryType="") const;
    virtual bool runtype(const TRunType& runtype);
    virtual TRunType runtype(void) const;
    virtual bool guiparams(GUIValues *params, const string& theoryType="");
    virtual GUIValues* guiparams(const string& theoryType="") const;
    virtual bool gbsConfig(TGBSConfig* config, const string& theoryType="");
    virtual TGBSConfig* gbsConfig(const string& theoryType="") const;
    virtual bool gbsUseExpCoeff(bool useExpCoeff);
    virtual bool gbsUseExpCoeff(void) const;
    virtual bool openShells(unsigned long openshell);
    virtual unsigned long openShells(void) const;
    virtual bool spinMultiplicity(SpinMult::SpinMultEnum sm);
    virtual SpinMult::SpinMultEnum spinMultiplicity(void) const;

  // Individual data element modifiers:
     bool          setName(const string& name);
     bool          owner(const string& owner);
     bool          annotations(const string& annotations);
     bool          citations(const string& citations);
     bool          isReviewed(const bool& flag);
     bool          setReviewedDate(const string& date);
     bool          createdWith(const string& version);
     bool          currentVersion(const string& version);
     bool          setURL(const EcceURL& url); 
     // Task operations
     string        getTaskDescription() const;
     string        getTaskTitle() const;
     bool          setTaskDescription(const string& taskDescription);

  protected:
    string           p_name;
    string           p_owner;
    string           p_annotations;
    string           p_citations;
    bool             p_isReviewed; 
    string           p_reviewedDate;
    string           p_createdwith;
    string           p_currentversion;
    GUIValues       *p_guiparams;
    vector<string>   p_datafiles;
    string           p_startDate;
    string           p_completionDate;
    ResourceDescriptor::RUNSTATE         p_state;
    Launchdata       p_launchdata; 
    unsigned long    p_openshells;
    SpinMult::SpinMultEnum p_spinmultiplicity;
    bool             p_gbsUseExpCoeff;

    Jobdata          p_jobdata;
    TRunType        *p_runtype;   // use pointer so we can use forward decl.
    TTheory         *p_theory;
    const JCode      *p_code;
    TGBSConfig      *p_gbsconfig;
    Fragment        *p_fragment; 
};
#endif
