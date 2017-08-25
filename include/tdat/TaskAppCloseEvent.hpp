/**
 * @file
 *
 *
 */
#ifndef TASKAPPCLOSEEVENT_H
#define TASKAPPCLOSEEVENT_H

class TaskApp;


/**
 * An event to communicate the closing of a TaskApp instance.
 */
class TaskAppCloseEvent
{
   public:
     TaskAppCloseEvent(TaskApp *app);
     virtual ~TaskAppCloseEvent();

     TaskApp  *getTaskApp() const;

   protected:
     TaskApp *p_taskApp;

};
#endif 
