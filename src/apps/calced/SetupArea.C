/**
 * @file
 *
 * See SetupArea.H and docs/claude/services/tools-as-tabs.md.
 */
#include <fstream>
#include <sstream>

#include "wx/wx.h"
#include "wx/statline.h"

#include "dsm/ICalculation.H"
#include "dsm/JCode.H"
#include "util/StringConverter.H"
#include "wxgui/ToolTabs.H"
#include "wxgui/WxFeedback.H"

#include "DetailsPanel.h"

#include "CalcEd.H"
#include "CalcEdGUI.H"
#include "SetupArea.H"
#include "BasisPanel.H"
#include "wxgui/LaunchEmbed.H"

using std::string;
using std::vector;

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
         k == BASIS ? "basis" : k == LAUNCH ? "launch" : "input";
}


SetupArea::SetupArea(wxWindow *parent, CalcEd *editor)
  : wxPanel(parent, wxID_ANY), p_ed(editor)
{
  wxBoxSizer *top = new wxBoxSizer(wxVERTICAL);
  wxBoxSizer *head = new wxBoxSizer(wxHORIZONTAL);
  p_fold = new wxButton(this, wxID_ANY, wxT("◀"), wxDefaultPosition,
                        wxDefaultSize, wxBU_EXACTFIT);
  p_fold->SetToolTip("Hide or show the editor column");
  p_fold->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
    bool folded = ToolTabs::fold(p_ed, ToolTabs::fold(p_ed, -1) ? 0 : 1);
    p_fold->SetLabel(folded ? wxT("▶") : wxT("◀"));
  });
  p_title = new wxStaticText(this, wxID_ANY, "");
  wxFont bold = p_title->GetFont();
  bold.SetWeight(wxFONTWEIGHT_BOLD);
  p_title->SetFont(bold);
  head->Add(p_fold, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
  head->Add(p_title, 1, wxALIGN_CENTER_VERTICAL);
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
  Bind(wxEVT_TIMER, [this](wxTimerEvent&) { dryRun(); });
  showInput();
}


SetupArea::~SetupArea()
{
  p_timer->Stop();
  delete p_timer;
  drop();
}


void SetupArea::drop()
{
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


void SetupArea::mark()
{
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
      *e->model, [this]() { CallAfter([this]() { showInput(); }); });
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
  if (!LaunchEmbed::maker()) return false;
  if (p_kind == LAUNCH && p_launch) return true;
  drop();
  p_input->Hide();
  p_launch.reset(LaunchEmbed::maker()(p_body, p_ed->getContext(),
      [this]() { CallAfter([this]() { showInput(); }); },
      [this]() { if (p_launch) { Layout(); p_body->Layout(); fitBody(); } }));
  wxWindow *w = p_launch->window();
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


void SetupArea::basisChanged()
{
  if (p_basisPanel) p_basisPanel->refresh();
}


void SetupArea::external(Kind kind)
{
  showInput();
  p_note->SetLabel(wxString(kind == THEORY ? "Theory" : "Runtype") +
                   " Details of this code open in their own window.");
  p_note->Show();
  Layout();
}


void SetupArea::dryRun()
{
  if (p_kind != INPUT) return;
  string deck, err;
  wxBusyCursor busy;
  bool ok = p_ed->previewDeck(deck, err);
  p_dry = ok;
  p_note->SetLabel(ok ? wxString("Unsaved changes: Save writes this file.")
                      : "Unsaved changes; the settings could not be "
                        "generated: " + wxString::FromUTF8(err.c_str()));
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
  if (p_kind == kind && p_emb) showInput();
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


void SetupArea::showInput()
{
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


void SetupArea::statusChanged()
{
  if (p_kind != INPUT) return;
  string text = previewText();
  bool modified = p_ed->p_feedback &&
      p_ed->p_feedback->getEditStatus() == WxFeedback::MODIFIED;
  if (modified) {
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
  p_note->SetLabel(note);
  p_note->Show(!note.empty());
  wxString shown = wxString::FromUTF8(text.c_str());
  if (p_input->GetValue() != shown) p_input->ChangeValue(shown);
  Layout();
}


wxString SetupArea::describe() const
{
  wxString out = wxString::Format("area=%s title=\"%s\" folded=%d",
      kindName(p_kind), p_title->GetLabel(), (int)ToolTabs::fold(p_ed, -1));
  wxSize sz = p_body->GetSize();
  out += wxString::Format(" size=%dx%d", sz.x, sz.y);
  wxSize win = wxGetTopLevelParent(const_cast<SetupArea*>(this))->GetSize();
  wxSize virt = p_body->GetVirtualSize();
  out += wxString::Format(" window=%dx%d virtual=%dx%d", win.x, win.y,
                          virt.x, virt.y);
  if (p_kind == INPUT) {
    out += wxString::Format(" lines=%d dry=%d", p_input->GetNumberOfLines(),
                            (int)p_dry);
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
