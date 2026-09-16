#include <iostream>
  using std::cout;
  using std::endl;

#include "wx/link.h"
#include "wx/sizer.h"

#include "tdat/PropTSVecString.H"

#include "dsm/IPropCalculation.H"

#include "wxgui/ewxNonBoldLabel.H"
#include "wxgui/ewxStaticText.H"

#include "PntGrpPanel.H"


wxFORCE_LINK_THIS_MODULE(PntGrpPanel)


IMPLEMENT_DYNAMIC_CLASS(PntGrpPanel, PropertyPanel)


PntGrpPanel::PntGrpPanel()
  : PropertyPanel()
{
}


PntGrpPanel::PntGrpPanel(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id,
        const wxPoint& pos, const wxSize& size, long style,
        const wxString& name)
  : PropertyPanel()
{
  Create(calculation, parent, id, pos, size, style, name);
}


bool PntGrpPanel::Create(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id,
        const wxPoint& pos, const wxSize& size, long style,
        const wxString& name)
{
  if (!PropertyPanel::Create(calculation, parent, id, pos, size, style, name)) {
    wxFAIL_MSG( wxT("PntGrpPanel creation failed") );
    return false;
  }

  if (!GetSizer()) {
    SetSizer(new wxFlexGridSizer(2, 0, 5)); // two columns
  }

  return true;
}


PntGrpPanel::~PntGrpPanel()
{
}


void PntGrpPanel::refresh()
{
  DestroyChildren();

  PropTSVecString *prop =
      (PropTSVecString*)getCalculation()->getProperty("PNTGRP");

  if (prop && prop->rows() > 0) {
    GetSizer()->Add(new ewxNonBoldLabel(this, wxID_ANY, "Point Group"),
            0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL);
    GetSizer()->Add(new ewxStaticText(this, wxID_ANY,
            prop->value(prop->rows() - 1)),
            0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL);
  }

  GetSizer()->Fit(this);
  GetSizer()->SetSizeHints(this);
}


void PntGrpPanel::initialize()
{
  refresh();
}
