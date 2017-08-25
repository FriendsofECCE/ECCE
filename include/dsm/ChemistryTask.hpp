/**
 * @file
 *
 *
 */
#ifndef _CHEMISTRYTASK_HH
#define _CHEMISTRYTASK_HH


#include "tdat/ESPConstraintModel.H"
#include "tdat/GeomConstraintModel.H"
#include "tdat/MetaPotentialModel.H"
#include "tdat/QMMMModel.H"
#include "tdat/Fragment.H"
#include "dsm/TaskJob.H"


/**
 * Class providing an access architecture fo applications requiring
 * Chemistry Tasks.
 */
class ChemistryTask : virtual public TaskJob 
{
   public:
      virtual ~ChemistryTask();

      virtual bool fragment(Fragment* frag); 
      virtual bool hasFragment();
      virtual bool getFragment(Fragment& frag, const bool& taskCheck=true);
      virtual bool getTaskFragment(Fragment& frag);
      virtual Fragment* fragment() const; 
      virtual bool isFragmentReadOnly() const;
      virtual bool isFragmentNew()  const;

      virtual bool hasOriginalFragment();
      virtual bool setOriginalFragment(istream *is,
                                       int selectedModel = 1,
                                       const string& alternatelocation = " ",
                                       const string& chainId = " ",
                                       const string& name = "");

      virtual bool getOriginalFragment(Fragment& frag);

      virtual bool setESPModel(ESPConstraintModel* model); 
      virtual bool getESPModel(ESPConstraintModel& model) const; 

      virtual bool setGeomConstraintModel(GeomConstraintModel* model); 
      virtual bool getGeomConstraintModel(GeomConstraintModel& model) const; 

      virtual bool setMetaPotentialModel(MetaPotentialModel* model); 
      virtual bool getMetaPotentialModel(MetaPotentialModel& model) const; 

      virtual bool setQMMMModel(QMMMModel* model); 
      virtual bool getQMMMModel(QMMMModel& model) const; 

      virtual bool setThumbnail(SFile* file);
      virtual bool getThumbnail(SFile* file) const;

      virtual string import( const string& dir, const string& file) 
         throw(EcceException);
      virtual string getParseScriptArgs() const;

   protected:
      ChemistryTask();

      bool load(ostream& data, EcceMap& props) const;    

      void importFragment(const string& fragFile, const Preferences& params);
      virtual void  processImportParameters(const Preferences& params)
         throw(IOException);

      vector<string> p_atomTypes; // used for reaction studies
};


#endif


