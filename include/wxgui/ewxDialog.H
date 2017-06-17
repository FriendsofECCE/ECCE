#ifndef _EWXDIALOG_H  
#define _EWXDIALOG_H  

#include "util/Preferences.H"
#include "wx/dialog.h"
#include "wxgui/ewxStyledWindow.H"

class ewxConfig;

/**
 * An ecce stylized dialog.
 * This class provides ecce colors.
 */
//It also has other convenience behaviors
//such as the capability to be used as a basic shell for any panel you want
//to show as a dialog.   See setContents for more information.
class ewxDialog : public wxDialog, public ewxStyledWindow
{
   public:
      //enum { ID_BUTTON_EWXDIALOG_OK=20101, ID_BUTTON_EWXDIALOG_CANCEL };

      ewxDialog();
      ewxDialog(wxWindow* parent, wxWindowID id, const wxString& title,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = wxDEFAULT_DIALOG_STYLE,
                const wxString& name = "dialogBox");
      bool Create(wxWindow* parent, wxWindowID id, const wxString& title,
             const wxPoint& pos = wxDefaultPosition,
             const wxSize& size = wxDefaultSize,
             long style = wxDEFAULT_DIALOG_STYLE,
             const wxString& name = "dialogBox");
      virtual ~ewxDialog();

      /*
      virtual void setContents(wxWindow *panel, bool allowClose=true);

      virtual wxWindow *getContents() const;

      virtual void OnCloseWindow( wxCloseEvent& event );

      virtual void OnCloseButton( wxCloseEvent& event );

      virtual void OnOkButton( wxCloseEvent& event );
      */

      virtual void saveWindowSettings(ewxConfig * config, bool saveSize);
      virtual void restoreWindowSettings(ewxConfig * config, bool restoreSize);

      virtual bool restoreWindowSettings(const string& prefix, 
            Preferences & prefs, bool restoreSize);

      virtual void saveWindowSettings(const string& prefix, 
            Preferences & prefs, bool saveSize);

      virtual void setUnitFamily(const string& family);

   protected:
      /* pointer to user supplied window associated with setContents. */
      //wxWindow *p_contents;

   private:
      // common part of all ctors
      void Init();
};

#endif
