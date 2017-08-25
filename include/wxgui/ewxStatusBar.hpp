#ifndef _EWXSTATUSBARTOOL_H  
#define _EWXSTATUSBARTOOL_H

#include "wx/statusbr.h"
#include "wx/bmpbuttn.h"
#include "wxgui/ewxStyledWindow.H"
#include "dsm/ResourceDescriptor.H"

class WxState;
class FeedbackSaveHandler;

class ewxStatusBar : public wxStatusBar, public ewxStyledWindow
{
public:
  enum {
    FIELD_SAVE,
    FIELD_LOG,
    FIELD_RUNSTATE,
    FIELD_URL,
    FIELD_MAX
  };

  ewxStatusBar();
  ewxStatusBar(wxWindow *parent, wxWindowID id = wxID_ANY,
               long style = wxST_SIZEGRIP,
               const wxString& name = "ewxStatusBar");
  bool Create(wxWindow *parent, wxWindowID id = wxID_ANY,
              long style = wxST_SIZEGRIP,
              const wxString& name = "ewxStatusBar");
  virtual ~ewxStatusBar();

  virtual void SetStatusText(const wxString& text, int i=1);
  virtual void setSaveHandler(FeedbackSaveHandler *handler);
  virtual void setEditStatus(string saveStatus);
  virtual void setRunState(long state);
  virtual void showRunState(bool show);
  virtual void setContext(string url);

  virtual string getEditStatus() const;
  virtual ResourceDescriptor::RUNSTATE getRunState() const;
  virtual string getContext() const;


  void OnSize(wxSizeEvent& event);
  void OnButton(wxCommandEvent& event);

private:

  void Init();
  wxBitmap CreateBitmapForButton(bool on = false);

  wxBitmapButton * p_save;
  WxState * p_runstate;
  FeedbackSaveHandler * p_saveHandler;
  string p_editStatus;

  DECLARE_EVENT_TABLE()
};

#endif
