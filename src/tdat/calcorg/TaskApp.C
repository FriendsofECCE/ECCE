/**
 * @file
 *
 *
 */

#include "util/EcceException.H"

#include "tdat/TaskApp.H"
#include "tdat/TaskAppCloseEvent.H"
#include "tdat/TaskAppCloseListener.H"



/**
 * Constructor
 */
TaskApp::TaskApp(const string& name, int maxCommands)
{
   p_cmdMgr = new CommandManager(name, maxCommands);
   p_model = 0;
   p_closeListener = 0;
}


/**
 * Destructor
 */
TaskApp::~TaskApp()
{
   delete p_cmdMgr;
   p_closeListener = 0;
}


CommandManager& TaskApp::getCommandManager() const
{
   return *p_cmdMgr;
}



ITaskModel& TaskApp::getModel() const
{
   return (ITaskModel&)*p_model;
}


void TaskApp::setContext(const string& url) throw(EcceException)
{
}


/**
 * Set the close listener for this app.
 * Use a 0 pointer to remove the listener.
 */
void TaskApp::setCloseListener(TaskAppCloseListener *l)
{
   p_closeListener = l;
}



/**
 * Notify the close listener of a close event.
 */
void TaskApp::notifyClose()
{
   if (p_closeListener != 0) {
      p_closeListener->taskAppClosed(TaskAppCloseEvent(this));
   }
}
