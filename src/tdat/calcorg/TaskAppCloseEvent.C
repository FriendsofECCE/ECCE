/**
 * @file
 *
 *
 */

#include "tdat/TaskAppCloseEvent.H"
#include "tdat/TaskApp.H"



/**
 * Constructor
 */
TaskAppCloseEvent::TaskAppCloseEvent(TaskApp *closedApp)
  : p_taskApp(closedApp)
{
}


/**
 * Destructor
 */
TaskAppCloseEvent::~TaskAppCloseEvent()
{
   p_taskApp = 0;
}


TaskApp *TaskAppCloseEvent::getTaskApp() const
{
   return p_taskApp;
}

