#include "TextFileViewer.H"
#include <fstream>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/filename.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/utils.h>
#include "dsm/EDSIFactory.H"
#include "util/TempStorage.H"
#include "util/SDirectory.H"
#include "wxgui/ewxStyledWindow.H"

TextFileViewer::TextFileViewer(wxWindow *parent, const EcceURL& url)
  : wxFrame(parent, wxID_ANY, "View — " + wxString::FromUTF8(url.toString()),
            wxDefaultPosition, wxSize(900, 650)), p_url(url),
    p_source(EDSIFactory::getEDSI(url)), p_timer(this)
{
  auto *panel = new wxPanel(this);
  auto *layout = new wxBoxSizer(wxVERTICAL);
  auto *bar = new wxBoxSizer(wxHORIZONTAL);
  p_more = new wxButton(panel, wxID_ANY, "Load more");
  p_follow = new wxCheckBox(panel, wxID_ANY, "Follow");
  p_find = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxSize(200, -1), wxTE_PROCESS_ENTER);
  auto *findButton = new wxButton(panel, wxID_ANY, "Find next");
  auto *editor = new wxButton(panel, wxID_ANY, "Open in editor");
  auto *close = new wxButton(panel, wxID_CLOSE, "Close");
  bar->Add(p_more, 0, wxALL, 4);
  bar->Add(p_follow, 0, wxALL | wxALIGN_CENTER_VERTICAL, 4);
  bar->Add(p_find, 1, wxALL | wxALIGN_CENTER_VERTICAL, 4);
  bar->Add(findButton, 0, wxALL, 4);
  bar->Add(editor, 0, wxALL, 4);
  bar->Add(close, 0, wxALL, 4);
  p_status = new wxStaticText(panel, wxID_ANY, "");
  p_text = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                         wxTE_MULTILINE | wxTE_READONLY | wxTE_DONTWRAP | wxHSCROLL);
  p_text->SetFont(ewxStyledWindow::getMonoSpaceFont());
  layout->Add(bar, 0, wxEXPAND);
  layout->Add(p_status, 0, wxEXPAND | wxALL, 6);
  layout->Add(p_text, 1, wxEXPAND | wxALL, 6);
  panel->SetSizer(layout);
  p_more->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { loadMore(); });
  p_follow->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) { follow(p_follow->GetValue()); });
  auto search = [this](wxCommandEvent&) { find(p_find->GetValue()); };
  p_find->Bind(wxEVT_TEXT_ENTER, search);
  findButton->Bind(wxEVT_BUTTON, search);
  editor->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { openEditor(); });
  close->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { Close(); });
  Bind(wxEVT_TIMER, [this](wxTimerEvent&) { refreshTail(); });
  refreshTail();
}

void TextFileViewer::refreshTail()
{
  if (!p_source) p_reader.error = "No reader for this URL.";
  else p_reader.tail(*p_source);
  if (!p_reader.error.empty() || p_reader.binary) follow(false);
  render(true);
}

void TextFileViewer::render(bool atEnd)
{
  const auto& r = p_reader;
  wxString type = wxFileName(wxString::FromUTF8(p_url.getPath())).GetExt();
  if (type.empty()) type = "unknown extension";
  wxString summary = wxString::Format("%lu bytes; type: %s; ", r.size, type);
  summary += r.binary ? "binary file (NUL in first 8 kB)" :
      wxString::Format("showing bytes %lu–%lu; Find searches loaded text", r.start, r.start + static_cast<unsigned long>(r.text.size()));
  if (!r.error.empty()) summary = wxString::FromUTF8(r.error);
  p_status->SetLabel(summary);
  if (r.binary) p_text->ChangeValue("");
  else {
    wxString content = wxString::FromUTF8(r.text.data(), r.text.size());
    if (content.empty() && !r.text.empty()) content = wxString(r.text.data(), wxConvISO8859_1, r.text.size());
    p_text->ChangeValue(content);
    if (atEnd) p_text->ShowPosition(p_text->GetLastPosition());
  }
  p_more->Enable(!r.binary && r.start > 0 && r.error.empty());
  p_follow->Enable(!r.binary && r.error.empty());
  p_find->Enable(!r.binary);
}

void TextFileViewer::loadMore()
{
  follow(false);
  if (p_source) p_reader.more(*p_source);
  render(false);
}

void TextFileViewer::follow(bool on)
{
  on = on && !p_reader.binary && p_reader.error.empty();
  p_follow->SetValue(on);
  if (on) { p_timer.Start(3000); refreshTail(); }
  else p_timer.Stop();
}

bool TextFileViewer::find(const wxString& needle)
{
  if (needle.empty() || p_reader.binary) return false;
  const wxString text = p_text->GetValue();
  long from, to;
  p_text->GetSelection(&from, &to);
  auto pos = text.find(needle, static_cast<size_t>(to));
  if (pos == wxString::npos) pos = text.find(needle);
  if (pos == wxString::npos) { p_status->SetLabel("Not found in loaded text. Use Load more to search earlier text."); return false; }
  p_text->SetSelection(pos, pos + needle.size());
  p_text->ShowPosition(pos);
  return true;
}

void TextFileViewer::openEditor()
{
  // Keep the copy alive for the editor; TempStorage sweeps stale folders.
  std::unique_ptr<SDirectory> dir(TempStorage::getTempJobDirectory("text-viewer"));
  if (!dir) { p_status->SetLabel("Unable to create the editor copy."); return; }
  const std::string path = dir->path() + "/copy.txt";
  std::ofstream out(path, std::ios::binary);
  if (!p_source || !out || !p_source->getDataSet(out)) {
    p_status->SetLabel("Unable to download the editor copy.");
    out.close();
    dir->remove();
    return;
  }
  out.close();
  if (!out || !wxLaunchDefaultApplication(wxString::FromUTF8(path)))
    p_status->SetLabel("Unable to open the downloaded copy in the system editor.");
}

std::string TextFileViewer::report() const
{
  const auto& r = p_reader;
  return std::string(r.error.empty() ? "ok" : "error: " + r.error) +
      " size=" + std::to_string(r.size) + " start=" + std::to_string(r.start) +
      " bytes=" + std::to_string(r.text.size()) + " read=" + std::to_string(r.bytesRead) +
      " lines=" + std::to_string(std::count(r.text.begin(), r.text.end(), '\n')) +
      " binary=" + (r.binary ? "1" : "0") + " follow=" + (p_timer.IsRunning() ? "1" : "0") +
      " readonly=" + (p_text->IsEditable() ? "0" : "1");
}

void TextFileViewer::snapshotText(const std::string& path) const
{
  std::ofstream out(path, std::ios::binary);
  const wxString value = p_text->GetValue();
  const auto text = value.ToUTF8();
  out.write(text.data(), text.length());
}
