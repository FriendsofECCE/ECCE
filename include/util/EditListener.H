/**
 * @file 
 *
 *
 */
#ifndef EDITLISTENER_HH
#define EDITLISTENER_HH

class EditEvent;

/**
 * An EditListener can be notified by when a user editor session
 * completes or when the user writes changes.
 * @see EditEvent
 * Known users include TEditFile and WxEditFile.
 */
class EditListener
{
   public:

      virtual void processEditCompletion(const EditEvent& ee) = 0;
};

#endif
