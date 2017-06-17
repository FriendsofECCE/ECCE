#ifndef TENSORTABLE__H
#define TENSORTABLE__H

#include "wxgui/ewxDialog.H"

#include "VizPropertyPanel.H"

class WxColorSchemeButton;
class ewxStaticText;


class wxGrid;
class ICalculation;
class PropVector;

/**
 */
class TensorTable : public ewxDialog
{
  DECLARE_DYNAMIC_CLASS(TensorTable)
  DECLARE_EVENT_TABLE()

  public:
    static const wxWindowID ID_DIALOG_TENSOR;
    static const wxWindowID ID_TENSOR_TABLE;

    TensorTable();
    TensorTable(wxWindow *parent, wxWindowID id=wxID_ANY, const wxString& title="ECCE Tensor Data", const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE, const wxString& name = "tenors");
    virtual ~TensorTable();
    virtual void fillTable(PropVector *vec, int index);

   protected:
     void initialize();
     bool Create();

   private:

      /* The table showing Tensor data used to generate vectors */
      wxGrid *p_grid;  

      /* Atom label from row labels */
      ewxStaticText *p_label;

      /* units from property */
      ewxStaticText *p_units;

};

#endif // TENSORTABLE__H

