/**
 * @file
 *
 */

#include "wx/sizer.h"

#include "wxgui/WxPovrayOptions.H"

#include "wxgui/WxPovrayOptionsDialog.H"

WxPovrayOptionsDialog::WxPovrayOptionsDialog( wxWindow* parent, wxWindowID id,
                                              const wxString& title )
  : ewxDialog(parent, id, title),
    p_options(new WxPovrayOptions(this))
{
  wxSizer * sizer = new wxBoxSizer(wxVERTICAL);
  SetSizer(sizer);

  sizer->Add(p_options);
  sizer->Add(CreateStdDialogButtonSizer(wxOK|wxCANCEL), 1, wxALIGN_CENTER);

  sizer->Fit(this);
  sizer->SetSizeHints(this);
}



string WxPovrayOptionsDialog::getFinishStyle() const
{
  return p_options->getFinishStyle();
}


string WxPovrayOptionsDialog::getBondStyle() const
{
  return p_options->getBondStyle();
}


string WxPovrayOptionsDialog::getIsoStyle() const
{
  return p_options->getIsosurfaceStyle();
}


bool WxPovrayOptionsDialog::isDisplayChecked() const
{
  return p_options->isDisplayChecked();
}


int WxPovrayOptionsDialog::getWidth() const
{
  return p_options->getWidth();
}


int WxPovrayOptionsDialog::getHeight() const
{
  return p_options->getHeight();
}

