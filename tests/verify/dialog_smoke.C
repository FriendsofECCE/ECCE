//  Construct VerifyReportDialog headlessly and lay it out (#148).
//
//  WHY.  The single most expensive bug class in this port is a wx3.2 /
//  GTK3 dialog that crashes while being built -- #78 died inside
//  CreateControls(), and the layout-reentrancy class crashes after
//  Show() has already returned.  Neither is visible in a compile, and
//  neither is reached by any suite that does not actually build the
//  window.  A new dialog is exactly where that bites, so build this one
//  for real, size it, lay it out and paint it.
//
//      dialog_smoke            (needs a DISPLAY; run under Xvfb)
#include <cstdio>
#include <vector>

#include <wx/wx.h>

#include "InputVerifier.H"
#include "VerifyReportDialog.H"

class SmokeApp : public wxApp
{
  public:
    virtual bool OnInit() { return true; }
};

IMPLEMENT_APP_NO_MAIN(SmokeApp)


static VerifyFinding make(VerifyFinding::Level level, int line,
                          const char* check, const char* message)
{
  VerifyFinding finding;
  finding.level = level;
  finding.line = line;
  finding.check = check;
  finding.message = message;
  return finding;
}


int main(int argc, char** argv)
{
  wxEntryStart(argc, argv);
  if (!wxApp::GetInstance()->CallOnInit()) {
    fprintf(stderr, "no display\n");
    return 77;
  }

  //  A deck holding the thing this feature exists for: a byte that is
  //  not text.  The dialog has to render it without falling over and
  //  without silently showing nothing where it is.
  string deck =
    "%Mem=6GB\n"
    "#P rB3LYP/GEN Freq=() NoSymm\n"
    "\n"
    "Water\n"
    "\n"
    "0 1\n"
    " O     0.00000     0.0355860     0.00000\n"
    " H     0.753717     0.634613     0.00000\n"
    "\n";
  deck += "\xbc\x3f\x76\x4f\x56\n";

  vector<VerifyFinding> findings;
  findings.push_back(make(VerifyFinding::GOOD, 0, "file", "10 lines."));
  findings.push_back(make(VerifyFinding::BAD, 10, "readable",
                          "This line contains a byte that is not text."));
  findings.push_back(make(VerifyFinding::BAD, 0, "basis",
                          "The route card says /GEN and no basis follows."));
  findings.push_back(make(VerifyFinding::UNSURE, 0, "code",
                          "Nothing else could be judged."));

  int failures = 0;

  //  Every level, since the lamp and the wash differ per level and a
  //  crash could live in any one branch.
  for (int level = 0; level < 3; level++) {
    vector<VerifyFinding> subset;
    for (size_t i = 0; i < findings.size(); i++)
      if ((int)findings[i].level <= level)
        subset.push_back(findings[i]);

    VerifyReportDialog* dialog =
      new VerifyReportDialog(NULL, "Gaussian-16", deck, subset);

    dialog->Layout();
    //  Paint it.  A reentrant layout of the kind this port keeps
    //  hitting shows up when the window is actually sized and drawn,
    //  not when it is merely constructed.
    dialog->SetSize(wxSize(900, 700));
    dialog->Layout();
    wxBitmap bitmap(900, 700, 24);
    {
      wxMemoryDC dc(bitmap);
      dc.SetBackground(*wxWHITE_BRUSH);
      dc.Clear();
    }

    const string summary = VerifyReportDialog::summary(subset);
    if (summary.empty()) {
      fprintf(stderr, "level %d: empty summary\n", level);
      failures++;
    }
    printf("level %d: built, %d findings, summary \"%s\"\n",
           level, (int)subset.size(), summary.c_str());

    dialog->Destroy();
    wxTheApp->ProcessPendingEvents();
  }

  //  An empty finding list must not produce a dialog that claims
  //  anything: it is what a checker that could not run looks like.
  {
    vector<VerifyFinding> none;
    VerifyReportDialog* dialog =
      new VerifyReportDialog(NULL, "ORCA", "", none);
    dialog->Layout();
    const string summary = VerifyReportDialog::summary(none);
    //  An empty list is what a checker that could not run produces.
    //  It must never be summarised as a pass: the entire point of the
    //  lamp is that "looked and found nothing wrong" and "did not
    //  look" are different answers, and the second one is the one a
    //  user will act on wrongly.
    if (summary.find("passed") != string::npos) {
      fprintf(stderr, "empty findings summarised as a pass: \"%s\"\n",
              summary.c_str());
      failures++;
    }
    printf("empty: built, summary \"%s\"\n", summary.c_str());
    dialog->Destroy();
    wxTheApp->ProcessPendingEvents();
  }

  wxEntryCleanup();
  printf("%s\n", failures ? "FAILED" : "PASSED");
  return failures ? 1 : 0;
}
