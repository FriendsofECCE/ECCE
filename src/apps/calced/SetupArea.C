/**
 * @file
 *
 * See SetupArea.H and docs/claude/services/tools-as-tabs.md.
 */
#include <fstream>
#include <sstream>

#include "wx/wx.h"
#include "wx/statline.h"
#include "wx/tglbtn.h"

#include "dsm/ICalculation.H"
#include "dsm/JCode.H"
#include "dsm/ResourceDescriptor.H"
#include "util/StringConverter.H"
#include "wxgui/ToolTabs.H"
#include "wxgui/WxEditSessionMgr.H"
#include "wxgui/WxFeedback.H"

#include "DetailsPanel.h"

#include "CalcEd.H"
#include "CalcEdGUI.H"
#include "SetupArea.H"
#include "BasisPanel.H"
#include "wxgui/LaunchEmbed.H"

using std::string;
using std::vector;
using std::istream;

namespace {

// A dialog command line as JCode/CalcEd build it: words, "quoted" words.
vector<string> splitCommand(const string& cmd)
{
  vector<string> out;
  string cur;
  bool quoted = false, any = false;
  for (char c : cmd) {
    if (c == '"') { quoted = !quoted; any = true; continue; }
    if (!quoted && (c == ' ' || c == '\t')) {
      if (any) out.push_back(cur);
      cur.clear();
      any = false;
      continue;
    }
    cur += c;
    any = true;
  }
  if (any) out.push_back(cur);
  return out;
}

bool endsWith(const string& s, const string& e)
{
  return s.size() >= e.size() && s.compare(s.size() - e.size(), e.size(), e) == 0;
}

// The lines the dialog sends reach the editor as UDP datagrams would: after
// the current event, in order.
class EditorTransport : public details::Transport
{
public:
  EditorTransport(CalcEd *ed, bool theory) : p_ed(ed), p_theory(theory) {}
  void send(const string& line) override
  {
    CalcEd *ed = p_ed;
    bool theory = p_theory;
    ed->CallAfter([ed, theory, line]() {
      ed->detailsInput(theory, line.c_str());
    });
  }
private:
  CalcEd *p_ed;
  bool p_theory;
};

}  // namespace


struct SetupArea::Embedded {
  details::Description desc;
  std::unique_ptr<details::Model> model;
  std::unique_ptr<EditorTransport> transport;
  wxPanel *host = 0;
  details::DetailsPanel *panel = 0;
};


const char *SetupArea::kindName(Kind k)
{
  return k == THEORY ? "theory" : k == RUNTYPE ? "runtype" :
         k == BASIS ? "basis" : k == LAUNCH ? "launch" :
         k == OUTPUT ? "output" : "input";
}


