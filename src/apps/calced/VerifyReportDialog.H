/**
 * @file
 *
 * What the input checker found, shown over the deck it found it in
 * (GitHub #148).
 *
 * The findings alone are not much use: "no basis block" means little
 * until you can see the route card that promised one.  So this shows
 * the deck with the lines a finding points at marked in place, and the
 * findings listed beside it.
 *
 * On the colours, which carry the whole meaning and so must not be the
 * only thing carrying it: every finding is also written out in words,
 * and each line is prefixed in the margin, so the dialog still reads
 * correctly to someone who cannot distinguish red from green -- about
 * one man in twelve, and a chemistry cohort is not an exception.
 */

#ifndef VERIFYREPORTDIALOG_HH
#define VERIFYREPORTDIALOG_HH

#include <vector>
using std::vector;

#include "wxgui/ewxDialog.H"
#include "InputVerifier.H"

class ewxTextCtrl;


class VerifyReportDialog : public ewxDialog
{
  public:

    VerifyReportDialog(wxWindow* parent,
                       const string& codeName,
                       const string& deck,
                       const vector<VerifyFinding>& findings);

    /** A one-line summary of a set of findings, for the panel. */
    static string summary(const vector<VerifyFinding>& findings);

  private:

    void build(const string& codeName,
               const string& deck,
               const vector<VerifyFinding>& findings);
};

#endif
