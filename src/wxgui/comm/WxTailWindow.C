#include "wxgui/WxTailWindow.H"

#include <wx/app.h>
#include <wx/dcclient.h>
#include <wx/dcmemory.h>
#include <wx/image.h>
#include <wx/sizer.h>
#include <wx/textctrl.h>
#include <wx/utils.h>

#include "wxgui/ewxButton.H"
#include "wxgui/ewxPanel.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxStyledWindow.H"

namespace {

// Bytes at the end of s that begin a UTF-8 sequence not yet complete.
size_t incompleteUtf8(const std::string& s)
{
  for (size_t back = 1; back <= 3 && back <= s.size(); back++) {
    unsigned char c = (unsigned char)s[s.size() - back];
    if ((c & 0xC0) == 0x80) continue;          // continuation byte
    size_t need = (c & 0xE0) == 0xC0 ? 2 : (c & 0xF0) == 0xE0 ? 3 :
                  (c & 0xF8) == 0xF0 ? 4 : 1;
    return need > back ? back : 0;
  }
  return 0;
}

wxString decode(const std::string& s)
{
  wxString w(s.c_str(), wxConvUTF8, s.size());
  // Output that is not UTF-8 (old codes print Latin-1) is shown byte by byte.
  if (w.empty() && !s.empty()) w = wxString(s.c_str(), wxConvISO8859_1, s.size());
  return w;
}

int countLines(const std::string& s)
{
  int n = 0;
  for (size_t i = 0; i < s.size(); i++) if (s[i] == '\n') n++;
  return n;
}

}  // namespace


WxTailWindow* WxTailWindow::open(wxWindow* parent, const std::string& title,
                                 const std::string& machineName,
                                 const std::string& shell,
                                 const std::string& user,
                                 const std::string& path, std::string& error,
                                 int maxLines)
{
  WxTailWindow* w = new WxTailWindow(parent, title, path, maxLines);
  if (!w->p_source.open(machineName, shell, user, path, maxLines, error,
                        &w->p_missing)) {
    w->Destroy();
    return 0;
  }
  w->setStatus();
  w->Show();
  w->p_timer.Start(250);
  return w;
}


WxTailWindow::WxTailWindow(wxWindow* parent, const std::string& title,
                           const std::string& path, int maxLines)
  : ewxFrame(parent, wxID_ANY, wxString::FromUTF8(("Tail: " + title).c_str()),
             wxDefaultPosition, wxSize(760, 520)),
    p_path(path), p_maxLines(maxLines < 1 ? 1 : maxLines), p_lines(0),
    p_paused(false), p_ended(false), p_missing(false), p_timer(this)
{
  ewxPanel* panel = new ewxPanel(this);
  wxBoxSizer* top = new wxBoxSizer(wxVERTICAL);

  p_text = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition,
                          wxDefaultSize,
                          wxTE_MULTILINE | wxTE_READONLY | wxTE_DONTWRAP |
                          wxTE_RICH2);
  p_text->SetFont(ewxStyledWindow::getMonoSpaceFont());
  top->Add(p_text, 1, wxEXPAND | wxALL, 6);

  wxBoxSizer* row = new wxBoxSizer(wxHORIZONTAL);
  p_status = new ewxStaticText(panel, wxID_ANY, "");
  row->Add(p_status, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
  p_pause = new ewxButton(panel, wxID_ANY, _("Pause"));
  row->Add(p_pause, 0, wxRIGHT, 6);
  ewxButton* closeButton = new ewxButton(panel, wxID_CLOSE, _("Close"));
  row->Add(closeButton, 0);
  top->Add(row, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);

  panel->SetSizer(top);

  p_pause->Bind(wxEVT_BUTTON, &WxTailWindow::OnPause, this);
  closeButton->Bind(wxEVT_BUTTON, &WxTailWindow::OnCloseButton, this);
  Bind(wxEVT_TIMER, &WxTailWindow::OnTimer, this, p_timer.GetId());
  Bind(wxEVT_CLOSE_WINDOW, &WxTailWindow::OnClose, this);
}