SetupArea::SetupArea(wxWindow *parent, CalcEd *editor)
  : wxPanel(parent, wxID_ANY), p_ed(editor)
{
  wxBoxSizer *top = new wxBoxSizer(wxVERTICAL);
  wxBoxSizer *head = new wxBoxSizer(wxHORIZONTAL);
  // The arrow points where the area goes: ◀ folds it to the left, ▶ opens it.
  p_fold = new wxButton(this, wxID_ANY, wxT("◀"), wxDefaultPosition,
                        wxDefaultSize, wxBU_EXACTFIT);
  p_fold->SetToolTip("Fold this area away");
  p_fold->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { fold(!folded()); });
  p_title = new wxStaticText(this, wxID_ANY, "");
  wxFont bold = p_title->GetFont();
  bold.SetWeight(wxFONTWEIGHT_BOLD);
  p_title->SetFont(bold);
  head->Add(p_fold, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
  head->Add(p_title, 1, wxALIGN_CENTER_VERTICAL);
  // The input and output views share one two-way switch in place of the
  // title; any other view (details, basis, launch) shows its title again.
  p_inBtn = new wxToggleButton(this, wxID_ANY, "Input file");
  p_outBtn = new wxToggleButton(this, wxID_ANY, "Output file");
  p_inBtn->SetToolTip("The input file as Save writes it");
  p_outBtn->SetToolTip("The job's output file");
  p_inBtn->Bind(wxEVT_TOGGLEBUTTON, [this](wxCommandEvent&) { showInput(); });
  p_outBtn->Bind(wxEVT_TOGGLEBUTTON, [this](wxCommandEvent&) { showOutput(); });
  head->Add(p_inBtn, 0, wxALIGN_CENTER_VERTICAL);
  head->Add(p_outBtn, 0, wxALIGN_CENTER_VERTICAL);
  p_headSpacer = head->AddStretchSpacer(1);
  p_openEditor = new wxButton(this, wxID_ANY, "Open in editor");
  p_openEditor->SetToolTip("Show the whole file in the external text editor");
  p_openEditor->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { openInEditor(); });
  p_openEditor->Hide();
  head->Add(p_openEditor, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 6);
  top->Add(head, 0, wxEXPAND | wxALL, 6);
  p_note = new wxStaticText(this, wxID_ANY, "");
  top->Add(p_note, 0, wxEXPAND | wxLEFT | wxRIGHT, 6);
  top->Add(new wxStaticLine(this), 0, wxEXPAND | wxALL, 4);
  // Scrolls both ways: an embedded dialog wider or taller than the area
  // (with its Close/Reset row) stays reachable.
  wxScrolledWindow *sw = new wxScrolledWindow(this, wxID_ANY);
  sw->SetScrollRate(10, 10);
  p_body = sw;
  p_body->SetSizer(new wxBoxSizer(wxVERTICAL));
  top->Add(p_body, 1, wxEXPAND | wxALL, 2);
  p_input = new wxTextCtrl(p_body, wxID_ANY, "", wxDefaultPosition,
                           wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY |
                           wxTE_DONTWRAP | wxHSCROLL);
  p_input->SetFont(wxFont(wxFontInfo(p_title->GetFont().GetPointSize())
                          .Family(wxFONTFAMILY_TELETYPE)));
  p_body->GetSizer()->Add(p_input, 1, wxEXPAND);
  SetSizer(top);
  // The preview follows unsaved changes, 500 ms after the last one.
  p_timer = new wxTimer(this);
  Bind(wxEVT_TIMER, [this](wxTimerEvent&) { dryRun(); }, p_timer->GetId());
  p_switchTimer = new wxTimer(this);
  Bind(wxEVT_TIMER, [this](wxTimerEvent&) {
    if (p_kind == LAUNCH && submitted()) showOutput();
  }, p_switchTimer->GetId());
  p_outTimer = new wxTimer(this);
  Bind(wxEVT_TIMER, [this](wxTimerEvent&) {
    if (p_kind != OUTPUT || !loadOutput(false)) p_outTimer->Stop();
  }, p_outTimer->GetId());
  showDefault();
}


SetupArea::~SetupArea()
{
  p_timer->Stop();
  delete p_timer;
  drop();
  delete p_switchTimer;
  delete p_outTimer;
}


void SetupArea::drop()
{
  p_outTimer->Stop();
  p_openEditor->Hide();
  p_launch.reset();
  if (p_basisPanel) {
    p_basisPanel->Destroy();
    p_basisPanel = 0;
  }
  if (!p_emb) return;
  // Detach first: the model must not reach the panel being destroyed.
  if (p_emb->model) p_emb->model->attach(0, 0);
  if (p_emb->host) p_emb->host->Destroy();
  p_emb.reset();
}


bool SetupArea::submitted() const
{
  return p_ed->p_iCalc &&
         p_ed->p_iCalc->getState() >= ResourceDescriptor::STATE_SUBMITTED;
}


void SetupArea::showDefault()
{
  if (submitted()) showOutput();
  else showInput();
}


// The calculation was (or was no longer) submitted: a launch opens the
// output once, and a reset gives the choice of views back.
void SetupArea::submittedChanged(bool on)
{
  if (!on) { p_autoOutput = false; return; }
  if (p_autoOutput) return;
  p_autoOutput = true;
  if (p_kind == INPUT) showOutput();
  // From the Launch panel only once its launch loop has gone quiet: the
  // loop yields, and the panel must not be destroyed under it.
  else if (p_kind == LAUNCH) p_switchTimer->StartOnce(1500);
}


void SetupArea::openInEditor()
{
  if (p_kind != INPUT && p_kind != OUTPUT) return;
  string text;
  if (p_kind == OUTPUT) {
    ICalculation *calc = p_ed->p_iCalc;
    istream *is = calc ? calc->getDataFile(JCode::PRIMARY_OUTPUT) : 0;
    if (!is) return;
    StringConverter::streamToText(*is, text);
    delete is;
  } else {
    text = previewText();
  }
  WxEditSessionMgr sessionMgr;
  sessionMgr.edit(text, p_kind == OUTPUT ? "output" : "input", p_ed, true,
                  p_kind == OUTPUT ? "Calculation Output" : "Input File");
}


