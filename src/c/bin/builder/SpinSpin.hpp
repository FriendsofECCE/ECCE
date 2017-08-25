#ifndef SPINSPIN__H
#define SPINSPIN__H


#include "VizPropertyPanel.H"


class SliderCombo;
class wxGrid;


/**
 * Similar to MomentsPanel but slightly different.
 * Designed soley for SPINSPIN properties.
 *
 * The table is made from the SSISOTROPY while the vectors are constructed
 * from SPINSPIN.
 * Run an NWChem SCF/UHF Property calc.
 */
class SpinSpin : public VizPropertyPanel
{
   DECLARE_DYNAMIC_CLASS(SpinSpin)
   DECLARE_EVENT_TABLE()

   public:

      static const wxWindowID ID_DIALOG_SPINSPIN;
      static const wxWindowID ID_SPINSPIN_SLIDER;
      static const wxWindowID ID_BUTTON_SPINSPIN_POSCOLOR;
      static const wxWindowID ID_BUTTON_SPINSPIN_NEGCOLOR;
      static const wxWindowID ID_SPINSPIN_GRID;

      SpinSpin();
      SpinSpin(IPropCalculation *calculation,
            wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="SpinSpin");
      virtual bool Create(IPropCalculation *calculation,
            wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="SpinSpin");
      virtual ~SpinSpin();

      virtual void receiveFocus();
      virtual void loseFocus();

   protected:
      virtual void fillTable();
      virtual void showVectors(bool guessScale);
      virtual string getConfigKey();

      virtual void OnPosColorClick( wxCommandEvent& event);
      virtual void OnNegColorClick( wxCommandEvent& event);
      virtual void OnEndSliderMotion(wxScrollEvent& event);
      virtual void OnSliderTextEnter(wxCommandEvent& event);
      virtual void OnRowSelection(wxGridEvent& event);
      virtual void OnGridRangeSelect(wxGridRangeSelectEvent& event);

      /* The report oriented table showing Moments. Could be dipole... */
      wxGrid *p_grid;  

      SliderCombo *p_slider;

      /* The first time, we want to let the command guess the scale
         but after that, stick with the user's choice. */
      bool p_guessScale;

      vector<int> p_atom1Indices;
      vector<int> p_atom2Indices;
      set<int> p_selectedRows;

};

#endif // SPINSPIN__H
