#ifndef MOCOEFFS__H
#define MOCOEFFS__H

#include "wxgui/ewxDialog.H"

#include "VizPropertyPanel.H"

class WxColorSchemeButton;
class ewxStaticText;

////@begin control identifiers
#define ID_DIALOG_MOCOEFFS 9980
#define ID_MOCOEFFS_TABLE 9981
////@end control identifiers

class wxGrid;
class ICalculation;

/**
 */
class MoCoeffs : public ewxDialog
{
  DECLARE_DYNAMIC_CLASS(MoCoeffs)
  DECLARE_EVENT_TABLE()

  public:
    MoCoeffs();
    MoCoeffs(wxWindow *parent, wxWindowID id=wxID_ANY, const wxString& title="ECCE MO Coefficients", const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE, const wxString& name = "coeffs");
    virtual ~MoCoeffs();
    virtual void showCoeffs(ICalculation *expt, int mo);

   protected:
     void initialize();
     bool Create();

   private:

      /* The report oriented table showing MO energies */
      wxGrid *p_grid;  

};

#endif // MOCOEFFS__H

