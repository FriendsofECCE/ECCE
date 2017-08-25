#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wxgui/ewxDialog.H"
#include "wxgui/ewxButton.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxBitmap.H"
#include "wxgui/ewxMessageDialog.H"


/**
 * Constructor.
 * Override styles with ecce styles.
 */
ewxMessageDialog::ewxMessageDialog( wxWindow* parent,
                                    const wxString& message,
                                    const wxString& caption,
				    long style,
                                    const wxPoint& pos,
                                    long extraStyle ) :
  ewxDialog(parent, wxID_ANY, caption, pos,
            wxDefaultSize, wxDEFAULT_DIALOG_STYLE, "ewxMessageDialog")
{
  SetFont(getNormalFont());

  p_topSizer = new wxBoxSizer(wxVERTICAL);
  SetSizer(p_topSizer);

  p_labelSizer = new wxBoxSizer(wxHORIZONTAL);

  p_topSizer->Add(p_labelSizer, 0, wxALL, 5);

  wxString iconName = "msg_information.png";

  if (style & wxICON_INFORMATION)
    iconName = "msg_information.png";
  else if (style & wxICON_EXCLAMATION)
    iconName = "msg_exclamation.png";
  else if ((style & wxICON_HAND) || (style & wxICON_ERROR))
    iconName = "msg_error.png";
  else if (style & wxICON_QUESTION)
    iconName = "msg_question.png";

  wxStaticBitmap * icon = new wxStaticBitmap(this,
                                             wxID_STATIC,
                                             ewxBitmap(iconName,
                                                       wxBITMAP_TYPE_PNG),
                                             wxDefaultPosition);

  p_labelSizer->Add(icon, 0, wxALL, 5);

  p_label = new ewxStaticText(this, -1, message);
  p_label->SetFont(getBoldFont());
  p_label->Wrap(400);

  p_labelSizer->Add(p_label, 0, wxALL, 5);

  p_buttonSizer = CreateButtonSizer(0);
  if (style & wxOK)
    AddButton(wxID_OK, "OK");

  if (style & wxCANCEL)
    AddButton(wxID_CANCEL, "Cancel");

  if (style & wxYES_NO) {
    wxButton * but;
    but = AddButton(wxID_NO, "No");
    if (style & wxNO_DEFAULT)
      but->SetDefault();
    but = AddButton(wxID_YES, "Yes");
    if (style & wxYES_DEFAULT)
      but->SetDefault();
  }

  if (extraStyle & wxYES_NO_TO_ALL) {
    wxButton * but;
    but = AddButton(wxID_NO_TO_ALL, "No to All");
    if (extraStyle & wxNO_TO_ALL_DEFAULT)
      but->SetDefault();
    but = AddButton(wxID_YES_TO_ALL, "Yes to All");
    if (style & wxYES_TO_ALL_DEFAULT)
      but->SetDefault();
  }

  p_topSizer->Add(p_buttonSizer, 0, wxALIGN_RIGHT|wxALL, 5);

  Layout();
  Fit();
  SetAutoLayout(true);
  Center();
}


ewxMessageDialog::~ewxMessageDialog()
{
}


wxButton * ewxMessageDialog::AddButton(int id, wxString label)
{
  wxButton * but = new ewxButton(this, id, label);
  p_buttonSizer->Add(but, 0, wxALIGN_CENTER|wxALL, 5);
  Connect(id, wxEVT_COMMAND_BUTTON_CLICKED,
          wxCommandEventHandler(ewxMessageDialog::OnButtonClick));
  Layout();
  Fit();
  return but;
}


void ewxMessageDialog::OnButtonClick( wxCommandEvent& event )
{
  EndModal(event.GetId());
}


// int ewxMessageDialog::ShowModal()
// {
//   CaptureMouse();
//   return ewxDialog::ShowModal();
// }