void SetupArea::mark()
{
  const bool files = p_kind == INPUT || p_kind == OUTPUT;
  p_title->Show(!files);
  p_inBtn->Show(files);
  p_outBtn->Show(files);
  p_inBtn->SetValue(p_kind == INPUT);
  p_outBtn->SetValue(p_kind == OUTPUT);
  p_openEditor->Show(files);
  if (p_headSpacer) p_headSpacer->Show(files);
  Layout();

  // The button whose details fill the area stays marked.
  struct B { long id; Kind k; } bs[] = {
    {CalcEd::ID_BUTTON_CALCED_THEORY, THEORY},
    {CalcEd::ID_BUTTON_CALCED_RUNTYPE, RUNTYPE}};
  for (const B& b : bs) {
    wxWindow *w = p_ed->FindWindow(b.id);
    if (!w) continue;
    wxFont f = w->GetFont();
    f.SetWeight(p_kind == b.k ? wxFONTWEIGHT_BOLD : wxFONTWEIGHT_NORMAL);
    w->SetFont(f);
  }
}


bool SetupArea::showDetails(Kind kind, const string& cmd)
{
  fold(false);   // a view asked for is shown
  vector<string> words = splitCommand(cmd);
  if (words.size() < 19) return false;
  string dlg = words[1];
  if (endsWith(dlg, ".py")) dlg = dlg.substr(0, dlg.size() - 3) + ".dlg";
  if (!endsWith(dlg, ".dlg") || !wxFileExists(wxString::FromUTF8(dlg)))
    return false;

  std::unique_ptr<Embedded> e(new Embedded);
  details::Params par;
  long port = 0;
  if (!e->desc.load(dlg) ||
      !details::makeParams(vector<string>(words.begin() + 2, words.end()),
                           par, port))
    return false;
  e->model.reset(new details::Model(e->desc, par));
  string err;
  if (!e->model->build(err)) {
    wxLogDebug("SetupArea: %s", err.c_str());
    return false;
  }
  drop();
  p_input->Hide();
  e->transport.reset(new EditorTransport(p_ed, kind == THEORY));
  e->host = new wxPanel(p_body, wxID_ANY);
  details::DetailsPanel *panel = new details::DetailsPanel(e->host,
      *e->model, [this]() { CallAfter([this]() { showDefault(); }); });
  e->model->attach(e->transport.get(), panel);
  e->panel = panel;
  p_body->GetSizer()->Add(e->host, 1, wxEXPAND);
  p_emb = std::move(e);
  p_kind = kind;
  // No process: the editor's pid check (kill on close) must not fire.
  p_emb->transport->send("#STARTED 0\n");
  p_emb->model->initialize();
  p_title->SetLabel(wxString::FromUTF8(p_emb->model->title));
  p_note->SetLabel("");
  p_note->Hide();
  mark();
  Layout();
  p_body->Layout();
  // wxGTK: windows made inside an already shown page get no size
  // allocation until the page is resized; resize it once they exist.
  CallAfter([this]() {
    if (!p_emb) return;
    p_emb->host->Layout();
    fitBody();
    SendSizeEvent();
    p_body->SendSizeEvent();
    ToolTabs::allocate(this);
    Refresh();
  });
  return true;
}


void SetupArea::showBasis()
{
  fold(false);   // a view asked for is shown
  if (p_kind == BASIS && p_basisPanel) return;
  drop();
  p_input->Hide();
  p_basisPanel = new BasisPanel(p_body, p_ed);
  p_body->GetSizer()->Add(p_basisPanel, 1, wxEXPAND);
  p_kind = BASIS;
  p_title->SetLabel("Basis set");
  p_note->SetLabel("");
  p_note->Hide();
  mark();
  Layout();
  p_body->Layout();
  CallAfter([this]() {
    if (!p_basisPanel) return;
    p_basisPanel->Layout();
    fitBody();
    SendSizeEvent();
    p_body->SendSizeEvent();
    ToolTabs::allocate(this);
    Refresh();
  });
}


