/**
 * @file
 *
 *
 */
#ifndef TASKAPPCLOSELISTENER_HH
#define TASKAPPCLOSELISTENER_HH

class TaskAppCloseEvent;

/**
 * An abstract listener for TaskAppCloseEvents.
 *
 * Part of the observer/observable design pattern for communicating closing
 * of TaskApp objects.
 */
class TaskAppCloseListener
{
   public:

   /** Event sent when TaskApp actually closes. */
   virtual void taskAppClosed(const TaskAppCloseEvent& event) = 0;

};
#endif 
