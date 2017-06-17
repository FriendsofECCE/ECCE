#ifndef NMODEPANEL__H
#define NMODEPANEL__H

#include "wxgui/TearableContentProvider.H"

#include "NModesGUI.H"

class ewxPlotCtrl;
class wxGrid;
class PropVector;

/**
 * A panel for viewing normal mode animations and vectors.
 */
class NModePanel : public NModesGUI,
                   public TearableContentProvider
{
   DECLARE_DYNAMIC_CLASS(NModePanel)
   DECLARE_EVENT_TABLE()

   enum MenuIds {GRAPH=9740, TABLE };

   static const wxWindowID ID_SLIDER;

   public:
      NModePanel();
      NModePanel(IPropCalculation *calculation,
            wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="NModeVizPropertyPanel");
      virtual bool Create(IPropCalculation *calculation,
            wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="NModeVizPropertyPanel");
      virtual ~NModePanel();
      int getSelectedMode() const;
      void selectMode(int index);
      void showMode(int index);
      void processStep(int step);
      void nextStep();
      void previousStep();
      void start();
      void stop();
      bool isGraphShown();

      void showAnimationMode();
      void showVectorMode();
      void updateVectors();
      void showTable();
      void showGraph();

      virtual void receiveFocus();
      virtual void loseFocus();

   protected:
      void initialize();
      void setSlider();
      void fillTable();
      bool fillGraph();
      void plotCurve(PropVector *x, PropVector *y);
      void updateVectors(int mode,
                         float amplitude,
                         bool sign,
                         string color);


      virtual wxWindow* GetTearableContent();

      void OnMenuClick( wxCommandEvent& event );
      void OnMenuButtonClick( wxCommandEvent& event );
      void OnTextctrlNmodeDelayEnter( wxCommandEvent& event );
      void OnBitmapbuttonStartClick( wxCommandEvent& event );
      void OnBitmapbuttonStopClick( wxCommandEvent& event );
      void OnButtonNmodeVeccolorClick( wxCommandEvent& event );
      void OnEndSliderMotion(wxScrollEvent& event);
      void OnSliderTextEnter(wxCommandEvent& event);
      void OnRadioboxSelected( wxCommandEvent& event );
      void OnModeSelection( wxGridEvent& event );
      void OnCheckboxNmodeVecsignClick( wxCommandEvent& event );
      void OnTimer( wxTimerEvent& event );
      void OnPointClick(wxPlotCtrlEvent& event);
      void OnPlotClick(wxPlotCtrlEvent& event);

   private:

      /* The report oriented table showing modes */
      wxGrid *p_grid;  
      ewxPlotCtrl *p_plotCtrl;

      wxTimer *p_timer;

      /* scaling factor */
      SliderCombo *p_slider;

      /* currently selected row; GetSelected* methods don't work */
      int p_selectedRow;

      int         p_loopSpeed;
      int         p_currentStep;
      int         p_mode;
      int         p_numAnimations;
      bool        p_isValid;
      float       p_vecAmplitude; // varies for vector vs animation
      float       p_aniAmplitude; // varies for vector vs animation

};

#endif // NMODEPANEL_H

