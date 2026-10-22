#ifndef NMODEPANEL__H
#define NMODEPANEL__H

#include <memory>
#include <vector>
#include "wxgui/TearableContentProvider.H"

#include "NModesGUI.H"
#include "SpectrumCanvas.H"
#include "tdat/VibAnalysis.H"

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
      virtual int openingHeight() const { return 440; }

      virtual void receiveFocus();
      virtual void loseFocus();

      /** A peak in the spectrum was clicked: select and animate that mode. */
      virtual void modeClicked(int mode);

      /** The spectrum as CSV (#392); false if the file cannot be written.
          The curve covers the data's full wavenumber range at 1 cm-1. */
      bool exportSpectrumCsv(const wxString& path, VibKind kind);

      /** Another calculation of the same folder as a second curve in the
          spectrum (#173); "" removes it.  False if there is no such
          calculation with IR or Raman data. */
      bool compareWith(const string& name);
      VibKind spectrumKindShown() const;

      /** The structure in the viewer is the one these modes belong to:
          as many atoms as the modes, of the calculation's elements. */
      bool modesApply();

   protected:
      bool checkStructure();
      void initialize();
      void setSlider();
      void fillTable();
      /** Irreps and descriptions of the modes (VibAnalysis) into p_vib. */
      void analyseModes();
      /** Mode column width and text for the grid's current width. */
      void fitModeColumn();
      void showModeDetail(int row);
      /** Sets a description label to `text`, wrapped to two lines at its width. */
      static void setWrapped(wxStaticText *label, const std::vector<wxString>& parts);
      void dumpTableIfRequested();
      wxString cellTip(int row, int col) const;
      void OnGridMotion(wxMouseEvent& event);
      void OnLabelMotion(wxMouseEvent& event);
      bool fillGraph();
      void createSpectrumPane();
      void openSpectrumWindow();
      void dumpSpectrumIfRequested();
      void exportSpectrumDialog();
      void compareDialog();
      vector<IPropCalculation*> comparableCalculations();
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
      std::shared_ptr<VibSpectrum> p_compare;   // the second curve, if any
      string p_compareName, p_compareCode;
      wxButton *p_compareRemove = 0;
      wxTextCtrl *p_fwhmText;
      wxTextCtrl *p_scaleText;

      wxTimer *p_timer;
      bool p_animating;          // from Start until Stop or a structure change

      /* scaling factor */
      SliderCombo *p_slider;

      /* currently selected row; GetSelected* methods don't work */
      int p_selectedRow;
      int p_initialMode = -1;   // the row the panel opened on, for the dump

      int         p_loopSpeed;
      int         p_currentStep;
      //  Grid columns for the optional Infrared/Raman properties, or -1
      //  when the code did not produce them. Tracked because skipping one
      //  shifts the other.
      int p_irColumn;
      int p_ramanColumn;
      int p_symColumn = -1;
      int p_modeColumn = -1;
      /** ECCE's own analysis of each mode, and what the code printed. */
      VibResult p_vib;
      vector<string> p_codeSym;
      vector<wxString> p_modeFull;   // untruncated Mode cells
      //  The selected mode's frequency and description: under the table, and
      //  under the plot in the Graph view, frequency first (p_graphParts).
      wxStaticText *p_modeDetail = NULL;
      wxStaticText *p_graphDetail = NULL;
      std::vector<wxString> p_detailParts;
      std::vector<wxString> p_graphParts;
      wxString p_detailText;          // unwrapped, for re-wrapping on a resize
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

