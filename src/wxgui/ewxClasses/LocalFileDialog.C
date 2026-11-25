#include <cstdio>
#include <cstdlib>

#include <wx/filedlg.h>
#include <wx/filename.h>

#include "wxgui/ewxConfig.H"
#include "wxgui/LocalFileDialog.H"

bool ecceLocalFile(wxWindow *parent, const wxString& title,
                   const wxString& wildcard, long style, const wxString& key,
                   wxString& path, int *filterIndex)
{
  if (const char *test = getenv("ECCE_TEST_LOCAL_FILE")) {
    path = wxString::FromUTF8(test);
    if (filterIndex) *filterIndex = 0;
    fprintf(stderr, "LOCALFILE: %s -> %s\n", (const char*)key.utf8_str(), test);
    return !path.empty();
  }
  ewxConfig *config = ewxConfig::getConfig("LocalFileDialog.ini");
  wxString dir;
  if (!config->Read(key, &dir) || !wxFileName::DirExists(dir))
    dir = wxFileName::GetHomeDir();
  wxFileDialog dlg(parent, title, dir, wxEmptyString, wildcard, style);
  if (dlg.ShowModal() != wxID_OK) return false;
  path = dlg.GetPath();
  if (filterIndex) *filterIndex = dlg.GetFilterIndex();
  config->Write(key, wxFileName(path).GetPath());
  config->Flush();
  return true;
}
