/**
 * @file
 *
 *
 */
#ifndef TASKAPP_HH
#define TASKAPP_HH

#include <string>

using std::string;

#include "util/CommandManager.H"
#include "util/EcceException.H"

class ITaskModel;
class TaskAppCloseListener;

/**
 * Abstract base class for applications associated with ITaskModel objects.
 * This class acts as a type for all task applications (guis).
 * Each TaskApp has a CommandManager and a ITaskModel.  The latter is the
 * Receiver for commands.
 * Subclasses must instance their model in the constructor.
 */
class TaskApp
{

   public:
      TaskApp(const string& name, int maxCommands=1000);
      virtual ~TaskApp();

      virtual string getTitle() const = 0;

      ITaskModel& getModel() const;

      CommandManager& getCommandManager() const;

      void setCloseListener(TaskAppCloseListener *l);
      void notifyClose();

      virtual void setContext(const string& url) throw(EcceException);

      /** Raise app to top of desktop */
      virtual void taskAppRaise() = 0;

      /** Destroy self - not allowed to refuse. */
      virtual void taskAppDestroy() = 0;

      /** Destroy self - not allowed to refuse. */
      virtual void taskAppShow() = 0;

      /** All input fields should be changed to the specified unit family */
      virtual void setUnitFamily(const string& family) = 0;

   protected:

      CommandManager *p_cmdMgr;

      ITaskModel      *p_model;

      TaskAppCloseListener *p_closeListener;

};
#endif 
