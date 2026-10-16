#ifndef _EWXAPP_H  
#define _EWXAPP_H  

#include <string>
  using std::string;

#include "wx/app.h"
#include "wx/event.h"


/**
 * Subclass wxApp to provide common Ecce application behavior.
 * This includes things like setting vendor name, requesting best
 * available visual etc etc.
 *
 * All ecce main classes should subclass this class.
 */
class ewxApp : public wxApp
{

   public:

      virtual ~ewxApp();

      virtual bool OnInit();

      virtual string getName() const { return "noname"; }

      virtual void processGlobalPreferenceChange();

      // Help > About ECCE...; reaches the app because unhandled menu
      // events propagate from the frame to wxTheApp.
      void OnAbout(wxCommandEvent& event);

      // Help > Report a Problem...; the Help menus append this ID and it
      // is handled here, the same way as About.
      static const int ID_REPORT_PROBLEM = wxID_HIGHEST + 7001;
      void OnReportProblem(wxCommandEvent& event);

      // GTK3: keep text at normal contrast in a window that has lost
      // focus (the theme's :backdrop state fades it), except where the
      // widget is disabled.  Called from OnInit; ECCE_BACKDROP=theme keeps
      // the theme's fading.  No-op elsewhere.
      static void applyBackdropStyle();

#ifdef __WXMAC__
      // A click on the tool's Dock icon brings its windows to the front.
      void MacReopenApp() override;
#endif


};

#endif
