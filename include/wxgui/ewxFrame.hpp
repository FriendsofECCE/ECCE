#ifndef _EWXFRAME_H  
#define _EWXFRAME_H  

#include "util/Preferences.H"
#include "wx/frame.h"
#include "wxgui/ewxStyledWindow.H"

class ewxConfig;

/**
 * Subclass wxFrame to provide style changes.
 */
class ewxFrame : public wxFrame, public ewxStyledWindow 
{

   public:

      ewxFrame();
      ewxFrame(wxWindow* parent, wxWindowID id, const wxString& title,
               const wxPoint& pos = wxDefaultPosition,
               const wxSize& size = wxDefaultSize,
               long style = wxDEFAULT_FRAME_STYLE,
               const wxString& name = "ECCE");
      bool Create(wxWindow* parent, wxWindowID id, const wxString& title,
                  const wxPoint& pos = wxDefaultPosition,
                  const wxSize& size = wxDefaultSize,
                  long style = wxDEFAULT_FRAME_STYLE,
                  const wxString& name = "ECCE");
      virtual ~ewxFrame();

      virtual bool Show(bool show = true);

      virtual void saveWindowSettings(ewxConfig * config, bool saveSize);
      virtual void restoreWindowSettings(ewxConfig * config, bool restoreSize);

      virtual void saveWindowSettings(const string& prefix, 
                                      Preferences & prefs, bool saveSize);
      virtual bool restoreWindowSettings(const string& prefix, 
                                      Preferences & prefs, bool restoreSize);

      virtual void setUnitFamily(const string& family);

      virtual void setFrameCustomDisabledStyle(bool enabled);

};

#endif
