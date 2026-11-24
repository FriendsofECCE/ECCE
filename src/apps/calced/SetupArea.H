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

class BasisPanel;
class LaunchEmbed;
class CalcEd;
class wxButton;
class wxStaticText;
class wxTextCtrl;
class wxTimer;

class SetupArea : public wxPanel
{
public:
  enum Kind { INPUT, THEORY, RUNTYPE, BASIS, LAUNCH, OUTPUT };

  SetupArea(wxWindow *parent, CalcEd *editor);
  ~SetupArea();

  // Embeds the details dialog that cmd (the dialog command line the editor
  // would run) names; false when it has no .dlg (the Python dialog runs).
  bool showDetails(Kind kind, const std::string& cmd);
  void showInput();
  // The job's primary output, read-only; it follows a running job.
  void showOutput();
  // The calculation's state changed: the output view reloads.
  void outputStateChanged();
  // The basis panel (BasisPanel) in place of the details; test hooks follow.
  void showBasis();
  // The Launcher's panel (LaunchEmbed.H); false when the Launcher is not
  // linked in, and Launch... opens the tool as before.
  bool showLaunch();
  std::string launchGet(const std::string& what);
  bool launchGo();
  void basisFilter(const std::string& text);
  bool basisPick(const std::string& name);
  bool basisUseAll();
  // kind: 0 orbital set, 1 ECP, 2 fitting set, 3 augmenting set; empty name with kind 1
  // accepts the offered matching ECP.  -1: no such set.
  int basisAssign(const std::string& name, int kind,
                  const std::vector<std::string>& elements);
  // Several sets of KIND in one step, each on its own targets (BasisPanel).
  bool basisAssignMany(int kind, const std::vector<std::pair<std::string,
                       std::vector<std::string> > >& steps);
  bool basisAtoms(const std::string& names);
  bool basisAppend(bool tight, const std::string& element, int shell,
                   int count);
  void basisChanged();
  // Contractions of EL: ROW < 0 only summarises, else uncontract|delete|keep
  // that row.  OUT is the summary, or the reason when false.
  bool basisContraction(const std::string& el, int row,
                        const std::string& action, std::string& out);
  std::string basisCoefficients();
  // A user edit of an embedded dialog's field (test hook "setup set").
  bool setValue(const std::string& name, const std::string& value);
  // The details have no .dlg and opened as the Python dialog's window.
  void external(Kind kind);
  // Those details' window closed: the note no longer says it is open.
  void externalClosed(Kind kind);
  // The note above the preview, as shown ("" when hidden; test hook).
  wxString note() const;
  std::string previewShown() const;
  // The editor dropped or restarted these details (code or theory change).
  void closeDetails(Kind kind);
  // Save state changed: the preview reloads the deck.
  void statusChanged();
  Kind current() const { return p_kind; }
  // Folds this area's content away to its header button (the editor column
  // keeps its width); opening any view unfolds it.
  void fold(bool on);
  bool folded() const { return !p_foldHidden.empty(); }
  wxString describe() const;
  std::string previewText() const;
  static const char *kindName(Kind k);

private:
  struct Embedded;
  CalcEd *p_ed;
  wxStaticText *p_title;
  wxStaticText *p_note;
  // Details open in their own window (INPUT when none).
  Kind p_external = INPUT;
  wxButton *p_fold;
  std::vector<wxWindow*> p_foldHidden;   // what fold() hid, to show again
  wxTextCtrl *p_input;
  wxWindow *p_body;
  wxTimer *p_timer;
  wxTimer *p_outTimer;       // reloads the output of a running job
  wxButton *p_openEditor;    // the output in the external editor
  bool loadOutput(bool first);
  bool p_dry = false;   // the preview shows unsaved settings
  void dryRun();
  std::unique_ptr<Embedded> p_emb;
  BasisPanel *p_basisPanel = 0;
  std::unique_ptr<LaunchEmbed> p_launch;
  Kind p_kind = INPUT;

  void drop();
  void fitBody();
  void mark();
};

#endif
