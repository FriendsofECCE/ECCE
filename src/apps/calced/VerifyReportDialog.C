#include <sstream>
using std::ostringstream;

#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/font.h>

#include "VerifyReportDialog.H"


//  Chosen to stay legible against a white background rather than for
//  saturation: a pale wash behind a whole line of monospaced text reads
//  far better than a strong one, and these still separate clearly in
//  greyscale, which is the cheapest check for a colour-blind reader.
static const wxColour BAD_BG    (255, 221, 221);
static const wxColour UNSURE_BG (255, 241, 204);
static const wxColour GOOD_BG   (223, 245, 223);
static const wxColour BAD_FG    (160,   0,   0);
static const wxColour UNSURE_FG (150,  90,   0);
static const wxColour GOOD_FG   (  0, 110,   0);


static const char* levelWord(VerifyFinding::Level level)
{
  switch (level) {
    case VerifyFinding::BAD:    return "Wrong";
    case VerifyFinding::UNSURE: return "Unsure";
    default:                    return "Checked";
  }
}


string VerifyReportDialog::summary(const vector<VerifyFinding>& findings)
{
  int bad = 0, unsure = 0, good = 0;
  for (size_t i = 0; i < findings.size(); i++) {
    if      (findings[i].level == VerifyFinding::BAD)    bad++;
    else if (findings[i].level == VerifyFinding::UNSURE) unsure++;
    else                                                 good++;
  }

  ostringstream text;
  if (bad) {
    text << bad << (bad == 1 ? " problem" : " problems")
         << " that will stop this calculation running";
  } else if (unsure) {
    text << "Nothing wrong found, but " << unsure
         << (unsure == 1 ? " thing" : " things")
         << " could not be judged";
  } else if (good == 0) {
    //  Nothing was checked at all.  This is what a checker that could
    //  not run looks like, and it must NOT come out as a pass -- the
    //  whole value of the light is that it distinguishes "looked and
    //  found nothing wrong" from "did not look".
    text << "Nothing was checked";
  } else {
    //  Deliberately not "the input is correct".  What was checked is
    //  the shape of the deck; whether the chemistry is right is a
    //  different question and this must never look like an answer to
    //  it.
    if (good == 1) text << "The one structural check passed";
    else           text << "All " << good << " structural checks passed";
  }
  return text.str();
}


VerifyReportDialog::VerifyReportDialog(wxWindow* parent,
                                       const string& codeName,
                                       const string& deck,
                                       const vector<VerifyFinding>& findings)
  : ewxDialog(parent, wxID_ANY, wxString(("Verify " + codeName +
                                          " Input").c_str(), wxConvUTF8),
              wxDefaultPosition, wxSize(860, 680),
              wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
  build(codeName, deck, findings);
}


