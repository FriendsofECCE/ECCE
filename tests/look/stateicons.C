//  Draw every run-state icon with WxState::draw at the sizes the app uses
//  (tree image list 16, legend, status bar) and save raw PNGs, for
//  stateicons.py.
//
//      stateicons OUTDIR [flat|shaded]   (needs a DISPLAY; GTK_THEME picks the theme)
#include <cstdio>
#include <string>
#include <unistd.h>

#include <wx/wx.h>
#include <wx/dcmemory.h>

#include "util/PreferenceLabels.H"
#include "util/Preferences.H"
#include "wxgui/WxState.H"

class App : public wxApp
{
  public:
    virtual bool OnInit() { return true; }
    virtual void OnAssertFailure(const wxChar*, int, const wxChar*,
                                 const wxChar*, const wxChar*) {}
};

IMPLEMENT_APP_NO_MAIN(App)

int main(int argc, char** argv)
{
  wxEntryStart(argc, argv);
  if (!wxApp::GetInstance()->CallOnInit() || argc < 2) return 77;
  wxInitAllImageHandlers();
  //  Optional style: saved as the preference, as the dialog does.
  if (argc >= 3) {
    {
      Preferences prefs(PrefLabels::GLOBALPREFFILE);
      prefs.setString(PrefLabels::STATEICONSTYLE, argv[2]);
      prefs.saveFile();
    }
    WxState::refreshStyle();
  }
  int sizes[] = { 12, 16, 20, 24 };
  for (int size : sizes) {
    for (int s = ResourceDescriptor::STATE_CREATED;
         s < ResourceDescriptor::NUMBER_OF_STATES; s++) {
      wxBitmap bmp(size, size);
      {
        wxMemoryDC dc(bmp);
        dc.SetBackground(*wxWHITE_BRUSH);
        WxState::draw(dc, (ResourceDescriptor::RUNSTATE)s);
      }
      std::string f = std::string(argv[1]) + "/icon-" + std::to_string(size) +
                      "-" + std::to_string(s) + ".png";
      bmp.SaveFile(f, wxBITMAP_TYPE_PNG);
      //  The bitmap the image lists hold: real alpha, no white halo.
      f = std::string(argv[1]) + "/alpha-" + std::to_string(size) + "-" +
          std::to_string(s) + ".png";
      WxState::alphaBitmap((ResourceDescriptor::RUNSTATE)s, size)
          .SaveFile(f, wxBITMAP_TYPE_PNG);
    }
  }
  _exit(0);
}
