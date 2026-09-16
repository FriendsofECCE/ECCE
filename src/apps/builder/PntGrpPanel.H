#ifndef PNTGRPPANEL_H_
#define PNTGRPPANEL_H_

#include "PropertyPanel.H"

// Displays the point group the computational code itself reported
// (PNTGRP, a PropTSVecString<Wave Step>) -- distinct from, and a useful
// cross-check against, Builder's own client-side symmetry detection
// shown in the Calculation Summary panel (Fragment::pointGroup(), set
// by the user's own "Find Symmetry" action before the job ever ran).
class PntGrpPanel : public PropertyPanel
{
  DECLARE_DYNAMIC_CLASS(PntGrpPanel)

  public:
    PntGrpPanel();
    PntGrpPanel(IPropCalculation *calculation,
            wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="PntGrpPanel");
    virtual bool Create(IPropCalculation *calculation,
            wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="PntGrpPanel");
    virtual ~PntGrpPanel();

    virtual void refresh();

  protected:
    virtual void initialize();
};

#endif // PNTGRPPANEL_H_
