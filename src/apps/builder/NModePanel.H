#ifndef NMODEPANEL__H
#define NMODEPANEL__H

#include <memory>
#include <vector>
#include "wxgui/TearableContentProvider.H"
#include "util/Listener.H"

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
                   public SpectrumClickTarget,
                   public ecce::Listener
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
      /// Upper end of the slider: arrow length in Vector, amplitude in
      /// Animation (beyond 1.5 the molecule flies apart).
      static constexpr double MAX_SCALE = 5.0;
      static constexpr double MAX_AMPLITUDE = 1.5;
      double maxScale();
      double sliderValue();
      void restoreHome();
      /// A value typed into the slider's field, as the panel then uses it.
      double typeScale(double value);
      /** Space in the viewer: pause or resume a running Animation.
          False, and nothing done, when no animation is running or paused. */
      bool togglePause();
      bool isAnimating() const { return p_animating; }
      bool isPaused() const { return p_paused; }
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

      /** Test hook (#445): a left click on mode `mode` (0-based) in the
          graph; `mismatch` gets the share of canvas pixels on screen that
          differ from the layout the click is tested on, `selected` the
          mode selected afterwards.  False if the graph is hidden or the
          mode has no stick. */
      bool scriptSpectrumClick(int mode, double *mismatch, int *selected);
      wxString scriptSpectrumFactor(const wxString& text);

      /** Another calculation of the same folder as a second curve in the
          spectrum (#173); "" removes it.  False if there is no such
          calculation with IR or Raman data. */
      bool compareWith(const string& name);
      VibKind spectrumKindShown() const;

      /** The Mode composition window (#486), with `extra` added to the
          automatic coordinate set: coordinates as 1-based atom numbers
          joined by dashes (2 = stretch, 3 = bend, 4 = torsion), separated
          by commas or spaces.  False, with `error` set, if `extra` is not
          understood or the calculation has no modes. */
      bool showComposition(const wxString& extra, wxString *error = 0);

      /** Colour the spectrum by the share of each mode in the atoms
          (0-based) selected in the viewer; fewer than two clears it.
          Returns the key text shown ("" when cleared).  Also called by the
          viewer's selection events. */
      std::string highlightAtoms(const std::vector<int>& atoms);

      virtual void eventMCB(const ecce::Event& event);

      /** Test hook (#486): the composition table as text ("coords" line,
          then one line per mode); optionally the window's picture. */
      bool scriptComposition(const wxString& extra, const wxString& textPath,
                             const wxString& pngPath);

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
      void wrapAll(wxStaticText *label, const std::vector<wxString>& parts);
      static void setWrapped(wxStaticText *label, const std::vector<wxString>& parts);
      void dumpTableIfRequested();
      void dumpCompositionIfRequested();
      bool compositionRows(const wxString& extra, VibComposition& comp,
                           vector<double>& freqs, wxString *error);
      void fillComposition();
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
      wxWeakRef<wxFrame> p_compFrame;            // Mode composition (#486)
      wxGrid *p_compGrid = 0;
      wxTextCtrl *p_compExtra = 0;
      std::vector<int> p_picked;          // atoms selected in the 3-D view, 0-based
      std::string pickedCoordinate() const;
      wxStaticText *p_compNote = 0;
      void fitCompColumn();
      vector<string> p_compLabels;               // short label per mode, as in the table
      VibComposition p_comp;
      vector<double> p_compFreqs;
      std::shared_ptr<VibSpectrum> p_compare;   // the second curve, if any
      string p_compareName, p_compareCode;
      wxButton *p_compareRemove = 0;
      wxTextCtrl *p_fwhmText;
      wxTextCtrl *p_scaleText;

      wxTimer *p_timer;
      std::vector<double> p_home;   // atoms as they were before the animation
      bool p_active = false;     // the panel has taken the viewer
      bool p_paused = false;     // Space stopped the timer, mode still Animation
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
      int p_modeUserWidth = 0;   ///< set once the user drags the Mode column
      /** ECCE's own analysis of each mode, and what the code printed. */
      VibResult p_vib;
      vector<string> p_codeSym;
      vector<wxString> p_modeFull;   // untruncated Mode cells
      //  The selected mode's frequency and description: under the table, and
      //  under the plot in the Graph view, frequency first (p_graphParts).
      wxPanel *p_modeBox = NULL;      // the box around p_modeDetail
      wxStaticText *p_modeDetail = NULL;
      wxStaticText *p_graphDetail = NULL;
      wxPanel *p_graphBox = NULL;     // its own box under the plot
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

