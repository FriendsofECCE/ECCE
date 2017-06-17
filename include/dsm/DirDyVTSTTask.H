/**
 * @file
 *
 *
 */
#ifndef DIRDYVTSTTASK_HH
#define DIRDYVTSTTASK_HH


#include "tdat/Fragment.H"
#include "dsm/TaskJob.H"
#include "dsm/DavCalculation.H"

class TTheory;
class TGBSConfig;


/**
 * Class providing an access architecture fo applications requiring
 * DirDyVTST Tasks.
 */
class DirDyVTSTTask : virtual public DavCalculation
{
   public:

      friend class EDSIFactory;

      enum Component 
      {
        TRANSITION, 
        REACT1, 
        REACT2, 
        PROD1, 
        PROD2,
        UNDEFINED_COMPONENT
      };

      virtual ~DirDyVTSTTask();

      // Resource virtuals
      virtual bool remove();
      virtual bool copy(EcceURL& target);
      virtual bool move(EcceURL& target);
      virtual bool canModifyInputProvider();

      // ChemistryTask virtuals
      virtual bool fragment(Fragment *frag);
      virtual bool getFragment(Fragment &frag);
      virtual Fragment* fragment() const;

      virtual Fragment* getFragment(DirDyVTSTTask::Component comp); 
      virtual string getBasis(const string& theoryType) const;
      virtual bool setFragment(Fragment *fragment,
                               DirDyVTSTTask::Component comp); 

      virtual bool anyFragmentsNew()  const;

      virtual string getParseScriptArgs() const;

      virtual string import( const string& dir, const string& file)
               throw(EcceException);

      static string componentTypeToString(DirDyVTSTTask::Component comp);
      static DirDyVTSTTask::Component stringToComponentType(const string& str);

      virtual DavCalculation *getComponentTask(DirDyVTSTTask::Component comp);

   protected:

      DirDyVTSTTask(const EcceURL& url);

      bool load(ostream& data, EcceMap& props) const;    

   private:

};


#endif
