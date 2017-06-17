#ifndef WXVIZTOOLFW_H
#define WXVIZTOOLFW_H

#include <string>

using std::string;

#include "util/CommandManager.H"
#include "util/Command.H"

#include "viz/SGFragment.H"
#include "viz/SGContainer.H"

#include "wxviz/SGViewer.H"

/**
 * Interface representing the controller class (core) of a viz application.
 * It provides access to key viz data structures such as the scenegraph, 
 * viewer, and feedback area as well as common operations, without exposing
 * all of its implementation.
 * Each viz application will have one class that implements this.  Each
 * viz toolkit will implement WxVizTool.
 *
 * This design provides relatively loose coupling between viz tools and
 * enables them to be used in multiple viz applications.
 * @see WxVizTool
 */
class WxVizToolFW
{
   public:

      virtual CommandManager&  getCommandManager() = 0;

      virtual SGContainer&      getSceneGraph() = 0;

      /* Get the Viewer . */
      virtual SGViewer& getViewer() = 0;

      /* Display message to the user. */
      virtual void showMessage(const string& msg, bool error=false) = 0;

      /* Centers the viz display. */
      virtual void center() = 0;

      /* returns true if each mode is auto-centering */
      virtual bool isAutoCenter() const = 0;

      /* returns true if context should be treated as readonly */
      virtual bool isReadOnly() const = 0;

      /* execute a command and do post command processing */
      virtual bool execute(Command *cmd, bool batch=false) = 0;

      virtual bool areLabelsOn() const = 0;

};

#endif
