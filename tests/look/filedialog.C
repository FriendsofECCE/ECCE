//  Construct ewxGenericFileDialog on the local filesystem, list a directory
//  under each filter and print the layout of its top row.
//
//      filedialog DIR WILDCARD WIDTHxHEIGHT|default HOLD_MS   (needs a DISPLAY)
//
//  Output: "FILTER <n> <label>" then "  <name>" per listed file, then
//  "ROW <class> x y w h" for the widgets above the file list.
#include <cstdio>
#include <cstdlib>
#include <string>
#include <unistd.h>

#include <wx/wx.h>
#include <wx/listctrl.h>

#include "wxgui/ewxFileCtrl.H"
#include "wxgui/ewxGenericFileDialog.H"

class App : public wxApp
{
  public:
    virtual bool OnInit() { return true; }
    virtual void OnAssertFailure(const wxChar*, int, const wxChar*,
                                 const wxChar*, const wxChar*) {}
};

IMPLEMENT_APP_NO_MAIN(App)

class Dlg : public ewxGenericFileDialog
{
  public:
    Dlg(const wxString& wild)
    {
      Create(NULL, "Open", "", "", wild, wxFD_OPEN|wxFD_FILE_MUST_EXIST);
      if (getenv("FD_RESTORE")) {  // as the app dialogs do after Create
        restoreSettings();
        printf("RESTORED %s\n", (const char*) m_list->GetDir().utf8_str());
        fprintf(stderr, "m_dir=%s\n", (const char*) m_dir.utf8_str());
      }
    }

    void listIn(const wxString& dir, const wxSize& size, long hold,
                int ntyped = 0, char** typed = NULL)
    {
      setServerChoice(0);
      m_list->GoToDir(dir);
      UpdateControls();
      if (size.x > 0) SetSize(size);
      Show();
      for (unsigned i = 0; i < m_choice->GetCount(); i++) {
        m_choice->SetSelection(i);
        wxCommandEvent ev(wxEVT_CHOICE, m_choice->GetId());
        ev.SetInt(i);
        ev.SetEventObject(m_choice);
        m_choice->GetEventHandler()->ProcessEvent(ev);
        printf("FILTER %u %s\n", i,
               (const char*) m_choice->GetString(i).utf8_str());
        for (int n = 0; n < m_list->GetItemCount(); n++)
          printf("  %s\n", (const char*) m_list->GetItemText(n).utf8_str());
      }
      // Programmatic path (as WxCalcImport/CalcMgr use): the displayed choice
      // and the list must both follow.
      SetFilterIndex(m_choice->GetCount() - 1);
      SetFilterIndex(0);
      printf("FINAL %s\n", (const char*) m_choice->GetStringSelection().utf8_str());
      for (int n = 0; n < m_list->GetItemCount(); n++)
        printf("  %s\n", (const char*) m_list->GetItemText(n).utf8_str());
      Layout();
      int listY = m_list->GetRect().y;
      for (wxWindowList::iterator it = GetChildren().begin();
           it != GetChildren().end(); ++it) {
        wxRect r = (*it)->GetRect();
        if (r.y < listY)
          printf("ROW %s x=%d y=%d w=%d h=%d \"%s\"\n",
                 (const char*) (*it)->GetClassInfo()->GetClassName(),
                 r.x, r.y, r.width, r.height,
                 (const char*) (*it)->GetLabel().utf8_str());
      }
      printf("DIALOG w=%d h=%d min_w=%d\n", GetSize().x, GetSize().y,
             GetMinSize().x);
      fflush(stdout);
      // Typed names, as if entered in "File name:" and OK pressed (the
      // dialog is not modal here, so EndModal only hides it).
      for (int i = 0; i < ntyped; i++) {
        SetPath("");
        wxString t = wxString::FromUTF8(typed[i]);
        m_list->GoToDir(dir);
        m_text->SetValue(t);
        HandleAction(t);
        printf("TYPED %s => %s\n", typed[i],
               (const char*) GetPath().utf8_str());
        Show();
      }
      fflush(stdout);
      wxStopWatch sw;
      while (sw.Time() < hold) { wxYield(); wxMilliSleep(20); }
    }
};

int main(int argc, char** argv)
{
  wxEntryStart(argc, argv);
  if (!wxApp::GetInstance()->CallOnInit() || argc < 5) return 77;
  wxSize size(0, 0);
  int w, h;
  if (sscanf(argv[3], "%dx%d", &w, &h) == 2) size = wxSize(w, h);
  Dlg* dlg = new Dlg(argv[2]);
  dlg->listIn(argv[1], size, atol(argv[4]), argc - 5, argv + 5);
  _exit(0);  // skip teardown, which segfaults headless
}
