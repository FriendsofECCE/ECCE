/**
 * @file
 *
 *
 */

#include "tdat/TaskAppCloseEvent.hpp"
#include "tdat/TaskApp.hpp"



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