bool SetupArea::showLaunch()
{
  if (submitted()) return false;   // the Launch panel is offered once
  fold(false);   // a view asked for is shown
  if (!LaunchEmbed::maker()) return false;
  if (p_kind == LAUNCH && p_launch) return true;
  drop();
  p_input->Hide();
  p_launch.reset(LaunchEmbed::maker()(p_body, p_ed->getContext(),
      [this]() { CallAfter([this]() { showDefault(); }); },
      [this]() { if (p_launch) { Layout(); p_body->Layout(); fitBody(); } },
      [this](const string& text, int sev) {
        WxFeedback *fb = p_ed->getFeedback();
        if (!fb) return;
        //  No flush: it is wxSafeYield, which greys the whole window for
        //  each launch log line (#1266); the launch loop's yield draws it.
        if (p_switchTimer->IsRunning()) p_switchTimer->StartOnce(1500);
        if (text.empty()) fb->clearMessage();
        else fb->setMessage(text, (WxFeedback::Severity)sev, false, false);
      }));
  wxWindow *w = p_launch->window();
  // Its fields stretch with the area, so folding the editor away gives
  // paths room.
  p_body->GetSizer()->Add(w, 1, wxEXPAND);
  p_kind = LAUNCH;
  p_title->SetLabel("Launch");
  p_note->SetLabel("");
  p_note->Hide();
  mark();
  Layout();
  p_body->Layout();
  CallAfter([this]() {
    if (!p_launch) return;
    p_launch->window()->Layout();
    fitBody();
    SendSizeEvent();
    p_body->SendSizeEvent();
    ToolTabs::allocate(this);
    Refresh();
  });
  return true;
}


string SetupArea::launchGet(const string& what)
{
  return p_launch ? p_launch->get(what) : "";
}


bool SetupArea::launchGo()
{
  return p_launch && p_launch->go();
}


void SetupArea::basisFilter(const string& text)
{
  if (p_basisPanel) p_basisPanel->setFilter(text);
}


bool SetupArea::basisPick(const string& name)
{
  return p_basisPanel && p_basisPanel->pick(name);
}


bool SetupArea::basisUseAll()
{
  return p_basisPanel && p_basisPanel->useForAll();
}


int SetupArea::basisAssign(const string& name, int kind,
                           const vector<string>& elements)
{
  if (!p_basisPanel) return -1;
  if (name.empty() && kind == BasisPanel::ECP)
    return p_basisPanel->acceptEcp() ? 1 : 0;
  return p_basisPanel->assignNamed(name, (BasisPanel::Kind)kind, elements)
             ? 1 : -1;
}


bool SetupArea::basisAssignMany(int kind, const vector<std::pair<string,
                                vector<string> > >& steps)
{
  return p_basisPanel &&
         p_basisPanel->assignMany((BasisPanel::Kind)kind, steps);
}


bool SetupArea::basisAtoms(const string& names)
{
  if (!p_basisPanel) return false;
  p_basisPanel->setAtoms(names);
  return true;
}


bool SetupArea::basisAppend(bool tight, const string& element, int shell,
                            int count)
{
  return p_basisPanel &&
         p_basisPanel->appendFunctions(tight, element, shell, count);
}


bool SetupArea::basisContraction(const string& el, int row,
                                 const string& action, string& out)
{
  if (!p_basisPanel) { out = "no basis panel"; return false; }
  return p_basisPanel->setContraction(el, row, action, out);
}


string SetupArea::basisCoefficients()
{
  return p_basisPanel ? p_basisPanel->coefficients() : string();
}


void SetupArea::basisChanged()
{
  if (p_basisPanel) p_basisPanel->refresh();
}


void SetupArea::external(Kind kind)
{
  p_external = kind;
  showInput();
}


void SetupArea::externalClosed(Kind kind)
{
  if (p_external != kind) return;
  p_external = INPUT;
  if (p_kind == INPUT) statusChanged();
}


wxString SetupArea::note() const
{
  return p_note->IsShown() ? p_note->GetLabel() : wxString();
}


//  While details are open in their own window the note says so, whatever the
//  preview's state: the dry runs their edits start must not swap it back and
//  forth with the preview's own note.
static wxString withExternal(SetupArea::Kind ext, const wxString& note)
{
  if (ext == SetupArea::INPUT) return note;
  wxString open = wxString(ext == SetupArea::THEORY ? "Theory" : "Runtype") +
                  " Details of this code open in their own window.";
  return note.empty() ? open : open + " " + note;
}


