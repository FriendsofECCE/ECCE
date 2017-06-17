#ifndef MOMENTSPANEL__H
#define MOMENTSPANEL__H


#include "VizPropertyPanel.H"


class SliderCombo;
class wxGrid;


/**
 * A class for displaying dipole type moments.
 * It is customized to show a property with just 1 row corresponding to
 * X,Y,Z values.  
 * Subclass for a more generic display - see QuadPolePanel
 */
class MomentsPanel : public VizPropertyPanel
{
   DECLARE_DYNAMIC_CLASS(MomentsPanel)
   DECLARE_EVENT_TABLE()

   public:

      static const wxWindowID ID_DIALOG_MOMENTS;
      static const wxWindowID ID_MOMENTS_SLIDER;
      static const wxWindowID ID_BUTTON_MOMENTS_VECCOLOR;
      static const wxWindowID ID_MOMENTS_GRID;

      MomentsPanel();
      MomentsPanel(IPropCalculation *calculation,
            wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="Moments");
      virtual bool Create(IPropCalculation *calculation,
            wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="Moments");
      virtual ~MomentsPanel();

      virtual void receiveFocus();
      virtual void loseFocus();

   protected:
      virtual void fillTable();
      virtual void showVectors(bool guessScale);
      virtual string getConfigKey();

      virtual void OnVecColorClick( wxCommandEvent& event);
      virtual void OnEndSliderMotion(wxScrollEvent& event);
      virtual void OnSliderTextEnter(wxCommandEvent& event);

      /* The report oriented table showing Moments. Could be dipole... */
      wxGrid *p_grid;  

      SliderCombo *p_slider;

      /* The first time, we want to let the command guess the scale
         but after that, stick with the user's choice. */
      bool p_guessScale;

};

#endif // MOMENTSPANEL_H

