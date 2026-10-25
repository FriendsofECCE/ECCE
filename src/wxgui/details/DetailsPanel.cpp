#include "DetailsPanel.h"

#include <wx/socket.h>
#include <wx/spinctrl.h>
#include <wx/statline.h>
#include <wx/utils.h>
#include <cstdlib>
#include <fstream>

namespace details {

using std::string;

UdpTransport::UdpTransport(long port)
{
  peer_ = new wxIPV4address;
  peer_->Hostname("127.0.0.1");
  peer_->Service((unsigned short)port);
  wxIPV4address local;
  local.Service(0);
  sock_ = new wxDatagramSocket(local);
}

UdpTransport::~UdpTransport()
{
  delete sock_;
  delete peer_;
}

void UdpTransport::send(const string& s)
{
  sock_->SendTo(*peer_, s.data(), s.size());
}

wxFont themeFont()
{
  // The Python dialogs' rule: 10 pt (or the system size) moved by the
  // preference step, read from the same EcceGlobal file.
  int base = 10, fs = 1;
  wxString home;
  if (!wxGetEnv("ECCE_REALUSERHOME", &home)) home = wxGetHomeDir();
  std::ifstream in((home + "/.ECCE/EcceGlobal").ToStdString());
  string line;
  bool sys = false;
  while (std::getline(in, line)) {
    size_t c = line.find(':');
    if (c == string::npos) continue;
    string k = trim(line.substr(0, c)), v = trim(line.substr(c + 1));
    for (auto& ch : k) ch = tolower(ch);
    for (auto& ch : v) ch = tolower(ch);
    if (k == "usesystemfont") sys = v == "true" || v == "1";
    if (k == "fontsize") fs = atoi(v.c_str());
  }
  wxFont f = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
  if (sys) base = f.GetPointSize();
  int step = fs == 0 ? -1 : fs == 2 ? 2 : fs == 3 ? 4 : 0;
  f.SetPointSize(base + step);
  return f;
}

DetailsPanel::DetailsPanel(wxFrame *frame, Model& model)
    : wxPanel(frame, wxID_ANY, wxDefaultPosition, wxDefaultSize,
              wxTAB_TRAVERSAL),
      model_(model), frame_(frame)
{
  SetFont(themeFont());
  helpURL_ = model.helpURL;
  build();
  buildFooter();
  frame->SetTitle(model.title);
}

void DetailsPanel::build()
{
  const std::vector<Item>& items = model_.items();
  w_.assign(items.size(), W());
  wxBoxSizer *top = new wxBoxSizer(wxVERTICAL);
  wxFlexGridSizer *grid = 0;
  const int fl = wxALL | wxALIGN_CENTER_VERTICAL;

  for (const Entry& e : model_.entries()) {
    if (e.type == 'B') {
      const Box& b = model_.boxes()[e.index];
      // The Python EcceBoxSizer: a rule, the label, a rule.
      wxBoxSizer *bar = new wxBoxSizer(wxHORIZONTAL);
      if (b.label.empty()) bar->Add(new wxStaticLine(this), 1);
      else {
        bar->Add(new wxStaticLine(this, wxID_ANY, wxDefaultPosition,
                                  wxSize(10, -1)), 0, wxALIGN_CENTER, 0);
        bar->Add(new wxStaticText(this, wxID_ANY, b.label), 0,
                 wxALIGN_CENTER | wxALL, 2);
        bar->Add(new wxStaticLine(this), 1, wxALIGN_CENTER, 0);
      }
      wxBoxSizer *box = new wxBoxSizer(wxVERTICAL);
      box->Add(bar, 0, wxEXPAND, 0);
      grid = new wxFlexGridSizer(0, b.cols, 0, 3);
      box->Add(grid, 1, wxALL | wxEXPAND, 0);
      top->Add(box, 0, wxALL | wxEXPAND, 1);
      continue;
    }
    int i = e.index;
    const Item& it = items[i];
    W& w = w_[i];
    wxString nm(it.name);
    const string& k = it.kind;
    if (k == "note") {
      wxStaticText *t = new wxStaticText(this, wxID_ANY, it.label);
      w.w = t;
      grid->Add(t, 0, fl, 3);
    } else if (k == "check") {
      wxCheckBox *b = new wxCheckBox(this, wxID_ANY, it.label,
          wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, nm);
      w.w = b;
      grid->Add(b, 0, fl, 3);
      b->Bind(wxEVT_CHECKBOX, [this, i](wxCommandEvent& ev) {
        model_.userEdit(i, ((wxCheckBox*)ev.GetEventObject())->GetValue()
                               ? "1" : "0");
      });
    } else if (k == "radiopair") {
      // first = the default (false) choice, second = the stored setting.
      wxRadioButton *a = new wxRadioButton(this, wxID_ANY, it.first,
          wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
      wxRadioButton *b = new wxRadioButton(this, wxID_ANY, it.second,
          wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, nm);
      a->SetValue(true);
      w.w = a;
      w.w2 = b;
      grid->Add(a, 0, fl, 3);
      grid->Add(b, 0, fl, 3);
      auto cb = [this, i, b](wxCommandEvent&) {
        model_.userEdit(i, b->GetValue() ? "1" : "0");
      };
      a->Bind(wxEVT_RADIOBUTTON, cb);
      b->Bind(wxEVT_RADIOBUTTON, cb);
      b->SetToolTip(nm);
    } else if (k == "combo" || k == "calcpick") {
      wxArrayString ch;
      for (const string& s : it.choices) ch.Add(wxString::FromUTF8(s));
      wxBoxSizer *row = new wxBoxSizer(wxHORIZONTAL);
      if (!it.label.empty()) {
        w.lab = new wxStaticText(this, wxID_ANY, wxString::FromUTF8(it.label) + "   ");
        row->Add(w.lab, 0, fl, 0);
      }
      wxChoice *c = new wxChoice(this, wxID_ANY, wxDefaultPosition,
          wxDefaultSize, ch, 0, wxDefaultValidator, nm);
      w.w = c;
      row->Add(c, 0, fl, 0);
      grid->Add(row, 0, fl, 3);
      c->Bind(wxEVT_CHOICE, [this, i, c, k](wxCommandEvent&) {
        int sel = c->GetSelection();
        const Item& m = model_.items()[i];
        if (k == "calcpick")
          model_.userEdit(i, sel >= 0 ? m.urls[sel] : "");
        else
          model_.userEdit(i, c->GetStringSelection().ToStdString());
      });
    } else {   // text, float, int
      wxBoxSizer *row = new wxBoxSizer(wxHORIZONTAL);
      if (!it.label.empty()) {
        w.lab = new wxStaticText(this, wxID_ANY, wxString::FromUTF8(it.label) + "   ");
        row->Add(w.lab, 0, fl, 0);
      }
      if (k == "int") {
        bool hl, hh;
        double lo = -1000000, hi = 1000000, a, b;
        if (rangeBounds(it.hard, hl, a, hh, b)) {
          if (hl) lo = a;
          if (hh) hi = b;
        }
        wxSpinCtrl *s = new wxSpinCtrl(this, wxID_ANY, "", wxDefaultPosition,
            wxSize(80, -1), wxSP_ARROW_KEYS, (int)lo, (int)hi,
            atoi(it.value.c_str()));
        s->SetName(nm);
        w.w = s;
        auto cb = [this, i, s]() {
          model_.userEdit(i, std::to_string(s->GetValue()));
        };
        s->Bind(wxEVT_SPINCTRL, [cb](wxCommandEvent&) { cb(); });
        s->Bind(wxEVT_KILL_FOCUS, [cb](wxFocusEvent& ev) { cb(); ev.Skip(); });
      } else {
        wxTextCtrl *t = new wxTextCtrl(this, wxID_ANY,
            wxString::FromUTF8(it.value), wxDefaultPosition, wxSize(80, -1),
            wxTE_PROCESS_ENTER, wxDefaultValidator, nm);
        w.w = t;
        auto cb = [this, i, t]() {
          model_.userEdit(i, t->GetValue().ToStdString());
        };
        t->Bind(wxEVT_KILL_FOCUS, [cb](wxFocusEvent& ev) { cb(); ev.Skip(); });
        t->Bind(wxEVT_TEXT_ENTER, [cb](wxCommandEvent&) { cb(); });
      }
      row->Add(w.w, 0, fl, 0);
      string rng = !it.soft.empty() ? it.soft : it.hard;
      if (k != "text" && (!it.unit.empty() || !rng.empty())) {
        // The stored unit stays "Angstrom"; only the label shows the sign.
        wxString shown = wxString::FromUTF8(it.unit);
        shown.Replace("Angstroms", wxString::FromUTF8("\xC3\x85"));
        shown.Replace("Angstrom", wxString::FromUTF8("\xC3\x85"));
        w.unitLab = new wxStaticText(this, wxID_ANY,
            "  " + shown + wxString::FromUTF8("\n   " + rng));
        row->Add(w.unitLab, 0, fl, 0);
      }
      grid->Add(row, 0, fl, 3);
    }
    w.w->SetToolTip(nm);
    itemUpdated(i);
  }
  if (model_.minWidth) top->SetMinSize(wxSize(model_.minWidth, -1));
  SetSizer(top);
}

void DetailsPanel::buildFooter()
{
  wxFont font = themeFont();
  // The footer belongs to the frame, as in the Python dialogs, so it stays put
  // when the settings area changes size.
  footer_ = new wxPanel(frame_, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                        wxTAB_TRAVERSAL);
  footer_->SetFont(font);
  wxBoxSizer *fs = new wxBoxSizer(wxVERTICAL);
  footer_->SetSizer(fs);
  wxBoxSizer *fr = new wxBoxSizer(wxVERTICAL);
  frame_->SetSizer(fr);
  fr->Add(this, 1, wxEXPAND);
  fr->Add(footer_, 0, wxEXPAND);

  msg_ = new wxTextCtrl(footer_, wxID_ANY, "", wxDefaultPosition,
                        wxSize(-1, 65),
                        wxTE_MULTILINE | wxTE_WORDWRAP | wxTE_READONLY);
  msg_->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
  fs->Add(msg_, 0, wxALL | wxEXPAND, 1);
  colorTimer_ = new wxTimer(msg_, 1);
  messageTimer_ = new wxTimer(msg_, 2);
  msg_->Bind(wxEVT_TIMER, [this](wxTimerEvent&) {
    msg_->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
    msg_->Refresh();
  }, 1);
  msg_->Bind(wxEVT_TIMER, [this](wxTimerEvent&) { msg_->SetValue(""); }, 2);

  wxBoxSizer *bs = new wxBoxSizer(wxHORIZONTAL);
  bs->AddStretchSpacer(1);
  wxButton *closeB = new wxButton(footer_, wxID_ANY, "Close");
  closeB->SetToolTip("close window");
  closeB->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { model_.close(); });
  resetBtn_ = new wxButton(footer_, wxID_ANY, "Reset");
  resetBtn_->SetToolTip("Restore default settings");
  resetBtn_->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { onReset(); });
  wxButton *helpB = 0;
  if (!helpURL_.empty()) {
    helpB = new wxButton(footer_, wxID_ANY, "Help");
    helpB->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
      wxLaunchDefaultBrowser(helpURL_);
    });
  }
  // One sizer item per button on every platform; the stretch spacers are
  // not added for Windows, so the row cannot be laid out twice there.
  for (wxButton *b : {closeB, resetBtn_, helpB}) {
    if (!b) continue;
#ifdef __WXMSW__
    bs->Add(b, 0, wxALL, 3);
#else
    bs->Add(b, 0, wxTOP, 3);
    bs->AddStretchSpacer(1);
#endif
  }
  fs->Add(bs, 0, wxEXPAND, 0);
  if (model_.hideMessage) msg_->Hide();
}

