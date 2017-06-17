#ifndef _EWXTOOL_H  
#define _EWXTOOL_H  

#include "wx/statusbr.h"
#include "wx/frame.h"

#include "dsm/ResourceDescriptor.H"

#include "wxgui/ewxFrame.H"
#include "wxgui/CalcDropHandler.H"

class FeedbackSaveHandler;
class ewxStatusBar;

/**
 * Subclass ewxFrame to provide a common interface to ecce tools that have
 * a dav context.
 *
 * Tools that take a single data url context should subclass ewxTool and
 * implement its interface so all tools have the same structure and are easier
 * to maintain between individual developers.
 *
 * No methods are pure virtual because inheritance is done from the *GUI class
 * which we don't want to edit.  Instead read the method descriptions to
 * see which ones you should override.
 *
 * Things we might add: 
 *  . messaging methods, 
 *  . preference saving/restoring methods.
 */
class ewxTool : public ewxFrame, public CalcDropHandler
{

public:

  ewxTool();

  ewxTool(wxWindow* parent, wxWindowID id, const wxString& title,
          const wxPoint& pos = wxDefaultPosition,
          const wxSize& size = wxDefaultSize,
          long style = wxDEFAULT_FRAME_STYLE,
          const wxString& name = "ECCE");

  virtual ~ewxTool();

  virtual bool Create(wxWindow* parent, wxWindowID id, const wxString& title,
                      const wxPoint& pos = wxDefaultPosition,
                      const wxSize& size = wxDefaultSize,
                      long style = wxDEFAULT_FRAME_STYLE,
                      const wxString& name = "ECCE");

  virtual wxStatusBar* CreateStatusBar();


  virtual void setEditStatus(const string & editStatus);
  virtual void setRunState(long state);
  virtual void showRunState(bool show);

  /**
   * Set context of tool to the url.
   * The default implementation simply sets the context state variable.
   * Tools should override this method.
   */
  virtual void setContext(const string& url);

  virtual string getEditStatus() const;
  virtual long getRunState() const;
  /**
   * Get the tools context.
   * Tools probably don't need to override this method.
   */
  virtual string getContext() const;

  /**
   * The CalcDropHandler method.
   * setContext is automatically called.
   */
  virtual void processDrop(const string &url);

  /**
   * Destroy the window.
   * Override to do prompting before quitting, etc.
   */
  virtual void quit();

  virtual void saveSettings();

  virtual void restoreSettings();

  virtual void setSaveHandler(FeedbackSaveHandler *handler);

  virtual void showCalcInfo(ResourceDescriptor::RUNSTATE state, bool writable,
                            const string& name);

protected:

  string p_context;
  ewxStatusBar * p_statusbar;

};

#endif
