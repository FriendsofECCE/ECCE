/**
 * @file
 *
 *
 */
#ifndef _EWXMESSAGEDIALOG_H  
#define _EWXMESSAGEDIALOG_H  

#include "wx/sizer.h"
#include "wx/collpane.h"
#include "wxgui/ewxDialog.H"
#include "wxgui/ewxStaticText.H"


#define wxYES_NO_TO_ALL      1
#define wxCANCEL_DEFAULT     2
#define wxOK_DEFAULT         4
#define wxYES_TO_ALL_DEFAULT 8
#define wxNO_TO_ALL_DEFAULT  16

#define wxID_YES_TO_ALL 106701
#define wxID_NO_TO_ALL  106702


/**
 * Subclass wxMessageDialog to provide style changes.
 */
class ewxMessageDialog : public ewxDialog
{
public:
  ewxMessageDialog( wxWindow* parent,
                    const wxString& message,
                    const wxString& caption = "Message box",
                    long style = wxDEFAULT_DIALOG_STYLE,
                    const wxPoint& pos = wxDefaultPosition,
                    long extraStyle = 0);

  ~ewxMessageDialog();

  virtual int ShowModal();
  wxButton * AddButton(int id, wxString label);
  // Name a standard button after its action, e.g. OK -> "Quit".
  void SetButtonLabel(int id, const wxString& label);
  // A closed pane under the message holding text that is only for those who
  // want it (a tool's original message); the test hook logs it too.
  void AddDetails(const wxString& title, const wxString& details);

protected:
  void OnButtonClick( wxCommandEvent& event );
  void OnPaneChanged( wxCollapsiblePaneEvent& event );

  wxBoxSizer * p_topSizer;
  wxBoxSizer * p_labelSizer;
  wxBoxSizer * p_textSizer;
  wxString p_details;
  ewxStaticText * p_label;
  wxString p_message;   // whole text; p_label holds its first paragraph
  wxSizer * p_buttonSizer;
};

#endif