void SetupArea::dryRun()
{
  if (p_kind != INPUT) return;
  string deck, err;
  wxBusyCursor busy;
  bool ok = p_ed->previewDeck(deck, err);
  p_dry = ok;
  wxString note = ok ? wxString("Unsaved changes: Save writes this file.")
                      : "Unsaved changes; the settings could not be "
                        "generated: " + wxString::FromUTF8(err.c_str());
  note = withExternal(p_external, note);
  if (p_note->GetLabel() != note) p_note->SetLabel(note);
  p_note->Wrap(std::max(200, p_body->GetSize().x));
  p_note->Show();
  if (ok) {
    wxString shown = wxString::FromUTF8(deck.c_str());
    if (p_input->GetValue() != shown) p_input->ChangeValue(shown);
  }
  Layout();
}


std::string SetupArea::previewShown() const
{
  return string(p_input->GetValue().utf8_str());
}


void SetupArea::fitBody()
{
  wxScrolledWindow *sw = static_cast<wxScrolledWindow*>(p_body);
  // The preview scrolls itself; only a dialog sets a virtual size.
  wxSize min = (p_emb || p_basisPanel) ? p_body->GetSizer()->CalcMin()
                                       : wxSize(1, 1);
  sw->SetVirtualSize(min);
  p_body->Layout();
}


bool SetupArea::setValue(const string& name, const string& value)
{
  if (!p_emb) return false;
  int i = p_emb->model->find(name);
  if (i < 0) return false;
  p_emb->model->userEdit(i, value);
  p_emb->panel->itemUpdated(i);
  return true;
}


void SetupArea::closeDetails(Kind kind)
{
  if (p_kind == kind && p_emb) showDefault();
}


std::string SetupArea::previewText() const
{
  string text;
  ICalculation *calc = p_ed->p_iCalc;
  if (!calc) return text;
  std::istream *is = calc->getDataFile(JCode::PRIMARY_INPUT);
  if (is) {
    StringConverter::streamToText(*is, text);
    delete is;
  }
  return text;
}


void SetupArea::fold(bool on)
{
  if (on == folded()) return;
  if (on) {
    for (wxWindow *w : GetChildren())
      if (w != p_fold && w->IsShown()) { w->Hide(); p_foldHidden.push_back(w); }
  } else {
    for (wxWindow *w : p_foldHidden) w->Show();
    p_foldHidden.clear();
  }
  p_fold->SetLabel(on ? wxT("▶") : wxT("◀"));
  p_fold->SetToolTip(on ? "Open this area" : "Fold this area away");
  Layout();
  Refresh();
}


void SetupArea::showInput()
{
  fold(false);   // a view asked for is shown
  drop();
  p_kind = INPUT;
  p_input->Show();
  fitBody();
  p_title->SetLabel("Input file as Save writes it");
  statusChanged();
  mark();
  Layout();
  p_body->Layout();
}


void SetupArea::showOutput()
{
  fold(false);   // a view asked for is shown
  drop();
  p_kind = OUTPUT;
  p_input->Show();
  fitBody();
  p_title->SetLabel("Output file");
  p_input->ChangeValue("");
  if (loadOutput(true)) p_outTimer->Start(5000);
  mark();
  Layout();
  p_body->Layout();
}


void SetupArea::outputStateChanged()
{
  if (p_kind != OUTPUT) return;
  if (loadOutput(false)) {
    if (!p_outTimer->IsRunning()) p_outTimer->Start(5000);
  } else {
    p_outTimer->Stop();
  }
}


// Reads the output again; true while the job may still write to it.  The
// view stays at the end unless the reader has moved away from it.
bool SetupArea::loadOutput(bool first)
{
  ICalculation *job = p_ed->p_iCalc;
  if (!job) return false;
  const ResourceDescriptor::RUNSTATE st = job->getState();
  const bool running = st == ResourceDescriptor::STATE_SUBMITTED ||
                       st == ResourceDescriptor::STATE_RUNNING ||
                       st == ResourceDescriptor::STATE_WAITING;
  istream *is = job->getDataFile(JCode::PRIMARY_OUTPUT);
  wxString note;
  string text;
  if (is) {
    StringConverter::streamToText(*is, text);
    delete is;
  } else {
    note = running ? "No output yet: it appears here while the job runs."
                   : "This calculation has no output file.";
  }
  if (p_note->GetLabel() != note) p_note->SetLabel(note);
  p_note->Show(!note.empty());
  wxString shown = wxString::FromUTF8(text.c_str());
  if (p_input->GetValue() != shown) {
    const long ip = p_input->GetInsertionPoint();
    const bool atEnd = first || ip >= p_input->GetLastPosition();
    p_input->ChangeValue(shown);
    const long to = atEnd ? p_input->GetLastPosition() : ip;
    p_input->SetInsertionPoint(to);
    p_input->ShowPosition(to);
  }
  Layout();
  return running;
}


