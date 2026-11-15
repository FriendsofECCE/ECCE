/**
 * @file
 *
 * The detail area of the combined Set-up tab (#983, Normal layout): right
 * of the Calculation Editor, it shows one thing at a time: the Theory or
 * Runtype Details (the native .dlg dialog, embedded) or, with no detail
 * chosen, the input file as Save writes it.
 */
#ifndef SETUPAREA_HH
#define SETUPAREA_HH

#include <memory>
#include <string>
#include <vector>

#include "wx/panel.h"

class CalcEd;
class wxButton;
class wxStaticText;
class wxTextCtrl;
class wxTimer;

class SetupArea : public wxPanel
{
public:
  enum Kind { INPUT, THEORY, RUNTYPE };

  SetupArea(wxWindow *parent, CalcEd *editor);
  ~SetupArea();

  // Embeds the details dialog that cmd (the dialog command line the editor
  // would run) names; false when it has no .dlg (the Python dialog runs).
  bool showDetails(Kind kind, const std::string& cmd);
  void showInput();
  // A user edit of an embedded dialog's field (test hook "setup set").
  bool setValue(const std::string& name, const std::string& value);
  // The details have no .dlg and opened as the Python dialog's window.
  void external(Kind kind);
  std::string previewShown() const;
  // The editor dropped or restarted these details (code or theory change).
  void closeDetails(Kind kind);
  // Save state changed: the preview reloads the deck.
  void statusChanged();
  Kind current() const { return p_kind; }
  wxString describe() const;
  std::string previewText() const;
  static const char *kindName(Kind k);

private:
  struct Embedded;
  CalcEd *p_ed;
  wxStaticText *p_title;
  wxStaticText *p_note;
  wxButton *p_fold;
  wxTextCtrl *p_input;
  wxWindow *p_body;
  wxTimer *p_timer;
  bool p_dry = false;   // the preview shows unsaved settings
  void dryRun();
  std::unique_ptr<Embedded> p_emb;
  Kind p_kind = INPUT;

  void drop();
  void fitBody();
  void mark();
};

#endif
