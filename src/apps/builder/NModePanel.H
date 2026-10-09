#ifndef NMODEPANEL__H
#define NMODEPANEL__H

#include <vector>
#include "wxgui/TearableContentProvider.H"

#include "NModesGUI.H"
#include "SpectrumCanvas.H"

#include <wx/frame.h>
#include <wx/weakref.h>

class wxCheckBox;
class wxGrid;
class wxTextCtrl;
class PropVector;

/**
 * A panel for viewing normal mode animations and vectors.
 */
class NModePanel : public NModesGUI,
                   public TearableContentProvider,
                   public SpectrumClickTarget
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
      int animationDelay();
      bool isGraphShown();

      void showAnimationMode();
      void showVectorMode();
      void updateVectors();
      void showTable();
      void showGraph();

      /** Room for the compact spectrum, not just a grip. */
      virtual int minimumHeight() const { return 360; }

      virtual void receiveFocus();
      virtual void loseFocus();

      /** A peak in the spectrum was clicked: select and animate that mode. */
      virtual void modeClicked(int mode);

      /** The structure in the viewer is the one these modes belong to:
          as many atoms as the modes, of the calculation's elements. */
      bool modesApply();

   protected:
      bool checkStructure();
      void initialize();
      void setSlider();
      void fillTable();
      bool fillGraph();
      void createSpectrumPane();
      void openSpectrumWindow();
      void dumpSpectrumIfRequested();
      void applySpectrumSettings();
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
      /** Graph-or-table for the frequency list (ID_RADIOBOX_NMODE_DATAVIEW). */
      void OnDataViewSelected( wxCommandEvent& event );
      void OnRadioboxUpdateUI( wxUpdateUIEvent& event );
      void OnModeSelection( wxGridEvent& event );
      void OnCheckboxNmodeVecsignClick( wxCommandEvent& event );
      void OnTimer( wxTimerEvent& event );

   private:

      /* The report oriented table showing modes */
      wxGrid *p_grid;  
      SpectrumCanvas *p_spectrum;
      wxWeakRef<wxFrame> p_spectrumFrame;        // "Open in window"
      wxWeakRef<SpectrumCanvas> p_spectrumPop;
      wxTextCtrl *p_fwhmText;
      wxTextCtrl *p_scaleText;

      wxTimer *p_timer;
      bool p_animating;          // from Start until Stop or a structure change

      /* scaling factor */
      SliderCombo *p_slider;

      /* currently selected row; GetSelected* methods don't work */
      int p_selectedRow;

      int         p_loopSpeed;
      int         p_currentStep;
      //  Grid columns for the optional Infrared/Raman properties, or -1
      //  when the code did not produce them. Tracked because skipping one
      //  shifts the other.
      int p_irColumn;
      int p_ramanColumn;
      int         p_mode;
      int         p_numAnimations;
      bool        p_isValid;
      int         p_lastRadioSel;
      float       p_vecAmplitude; // varies for vector vs animation
      float       p_aniAmplitude; // varies for vector vs animation
      //  Atomic numbers of the calculation's structure, in atom order.
      vector<int> p_calcElements;
      bool        p_structureOk;
      wxStaticText *p_staleNote;

};

#endif // NMODEPANEL_H

