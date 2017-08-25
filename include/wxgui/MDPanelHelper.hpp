/**
 * @file
 *
 *
 */

#ifndef _MDPANELHELPER_H
#define _MDPANELHELPER_H

#include "dsm/ResourceDescriptor.H"

class WxFeedback;

/**
 * Interface for application frames that will create a set of
 * NWChem MD panels.  We need a common ancestor to do things like communicate
 * changes (save icon and menu item), access feedback....
 */
class MDPanelHelper 
{

   public:

      virtual void setSaveState(bool unsavedChanges) = 0;

      virtual void setRunState(ResourceDescriptor::RUNSTATE state) = 0;

      virtual bool isUnsavedState() = 0;

      virtual WxFeedback& getFeedback() = 0;

      virtual void notifyChange() = 0;

};

#endif