void DetailsPanel::itemUpdated(int i)
{
  const Item& it = model_.items()[i];
  W& w = w_[i];
  if (!w.w) return;
  const string& k = it.kind;
  if (k == "check") ((wxCheckBox*)w.w)->SetValue(it.value == "1");
  else if (k == "radiopair") {
    ((wxRadioButton*)w.w2)->SetValue(it.value == "1");
    ((wxRadioButton*)w.w)->SetValue(it.value != "1");
  } else if (k == "combo")
    ((wxChoice*)w.w)->SetStringSelection(wxString::FromUTF8(it.value));
  else if (k == "calcpick") {
    int sel = 0;
    for (size_t j = 0; j < it.urls.size(); j++)
      if (it.urls[j] == it.value) sel = (int)j;
    ((wxChoice*)w.w)->SetSelection(sel);
  } else if (k == "int") {
    wxSpinCtrl *s = (wxSpinCtrl*)w.w;
    if (s->GetValue() != atoi(it.value.c_str()))
      s->SetValue(atoi(it.value.c_str()));
  } else if (k != "note") {
    wxTextCtrl *t = (wxTextCtrl*)w.w;
    if (t->GetValue().ToStdString() != it.value)
      t->SetValue(wxString::FromUTF8(it.value));
  }
  for (wxWindow *x : {w.w, w.w2, (wxWindow*)w.lab, (wxWindow*)w.unitLab}) {
    if (!x) continue;
    if (x->IsEnabled() != it.enabled) x->Enable(it.enabled);
    if (x->IsShown() != it.shown) x->Show(it.shown);
  }
}

void DetailsPanel::status(const string& text, Severity s)
{
  if (!msg_) return;
  msg_->SetValue(wxString::FromUTF8(text));
  if (s != NORMAL) {
    msg_->SetBackgroundColour(wxColour(s == WARNING ? "#fff1cc" : "#ffdddd"));
    colorTimer_->Start(3000, true);
  }
  messageTimer_->Start(5000, true);
}

void DetailsPanel::resetEnabled(bool on)
{
  if (resetBtn_) resetBtn_->Enable(on);
}

void DetailsPanel::layoutChanged()
{
  Layout();
}

void DetailsPanel::closeRequested()
{
  frame_->Destroy();
}

void DetailsPanel::onReset()
{
  wxMessageDialog d(this, "Hitting OK will set all parameters to their "
      "default values.\nDo you want to continue?", "Reset Defaults?",
      wxOK | wxCANCEL | wxICON_QUESTION | wxSTAY_ON_TOP);
  if (d.ShowModal() == wxID_OK) model_.reset();
}

}  // namespace details
