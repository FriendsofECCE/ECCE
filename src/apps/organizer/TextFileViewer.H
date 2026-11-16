#ifndef ECCE_TEXT_FILE_VIEWER_H
#define ECCE_TEXT_FILE_VIEWER_H
#include <memory>
#include <wx/frame.h>
#include <wx/timer.h>
#include "TextFileReader.H"
class wxTextCtrl;
class wxButton;
class wxCheckBox;
class wxStaticText;

class TextFileViewer : public wxFrame {
public:
  TextFileViewer(wxWindow *parent, const EcceURL& url);
  void loadMore();
  void follow(bool on);
  bool find(const wxString& needle);
  std::string report() const;
  void snapshotText(const std::string& path) const;
private:
  void refreshTail();
  void render(bool atEnd);
  void openEditor();
  EcceURL p_url;
  std::unique_ptr<EDSI> p_source;
  TextFileReader p_reader;
  wxTimer p_timer;
  wxTextCtrl *p_text, *p_find;
  wxStaticText *p_status;
  wxButton *p_more;
  wxCheckBox *p_follow;
};
#endif