std::string WxTailWindow::text() const
{
  return std::string(p_text->GetValue().utf8_str());
}


bool WxTailWindow::snapshot(const std::string& png)
{
  for (int i = 0; i < 3; i++) {
    Update();
    wxTheApp->Yield(true);
    wxMilliSleep(50);
  }
  wxSize sz = GetClientSize();
  wxClientDC screen(this);
  wxBitmap bmp(sz.x, sz.y);
  wxMemoryDC mem(bmp);
  mem.Blit(0, 0, sz.x, sz.y, &screen, 0, 0);
  mem.SelectObject(wxNullBitmap);
  return bmp.ConvertToImage().SaveFile(wxString::FromUTF8(png.c_str()),
                                       wxBITMAP_TYPE_PNG);
}


void WxTailWindow::OnTimer(wxTimerEvent&)
{
  if (p_ended) return;
  std::string data;
  if (!p_source.read(data)) {
    p_ended = true;
    p_timer.Stop();
  }
  if (!data.empty()) {
    p_pending += data;
    if (p_missing) p_missing = false;
    if (!p_paused) showPending();
    else {
      // Keep no more than the view would.
      int extra = countLines(p_pending) - p_maxLines;
      for (size_t at = 0; extra > 0; extra--) {
        at = p_pending.find('\n', at);
        if (at == std::string::npos) break;
        p_pending.erase(0, at + 1);
        at = 0;
      }
    }
  }
  if (!data.empty() || p_ended) setStatus();
}


void WxTailWindow::showPending()
{
  if (p_ended) {
    append(p_pending);
    p_pending.clear();
    return;
  }
  size_t keep = incompleteUtf8(p_pending);
  std::string now = p_pending.substr(0, p_pending.size() - keep);
  p_pending.erase(0, now.size());
  append(now);
}


void WxTailWindow::append(const std::string& data)
{
  if (data.empty()) return;
  p_text->AppendText(decode(data));
  p_lines += countLines(data);
  // Trimmed in batches, so a busy file does not rewrite the view per line.
  if (p_lines > p_maxLines + p_maxLines / 10) {
    int drop = p_lines - p_maxLines;
    wxString v = p_text->GetValue();
    long pos = 0;
    for (int i = 0; i < drop; i++) {
      int nl = v.find('\n', pos);
      if (nl == wxNOT_FOUND) break;
      pos = nl + 1;
    }
    p_text->Remove(0, pos);
    p_lines -= drop;
    p_text->ShowPosition(p_text->GetLastPosition());
  }
}


void WxTailWindow::setStatus()
{
  wxString how = p_source.backend() == "local" ? _("on this computer")
                 : _("over this session's ssh login");
  const size_t slash = p_path.rfind('/');
  const wxString name = wxString::FromUTF8(
    (slash == std::string::npos ? p_path : p_path.substr(slash + 1)).c_str());
  wxString s;
  if (p_ended)
    s = _("Ended: tail stopped or the connection closed.");
  else if (p_paused)
    s = wxString::Format(_("Paused; %d new lines waiting."),
                         countLines(p_pending));
  else if (p_missing)
    s = _("Waiting for ") + name + _(" to appear, ") + how + ".";
  else
    s = _("Following ") + name + " " + how +
        wxString::Format(_(", last %d lines kept."), p_maxLines);
  p_status->SetLabel(s);
  p_pause->Enable(!p_ended);
}


void WxTailWindow::OnPause(wxCommandEvent&)
{
  p_paused = !p_paused;
  p_pause->SetLabel(p_paused ? _("Resume") : _("Pause"));
  if (!p_paused) showPending();
  setStatus();
}


void WxTailWindow::OnCloseButton(wxCommandEvent&)
{
  Close();
}


void WxTailWindow::OnClose(wxCloseEvent&)
{
  p_timer.Stop();
  p_source.close();
  Destroy();
}
