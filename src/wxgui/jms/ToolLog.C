#include "wx/app.h"
#include "wx/button.h"
#include "wx/dialog.h"
#include "wx/sizer.h"
#include "wx/textctrl.h"
#include "wx/toplevel.h"

#include <map>

#include "wxgui/ToolTabs.H"
#include "wxgui/ToolLog.H"

namespace {
bool s_installed = false;

std::map<wxTopLevelWindow*, wxLog*>& sinks()
{
  static auto *m = new std::map<wxTopLevelWindow*, wxLog*>;
  return *m;
}

// The tool a message concerns: the focused tool window, else the active tab.
wxLog *sinkNow()
{
  std::map<wxTopLevelWindow*, wxLog*>& m = sinks();
  if (m.empty()) return 0;
  if (wxWindow *f = wxWindow::FindFocus()) {
    auto it = m.find(dynamic_cast<wxTopLevelWindow*>(wxGetTopLevelParent(f)));
    if (it != m.end()) return it->second;
  }
  auto it = m.find(ToolTabs::activeTool());
  return it == m.end() ? 0 : it->second;
}

// One box for every message that reaches no tool; later messages are added
// to it while it is open.
wxDialog *s_box = 0;
wxTextCtrl *s_boxText = 0;

void showBox(const wxString& title, const wxString& text)
{
  if (s_box) {
    s_boxText->AppendText("\n" + text);
    s_box->Raise();
    return;
  }
  wxWindow *parent = wxTheApp ? wxTheApp->GetTopWindow() : 0;
  s_box = new wxDialog(parent, wxID_ANY, title, wxDefaultPosition,
                       wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
  s_boxText = new wxTextCtrl(s_box, wxID_ANY, text, wxDefaultPosition,
                             wxSize(480, 140), wxTE_MULTILINE | wxTE_READONLY);
  wxButton *ok = new wxButton(s_box, wxID_OK);
  wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
  sizer->Add(s_boxText, 1, wxEXPAND | wxALL, 10);
  sizer->Add(ok, 0, wxALIGN_RIGHT | wxRIGHT | wxBOTTOM, 10);
  s_box->SetSizerAndFit(sizer);
  ok->Bind(wxEVT_BUTTON, [](wxCommandEvent&) { if (s_box) s_box->Destroy(); });
  s_box->Bind(wxEVT_CLOSE_WINDOW, [](wxCloseEvent&) { if (s_box) s_box->Destroy(); });
  s_box->Bind(wxEVT_DESTROY, [](wxWindowDestroyEvent& ev) {
    if (ev.GetEventObject() == s_box) { s_box = 0; s_boxText = 0; }
    ev.Skip();
  });
  if (parent) s_box->CentreOnParent();
  s_box->Show();
}
}


void ToolLog::install()
{
  if (s_installed) return;
  s_installed = true;
  delete wxLog::SetActiveTarget(new ToolLog);
}


bool ToolLog::installed()
{
  return s_installed;
}


void ToolLog::registerTool(wxTopLevelWindow *tool, wxLog *sink)
{
  if (tool && sink) sinks()[tool] = sink;
}


void ToolLog::unregisterTool(wxTopLevelWindow *tool)
{
  sinks().erase(tool);
}


void ToolLog::DoLogRecord(wxLogLevel level, const wxString& msg,
                          const wxLogRecordInfo& info)
{
  if (wxLog *sink = sinkNow()) sink->LogRecord(level, msg, info);
  else wxLogGui::DoLogRecord(level, msg, info);
}


void ToolLog::DoShowSingleLogMessage(const wxString& message,
                                     const wxString& title, int)
{
  showBox(title, message);
}


void ToolLog::DoShowMultipleLogMessages(const wxArrayString& messages,
                                        const wxArrayInt&,
                                        const wxArrayLong&,
                                        const wxString& title, int)
{
  wxString text;
  for (size_t i = 0; i < messages.size(); i++)
    text += (i ? "\n" : "") + messages[i];
  showBox(title, text);
}
