#include <cstring>
#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wxgui/ewxDialog.H"
#include "wxgui/ewxButton.H"
#include "wxgui/ewxStaticText.H"
#include <wx/artprov.h>
#include "wxgui/ewxMessageDialog.H"

#include <stdio.h>
#include <stdlib.h>


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
  p_message = message;
  SetFont(getNormalFont());
  p_topSizer = new wxBoxSizer(wxVERTICAL);
  SetSizer(p_topSizer);
  p_labelSizer = new wxBoxSizer(wxHORIZONTAL);
  p_topSizer->Add(p_labelSizer, 1, wxEXPAND | wxALL, 12);

  // Windows and macOS message boxes carry the system's icon; GNOME's carry
  // none (its theme has no full-colour large ones), only the bold heading.
#ifndef __WXGTK__
  wxArtID art = wxART_INFORMATION;
  if (style & wxICON_EXCLAMATION)
    art = wxART_WARNING;
  else if ((style & wxICON_HAND) || (style & wxICON_ERROR))
    art = wxART_ERROR;
  else if (style & wxICON_QUESTION)
    art = wxART_QUESTION;
  wxStaticBitmap * icon = new wxStaticBitmap(this, wxID_STATIC,
      wxArtProvider::GetBitmap(art, wxART_MESSAGE_BOX));
  p_labelSizer->Add(icon, 0, wxALIGN_TOP | wxRIGHT, 12);
#endif

  // A question or headline followed by an explanation: the first paragraph
  // in bold, as desktop message boxes set it, so it reads at a glance.
  wxBoxSizer * text = new wxBoxSizer(wxVERTICAL);
  p_labelSizer->Add(text, 1, wxALIGN_TOP);
  wxString head = message, body;
  int split = message.Find("\n\n");
  if (split != wxNOT_FOUND) {
    head = message.Left(split);
    body = message.Mid(split + 2);
  }
  p_label = new ewxStaticText(this, -1, head);
#ifdef __WXGTK__
  p_label->SetFont(getNormalFont().Bold());
#else
  if (!body.empty())
    p_label->SetFont(getNormalFont().Bold());
#endif
  p_label->Wrap(400);
  text->Add(p_label, 0);
  if (!body.empty()) {
    ewxStaticText * detail = new ewxStaticText(this, -1, body);
    detail->Wrap(400);
    text->Add(detail, 0, wxTOP, 8);
  }

  p_buttonSizer = CreateButtonSizer(0);
#ifdef __WXGTK__
  // GNOME puts Cancel before the action, Windows and macOS after it.
  if (style & wxCANCEL)
    AddButton(wxID_CANCEL, "Cancel");
  if (style & wxOK)
    AddButton(wxID_OK, "OK")->SetDefault();
#else
  if (style & wxOK)
    AddButton(wxID_OK, "OK")->SetDefault();
  if (style & wxCANCEL)
    AddButton(wxID_CANCEL, "Cancel");
#endif
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
  p_topSizer->Add(p_buttonSizer, 0, wxALIGN_RIGHT | wxLEFT | wxRIGHT | wxBOTTOM, 12);

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


void ewxMessageDialog::SetButtonLabel(int id, const wxString& label)
{
  if (wxButton * but = wxDynamicCast(FindWindow(id), wxButton)) {
    but->SetLabel(label);
    Layout();
    Fit();
  }
}


void ewxMessageDialog::OnButtonClick( wxCommandEvent& event )
{
  EndModal(event.GetId());
}


// ECCE_TEST_DIALOG_CLOSE=<seconds>: log the dialog to stderr, then dismiss it
// after that long, so a headless run can see it and still end cleanly.
// ECCE_TEST_DIALOG_ANSWER=yes|ok|cancel picks the button pressed (default ok).
// It also says whether the parent window is frozen.
int ewxMessageDialog::ShowModal()
{
  const char * secs = getenv("ECCE_TEST_DIALOG_CLOSE");
  if (!secs)
    return ewxDialog::ShowModal();
  fprintf(stderr, "ECCE_TEST_DIALOG: [%s] %s\n",
          (const char *)GetTitle().mb_str(),
          (const char *)p_message.mb_str());
  // A frozen parent is not repainted where the dialog uncovers it.
  if (GetParent())
    fprintf(stderr, "ECCE_TEST_DIALOG_PARENT: %s\n",
            GetParent()->IsFrozen() ? "frozen" : "not frozen");
  fflush(stderr);
  wxTimer timer(this);
  const char * ans = getenv("ECCE_TEST_DIALOG_ANSWER");
  int id = wxID_OK;
  if (ans && !strcmp(ans, "yes")) id = wxID_YES;
  else if (ans && !strcmp(ans, "cancel")) id = wxID_CANCEL;
  Bind(wxEVT_TIMER, [this, id](wxTimerEvent&) { EndModal(id); });
  timer.StartOnce(atoi(secs) * 1000);
  return ewxDialog::ShowModal();
}