void VerifyReportDialog::build(const string& codeName,
                               const string& deck,
                               const vector<VerifyFinding>& findings)
{
  wxBoxSizer* top = new wxBoxSizer(wxVERTICAL);

  const VerifyFinding::Level worst = InputVerifier::worst(findings);

  wxStaticText* heading =
    new wxStaticText(this, wxID_ANY,
                     wxString(summary(findings).c_str(), wxConvUTF8));
  wxFont bold = heading->GetFont();
  bold.SetWeight(wxFONTWEIGHT_BOLD);
  heading->SetFont(bold);
  heading->SetForegroundColour(worst == VerifyFinding::BAD    ? BAD_FG :
                               worst == VerifyFinding::UNSURE ? UNSURE_FG :
                                                                GOOD_FG);
  top->Add(heading, 0, wxALL, 8);

  //  Say what this does and does not cover, every time, where the
  //  answer is.  A user who reads "all checks passed" once and takes
  //  it to mean the calculation is right will not go looking for the
  //  caveat later.
  wxStaticText* scope =
    new wxStaticText(this, wxID_ANY,
      "These checks look at the shape of the input file -- whether the "
      "sections a deck needs are present and readable. They say nothing "
      "about whether the theory, basis set or keywords suit what you are "
      "trying to calculate.");
  scope->Wrap(820);
  top->Add(scope, 0, wxLEFT|wxRIGHT|wxBOTTOM, 8);

  //  ---------------------------------------------------- the findings
  wxTextCtrl* list =
    new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxSize(-1, 150),
                   wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2);
  list->SetFont(wxFont(wxFontInfo().Family(wxFONTFAMILY_TELETYPE)));

  for (size_t i = 0; i < findings.size(); i++) {
    const VerifyFinding& finding = findings[i];

    ostringstream row;
    row << levelWord(finding.level);
    if (finding.line > 0) row << " (line " << finding.line << ")";
    row << ": " << finding.message << "\n";

    const long from = list->GetLastPosition();
    list->AppendText(wxString(row.str().c_str(), wxConvUTF8));
    list->SetStyle(from, list->GetLastPosition(),
                   wxTextAttr(finding.level == VerifyFinding::BAD    ? BAD_FG :
                              finding.level == VerifyFinding::UNSURE ? UNSURE_FG
                                                                     : GOOD_FG));
  }
  top->Add(list, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 8);

  //  -------------------------------------------- the deck, marked up
  top->Add(new wxStaticText(this, wxID_ANY, "The input file:"),
           0, wxLEFT|wxBOTTOM, 8);

  wxTextCtrl* text =
    new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                   wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2 |
                   wxTE_DONTWRAP);
  text->SetFont(wxFont(wxFontInfo(10).Family(wxFONTFAMILY_TELETYPE)));

  //  Which lines each finding covers, and at what level.
  //
  //  Four states, not three, and the fourth is the important one: a
  //  line NO check covered is shown as not-known, the same amber as
  //  an explicit "unsure".  So the colours say how much of this file
  //  the checker actually understands, instead of leaving everything
  //  it never looked at indistinguishable from everything it
  //  approved.  Green here means "this line was examined and is
  //  right", never "nothing was found wrong with it".
  //
  //  A line may be covered by more than one finding; the worst wins.
  const int UNKNOWN = -1;
  const int OK      = 0;
  const int UNSURE  = 1;
  const int WRONG   = 2;

  vector<int> lineLevel;
  for (size_t i = 0; i < findings.size(); i++) {
    if (findings[i].line <= 0) continue;
    const int last = (findings[i].lineEnd > 0) ? findings[i].lineEnd
                                               : findings[i].line;
    for (int line = findings[i].line; line <= last; line++) {
      const size_t index = line - 1;
      if (lineLevel.size() <= index)
        lineLevel.resize(index + 1, UNKNOWN);
      const int level = (findings[i].level == VerifyFinding::BAD)    ? WRONG :
                        (findings[i].level == VerifyFinding::UNSURE) ? UNSURE
                                                                     : OK;
      if (level > lineLevel[index]) lineLevel[index] = level;
    }
  }

  size_t start = 0;
  int number = 0;
  while (start <= deck.size()) {
    const size_t end = deck.find('\n', start);
    string line = deck.substr(start, (end == string::npos)
                                     ? string::npos : end - start);

    //  A byte that is not text is why this dialog exists; it must be
    //  SEEN, and a control that renders it as nothing hides the one
    //  thing worth showing.  Write each such byte as its hex value.
    string shown;
    for (size_t i = 0; i < line.size(); i++) {
      const unsigned char byte = (unsigned char)line[i];
      if (byte == '\t' || (byte >= 0x20 && byte <= 0x7e)) {
        shown += (char)byte;
      } else {
        char hex[8];
        snprintf(hex, sizeof(hex), "<%02x>", byte);
        shown += hex;
      }
    }

    int level = ((size_t)number < lineLevel.size()) ? lineLevel[number]
                                                    : UNKNOWN;
    //  A blank line belongs to whatever surrounds it and is never
    //  interesting on its own; marking them amber would fill the
    //  margin with noise about separators.
    if (line.find_first_not_of(" \t\r") == string::npos && level == UNKNOWN)
      level = OK;

    //  The margin mark, so the classification survives being printed,
    //  screenshotted in greyscale, or read by someone who does not see
    //  the wash behind it.
    char margin[16];
    snprintf(margin, sizeof(margin), "%s%4d  ",
             (level == WRONG)   ? ">>" :
             (level == UNSURE)  ? " ?" :
             (level == OK)      ? " +" : "  ",
             number + 1);

    const long from = text->GetLastPosition();
    text->AppendText(wxString((margin + shown + "\n").c_str(), wxConvUTF8));
    if (level != UNKNOWN) {
      wxTextAttr attr;
      attr.SetBackgroundColour(level == WRONG  ? BAD_BG :
                               level == UNSURE ? UNSURE_BG : GOOD_BG);
      text->SetStyle(from, text->GetLastPosition(), attr);
    }

    number++;
    if (end == string::npos) break;
    start = end + 1;
  }
  text->SetInsertionPoint(0);

  top->Add(text, 1, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 8);

  wxStaticText* key =
    new wxStaticText(this, wxID_ANY,
      "  +  checked and right        ?  could not be judged        "
      ">>  wrong        (unmarked lines were not checked)");
  key->SetFont(wxFont(wxFontInfo(9).Family(wxFONTFAMILY_TELETYPE)));
  top->Add(key, 0, wxLEFT|wxRIGHT|wxBOTTOM, 8);

  wxSizer* buttons = CreateButtonSizer(wxOK);
  if (buttons)
    top->Add(buttons, 0, wxEXPAND|wxALL, 8);

  SetSizer(top);
  Layout();
}
