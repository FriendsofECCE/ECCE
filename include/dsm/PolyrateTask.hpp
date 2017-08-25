/**
 * @file
 *
 *
 */
#ifndef POLYRATETASK_HH
#define POLYRATETASK_HH


#include "tdat/Fragment.H"
#include "dsm/TaskJob.H"
#include "dsm/DavCalculation.H"
#include "dsm/DirDyVTSTTask.H"

class TTheory;
class TGBSConfig;

/**
 * Class providing an access architecture for applications requiring
 * Polyrate Tasks.
 */
class PolyrateTask : virtual public DavCalculation
{
   public:

      friend class EDSIFactory;

      virtual ~PolyrateTask();

      virtual string getParseScriptArgs() const;

      virtual bool canModifyInputProvider();

      virtual string import( const string& dir, const string& file)
               throw(EcceException);

      virtual bool remove();
      virtual bool copy(EcceURL& target);
      virtual bool move(EcceURL& target);

      static string componentTypeToString(DirDyVTSTTask::Component comp);
      static DirDyVTSTTask::Component stringToComponentType(const string& str);

      virtual DirDyVTSTTask *getDirdyTask();

   protected:

      PolyrateTask(const EcceURL& url);

      bool load(ostream& data, EcceMap& props) const;    

   private:

};


#endif