void SetupArea::statusChanged()
{
  if (p_kind != INPUT) return;
  string text = previewText();
  bool modified = p_ed->p_feedback &&
      p_ed->p_feedback->getEditStatus() == WxFeedback::MODIFIED;
  if (modified) {
    // Until the dry run, the note keeps what it said (or only that the
    // details are open), instead of blinking through an empty one.
    if (p_external != INPUT && !p_note->IsShown()) {
      p_note->SetLabel(withExternal(p_external, ""));
      p_note->Show();
      Layout();
    }
    p_timer->StartOnce(500);
    return;
  }
  p_timer->Stop();
  p_dry = false;
  wxString note;
  if (text.empty()) note = "No input file yet: Save writes it.";
  else if (p_ed->p_inputGenFailed)
    note = "The current settings could not be generated; this is the last "
           "file that was.";
  note = withExternal(p_external, note);
  if (p_note->GetLabel() != note) p_note->SetLabel(note);
  p_note->Show(!note.empty());
  wxString shown = wxString::FromUTF8(text.c_str());
  if (p_input->GetValue() != shown) p_input->ChangeValue(shown);
  Layout();
}


wxString SetupArea::describe() const
{
  wxString out = wxString::Format("area=%s title=\"%s\" folded=%d",
      kindName(p_kind), p_title->GetLabel(), (int)folded());
  wxSize sz = p_body->GetSize();
  out += wxString::Format(" size=%dx%d", sz.x, sz.y);
  wxSize win = wxGetTopLevelParent(const_cast<SetupArea*>(this))->GetSize();
  wxSize virt = p_body->GetVirtualSize();
  out += wxString::Format(" window=%dx%d virtual=%dx%d", win.x, win.y,
                          virt.x, virt.y);
  if (p_kind == INPUT) {
    out += wxString::Format(" lines=%d dry=%d", p_input->GetNumberOfLines(),
                            (int)p_dry);
  } else if (p_kind == OUTPUT) {
    out += wxString::Format(" lines=%d following=%d editor=%d",
                            p_input->GetNumberOfLines(),
                            (int)p_outTimer->IsRunning(),
                            (int)p_openEditor->IsShown());
  } else if (p_kind == LAUNCH && p_launch) {
    out += " " + wxString::FromUTF8(p_launch->get("info").c_str());
  } else if (p_kind == BASIS && p_basisPanel) {
    out += " " + wxString::FromUTF8(p_basisPanel->describe().c_str());
  } else if (p_emb) {
    int shown = 0;
    std::vector<wxWindow*> todo(p_emb->host->GetChildren().begin(),
                                p_emb->host->GetChildren().end());
    while (!todo.empty()) {
      wxWindow *w = todo.back();
      todo.pop_back();
      if (w->IsShown() && w->GetChildren().empty()) shown++;
      for (wxWindow *c : w->GetChildren()) todo.push_back(c);
    }
    out += wxString::Format(" controls=%d", shown);
    // Candidates in a ligand-molecule list (its "other" entry not counted).
    for (const details::Item& it : p_emb->model->items())
      if (it.kind == "ligandpick")
        out += wxString::Format(" ligandpick=%d", (int)it.urls.size() - 1);
  }
  // What the editor's summary under the Details buttons shows.
  for (wxSizer *sz : {p_ed->p_theoryDetailsSizer, p_ed->p_runtypeDetailsSizer}) {
    wxString sum;
    std::vector<wxSizer*> todo(1, sz);
    while (!todo.empty()) {
      wxSizer *cur = todo.front();
      todo.erase(todo.begin());
      for (wxSizerItem *it : cur->GetChildren()) {
        if (it->GetSizer()) todo.push_back(it->GetSizer());
        else if (wxWindow *t = it->GetWindow())
          sum += (sum.empty() ? "" : "|") + t->GetLabel();
      }
    }
    sum.Replace(" ", "_");
    out += (sz == p_ed->p_theoryDetailsSizer ? " theorysum=" : " runtypesum=")
           + sum;
  }
  for (long id : {(long)CalcEd::ID_BUTTON_CALCED_THEORY,
                  (long)CalcEd::ID_BUTTON_CALCED_RUNTYPE}) {
    wxWindow *w = p_ed->FindWindow(id);
    if (w && w->GetFont().GetWeight() == wxFONTWEIGHT_BOLD)
      out += " marked=" + w->GetLabel();
  }
  return out;
}
