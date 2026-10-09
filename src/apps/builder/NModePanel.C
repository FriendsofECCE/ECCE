#include <iostream>
  using std::cout;
  using std::cerr;
  using std::endl;
#include <algorithm>
  using std::fill;
#include <limits>

#include <wx/checkbox.h>
#include <wx/dcmemory.h>
#include <wx/link.h>
#include <wx/listctrl.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/tooltip.h>
#include <wx/wrapsizer.h>

#include <cstdio>
#include <cstdlib>

#include "util/EventDispatcher.H"
#include "util/InternalException.H"
#include "util/PreferenceLabels.H"

#include "tdat/Fragment.H"
#include "tdat/PropVecTable.H"
#include "tdat/PropVector.H"
#include "tdat/TAtm.H"
#include "tdat/PropVecString.H"

#include "dsm/ICalculation.H"
#include "dsm/IPropCalculation.H"

#include "wxgui/ewxButton.H"
#include "wxgui/ewxColorDialog.H"
#include "wxgui/ewxConfig.H"
#include "wxgui/ewxNumericValidator.H"
#include "wxgui/ewxTextCtrl.H"
#include "wxgui/ewxWindowUtils.H"
#include "wxgui/SliderCombo.H"
#include "wxgui/WindowEvent.H"

#include "viz/NModeStepCmd.H"
#include "viz/NModeTraceCmd.H"
#include "viz/NModeVectCmd.H"
#include "viz/SGContainer.H"
#include "viz/SGFragment.H"

#include "wxviz/SGSelection.H"
#include "wxviz/WxVizToolFW.H"

#include "NModePanel.H"

  using std::find;


static const char *INIFILE = "wxbuilder.ini";

const wxWindowID NModePanel::ID_SLIDER = wxNewId();

// TODO
// . The table doesn't resize properly in its panel if other tools get added
//   and so you can't find the compute button.
// . get wxgrid to scroll to the selected item.
// . connect with focus control


// ----------------------------------------------------------------------------


BEGIN_EVENT_TABLE( NModePanel, NModesGUI )

    EVT_MENU( wxID_ANY, NModePanel::OnMenuClick )
    EVT_SCROLL(NModePanel::OnEndSliderMotion)
    EVT_TEXT_ENTER(ID_SLIDER, NModePanel::OnSliderTextEnter)
    EVT_GRID_SELECT_CELL(NModePanel::OnModeSelection)

    EVT_RADIOBOX( ID_RADIOBOX_NMODE_VIZTYPE, NModePanel::OnRadioboxSelected )
    EVT_UPDATE_UI( ID_RADIOBOX_NMODE_VIZTYPE, NModePanel::OnRadioboxUpdateUI )
    EVT_TIMER(wxID_ANY, NModePanel::OnTimer)

END_EVENT_TABLE()


IMPLEMENT_DYNAMIC_CLASS(NModePanel, NModesGUI)


NModePanel::NModePanel()
  : NModesGUI(),
    TearableContentProvider(),
    p_grid(NULL),
    p_spectrum(NULL),
    p_fwhmText(NULL),
    p_scaleText(NULL),
    p_timer(NULL), p_animating(false),
    p_slider(NULL),
    p_selectedRow(0),
    p_loopSpeed(0),
    p_currentStep(0),
    p_mode(0),
    p_numAnimations(0),
    p_isValid(false),
    p_lastRadioSel(-1),
    p_structureOk(true),
    p_staleNote(NULL)
{
   p_vecAmplitude = 1.0;
   p_aniAmplitude = 1.0;
}


NModePanel::NModePanel(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id, const wxPoint& pos,
        const wxSize& size, long style, const wxString& name)
  : NModesGUI(),
    TearableContentProvider(),
    p_grid(NULL),
    p_spectrum(NULL),
    p_fwhmText(NULL),
    p_scaleText(NULL),
    p_timer(NULL), p_animating(false),
    p_slider(NULL),
    p_selectedRow(0),
    p_loopSpeed(0),
    p_currentStep(0),
    p_mode(0),
    p_numAnimations(0),
    p_isValid(false),
    p_lastRadioSel(-1),
    p_structureOk(true),
    p_staleNote(NULL)
{
   Create(calculation, parent, id, pos, size, style, name);
   p_vecAmplitude = 1.0;
   p_aniAmplitude = 1.0;
}

NModePanel::~NModePanel()
{
   //  The animation timer must not fire into a destroyed panel.
   delete p_timer;
   //  The window's canvas calls back into this panel.
   if (p_spectrumPop) p_spectrumPop->setClickHandler(0);
   if (p_spectrumFrame) p_spectrumFrame->Destroy();
}



bool NModePanel::Create(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id, const wxPoint& pos,
        const wxSize& size, long style, const wxString& name)
{
   if (!NModesGUI::Create(calculation, parent, id, pos, size, style )) {
      wxFAIL_MSG( wxT("NModePanel creation failed") );
      return false;
   }

   p_loopSpeed = 50;    // seems reasonable; overridden by prefs
   p_numAnimations = 15; // a reasonable guess
   p_currentStep = 0;
   p_isValid = false;
   p_mode = 0;
   p_irColumn = -1;
   p_ramanColumn = -1;

   p_timer = new wxTimer(this);
   ewxConfig *config = ewxConfig::getConfig(INIFILE);
   createSpectrumPane();

   int vid = GRAPH;
   config->Read("NMode/View", &vid, GRAPH);
   if (vid == GRAPH)
      showGraph();
   else
      showTable();

   // Customizations of DB code
   p_grid = (wxGrid*)FindWindow(ID_GRID_NMODE);
   p_grid->SetRowLabelSize(0);

   wxString color = config->Read("NMode/Color", "yellow");
   ewxButton *btn = (ewxButton*)FindWindow(ID_BUTTON_NMODE_VECCOLOR);
   btn->SetBackgroundColour(wxColour(color));

   // Add the slider
   ewxPanel *p = new ewxPanel(this);
   p_sliderSizer->Add(p, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);
   p_slider = new SliderCombo(p, ID_SLIDER);
   p_slider->SetRange((float)0.0, (float)30.0);
   p_slider->SetToolTip("Scale");


   // Nicer to open in vector display?
   wxRadioBox *radbox = (wxRadioBox*)FindWindow(ID_RADIOBOX_NMODE_VIZTYPE);
   radbox->SetSelection(1);

   // Issue #81: the Animation/Vector radio box's selection never reached
   // OnRadioboxSelected() under wx3.2/GTK3 -- verified at syscall level by
   // an earlier session (the native GTK bullet moves, the handler is never
   // entered), which left the whole display-mode switch dead: no row swap,
   // no vector/animate switch, and the Play button never appears, so the
   // animation can never be started at all.
   //
   // Two things are wired up here rather than relying on the static event
   // table, which is what was failing:
   //
   // 1. A dynamic Bind() directly on the radio box instance. The static
   //    EVT_RADIOBOX entry relies on the command event propagating from
   //    the control up to this panel, and ewxRadioBox::Create() pushes an
   //    ewxHelpHandler onto the control's own handler chain
   //    (PushEventHandler), so that propagation path is not the plain one
   //    wx documents. Binding on the widget itself delivers the event to
   //    this handler directly, without depending on it.
   //
   // 2. wxWS_EX_PROCESS_UI_UPDATES, which the existing
   //    OnRadioboxUpdateUI() idle-poll fallback needs in order to receive
   //    anything at all. wxGTK does not send wxUpdateUIEvent to ordinary
   //    child controls during idle unless they ask for it, so that
   //    fallback (added 2026-09-17 and reported as making "no visible
   //    difference") was inert for a concrete reason, not a mysterious
   //    one. With the style set it becomes a real second line of defence.
   //
   // Both are kept deliberately, the same layered approach used for the
   // #78 construction-order fix: if the Bind() works the poll is a no-op,
   // and if the Bind() somehow doesn't, the poll now actually runs.
   radbox->Bind(wxEVT_RADIOBOX, &NModePanel::OnRadioboxSelected, this);
   radbox->SetExtraStyle(radbox->GetExtraStyle() | wxWS_EX_PROCESS_UI_UPDATES);

   //  Graph/Table, bound the same way and for the same reason -- see the
   //  note above about EVT_RADIOBOX not arriving on this panel through the
   //  static table.  Its initial selection mirrors whatever initialize()
   //  already chose from the NMode/View preference, so the control always
   //  reflects the view actually on screen.
   wxRadioBox *viewbox = (wxRadioBox*)FindWindow(ID_RADIOBOX_NMODE_DATAVIEW);
   if (viewbox != 0) {
     viewbox->Bind(wxEVT_RADIOBOX, &NModePanel::OnDataViewSelected, this);
     viewbox->SetSelection(isGraphShown() ? 0 : 1);
   }

   int delay;
   config->Read("NMode/Delay",&delay,20);
   ewxTextCtrl *text = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_NMODE_DELAY);
   text->setValueAsInt(delay);
   p_loopSpeed = delay;

   // Construction is fully finished as of here -- every control
   // OnModeSelection()/showMode() touch via FindWindow() actually
   // exists, so it's safe to stop ignoring premature grid selection
   // events (see the guards in OnModeSelection() and showMode()).
   Fragment calcFrag;
   if (calculation && calculation->getFragment(calcFrag)) {
      for (size_t i = 0; i < calcFrag.numAtoms(); i++)
         p_calcElements.push_back(calcFrag.atomRef(i)->atomicNumber());
   }
   p_staleNote = new wxStaticText(this, wxID_ANY,
         "The structure has been changed since this calculation ran. "
         "Its normal modes belong to the calculated structure and are "
         "not shown.");
   p_staleNote->Wrap(300);
   p_staleNote->Hide();
   GetSizer()->Insert(0, p_staleNote, 0, wxEXPAND|wxALL, 5);

   p_isValid = true;

   initialize();

   GetSizer()->Fit(this);
   GetSizer()->SetSizeHints(this);


   return true;
}



void NModePanel::initialize()
{
   bool haveGraphable = false;
   fillTable();
   haveGraphable = fillGraph();


   ewxTextCtrl *txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_NMODE_DELAY);
   ewxNumericValidator val;
   val.setHardRange("[10,50000)");
   val.setValue(50);
   txt->SetValidator(val);

   TransferDataFromWindow();
   txt->SetValue(wxString::Format (_T("%d"), p_loopSpeed));

   showMode(p_mode);

   if (!haveGraphable) {
      // Tricky situation.  Table looks better but don't want to override
      // preferences.  Therefore call showTable.
      showTable();

      //  And say so on the control.  Falling back without moving the
      //  radio left it reading "Graph" over a table, which looks like
      //  the radio has stopped working -- the exact complaint #81 was
      //  about, arriving by a different route.
      wxRadioBox *viewbox =
            (wxRadioBox*)FindWindow(ID_RADIOBOX_NMODE_DATAVIEW);
      if (viewbox != 0 && viewbox->GetCount() > 1) {
         viewbox->SetSelection(1);   // 0 = Graph, 1 = Table
      }
   }

   getFW().getViewer().getSel()->deselectAll();
   getFW().getViewer().viewAll();

   dumpSpectrumIfRequested();
}

/**
 * Graph or table for the frequency list.
 *
 * Independent of the Animation/Vector choice, which is about the 3-D
 * viewer. Before this control existed the only way to switch was the
 * panel's tear-off options menu, unreachable since the wx3.2 AUI port
 * dropped the caption buttons -- and, once that was restored, sitting
 * behind a right-click that the plot's own context menu swallows.
 */
void NModePanel::OnDataViewSelected( wxCommandEvent& event )
{
   if (!p_isValid) {
      return;
   }
   if (event.GetSelection() == 0) {
      showGraph();
   } else {
      showTable();
   }
   //  Remembered the same way the tear-off menu remembered it.
   ewxConfig *config = ewxConfig::getConfig(INIFILE);
   config->Write("NMode/View", (int)(event.GetSelection() == 0 ? GRAPH : TABLE));
}


void NModePanel::showTable()
{
   p_gridPlotSizer->Show((size_t)0,true);
   p_gridPlotSizer->Show((size_t)1,false);
   p_gridPlotSizer->Layout();
}

void NModePanel::showGraph()
{
   p_gridPlotSizer->Show((size_t)1,true);
   p_gridPlotSizer->Show((size_t)0,false);
   p_gridPlotSizer->Layout();
}

bool NModePanel::isGraphShown()
{
   return p_gridPlotSizer->GetItem(1)->IsShown();
}

void NModePanel::OnTimer(wxTimerEvent& evt)
{
   if (checkStructure()) nextStep();
   if (p_animating) p_timer->StartOnce(animationDelay());
}


bool NModePanel::modesApply()
{
   SGFragment *frag = getFW().getSceneGraph().getFragment();
   PropVecTable *vib =
         dynamic_cast<PropVecTable*>(getCalculation()->getProperty("VIB"));
   if (!frag || !vib) return false;
   const size_t n = frag->numAtoms();
   if ((int)n != vib->rows() || n != p_calcElements.size()) return false;
   for (size_t i = 0; i < n; i++)
      if (frag->atomRef(i)->atomicNumber() != p_calcElements[i]) return false;
   return true;
}


/**
 * Enables the panel while the viewer holds the calculation's structure and
 * disables it, with a note, once that has been edited: the modes have one
 * vector per atom of the calculated structure and mean nothing for another.
 */
bool NModePanel::checkStructure()
{
   const bool ok = modesApply();
   if (ok == p_structureOk) return ok;
   p_structureOk = ok;
   for (wxWindow *child : GetChildren())
      if (child != p_staleNote) child->Enable(ok);
   p_staleNote->Show(!ok);
   if (!ok) {
      stop();
      SGContainer& sg = getFW().getSceneGraph();
      sg.getNMRoot()->whichChild.setValue(SO_SWITCH_NONE);
      sg.getNMVecRoot()->removeAllChildren();
      sg.getNMVecRoot()->whichChild.setValue(SO_SWITCH_NONE);
      sg.getcsSwitch()->whichChild.setValue(SO_SWITCH_ALL);
      sg.updateNMVecStarts();
   }
   Layout();
   return ok;
}

/**
 * The plot: the spectrum canvas with its controls underneath.  Item 1 of
 * p_gridPlotSizer, which showGraph()/showTable() rely on.
 */
void NModePanel::createSpectrumPane()
{
   ewxConfig *config = ewxConfig::getConfig(INIFILE);
   double fwhm = 15.0, scale = 1.0;
   bool reversed = true, sticks = true;
   config->Read("NMode/FWHM", &fwhm, 15.0);
   config->Read("NMode/Scale", &scale, 1.0);
   config->Read("NMode/WavenumberHighOnLeft", &reversed, true);
   config->Read("NMode/ShowSticks", &sticks, true);
   if (fwhm < 0 || fwhm > 200) fwhm = 15.0;
   if (scale <= 0) scale = 1.0;

   wxPanel *pane = new wxPanel(this);
   wxBoxSizer *col = new wxBoxSizer(wxVERTICAL);

   p_spectrum = new SpectrumCanvas(pane);
   p_spectrum->setClickHandler(this);
   p_spectrum->setFwhm(fwhm);
   p_spectrum->setScale(scale);
   p_spectrum->setReversed(reversed);
   p_spectrum->setShowSticks(sticks);
   col->Add(p_spectrum, 1, wxEXPAND);

   wxWrapSizer *row = new wxWrapSizer(wxHORIZONTAL);
   const int gap = 6;

   row->Add(new wxStaticText(pane, wxID_ANY,
              wxString::FromUTF8("Width:")),
            0, wxALIGN_CENTER_VERTICAL | wxLEFT, gap);
   p_fwhmText = new wxTextCtrl(pane, wxID_ANY,
                  wxString::Format("%g", fwhm), wxDefaultPosition,
                  wxSize(48, -1), wxTE_PROCESS_ENTER);
   p_fwhmText->SetToolTip(wxString::FromUTF8(
      "Full width at half maximum of each band, in cm\xE2\x81\xBB\xC2\xB9 "
      "(0 to 200).\nCondensed-phase bands are typically 5 to 30 wide.\n"
      "0 shows sticks only.\nA narrow width gives sharp lines at the band "
      "origins; it does not reproduce the rotational structure of a "
      "gas-phase spectrum."));
   row->Add(p_fwhmText, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 3);

   row->Add(new wxStaticText(pane, wxID_ANY, "Freq. scale:"),
            0, wxALIGN_CENTER_VERTICAL | wxLEFT, gap * 2);
   p_scaleText = new wxTextCtrl(pane, wxID_ANY,
                  wxString::Format("%g", scale), wxDefaultPosition,
                  wxSize(48, -1), wxTE_PROCESS_ENTER);
   p_scaleText->SetToolTip(wxString::FromUTF8(
      "Multiplies every frequency. Harmonic frequencies from most "
      "methods are 3 to 5 % too high; a typical factor is 0.96."));
   row->Add(p_scaleText, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 3);

   wxCheckBox *left = new wxCheckBox(pane, wxID_ANY,
                                     "High on left");
   left->SetValue(reversed);
   row->Add(left, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, gap * 2);

   wxCheckBox *stk = new wxCheckBox(pane, wxID_ANY, "Sticks");
   stk->SetValue(sticks);
   row->Add(stk, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, gap * 2);

   wxButton *reset = new wxButton(pane, wxID_ANY, "Reset");
   reset->SetToolTip("Show the whole spectrum (or double-click the plot, "
                     "or press Home).\nClick a peak to show its mode. Drag "
                     "to zoom to a band, mouse wheel to zoom, right-drag "
                     "to pan.");
   row->Add(reset, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, gap * 2);
   wxButton *popout = new wxButton(pane, wxID_ANY, "Open in window");
   popout->SetToolTip("The same spectrum in its own resizable window, with "
                      "both panes and both axes.");
   row->Add(popout, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, gap);
   col->Add(row, 0, wxEXPAND | wxTOP | wxBOTTOM, 3);

   //  The dock can give this panel less height than a plot is readable
   //  in; then this stands in for the whole plot rather than a squashed one.
   wxBoxSizer *noRoom = new wxBoxSizer(wxHORIZONTAL);
   noRoom->AddStretchSpacer(1);
   noRoom->Add(new wxStaticText(pane, wxID_ANY,
                                "Not enough room for the spectrum"),
               0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
   wxButton *noRoomButton = new wxButton(pane, wxID_ANY, "Open in window");
   noRoom->Add(noRoomButton, 0, wxALIGN_CENTER_VERTICAL);
   noRoom->AddStretchSpacer(1);
   col->Add(noRoom, 1, wxEXPAND);
   col->Show(noRoom, false);
   noRoomButton->Bind(wxEVT_BUTTON,
                      [this](wxCommandEvent&) { openSpectrumWindow(); });
   pane->Bind(wxEVT_SIZE, [pane, col, row, noRoom, this](wxSizeEvent& e) {
      const bool tight = e.GetSize().y < 190;
      if (col->IsShown(noRoom) != tight) {
         col->Show(p_spectrum, !tight);
         col->Show(row, !tight);
         col->Show(noRoom, tight);
      }
      e.Skip();
   });

   pane->SetSizer(col);
   p_gridPlotSizer->Add(pane, 1, wxGROW);

   p_fwhmText->Bind(wxEVT_TEXT_ENTER,
                    [this](wxCommandEvent&) { applySpectrumSettings(); });
   p_fwhmText->Bind(wxEVT_KILL_FOCUS, [this](wxFocusEvent& e) {
      applySpectrumSettings(); e.Skip(); });
   p_scaleText->Bind(wxEVT_TEXT_ENTER,
                     [this](wxCommandEvent&) { applySpectrumSettings(); });
   p_scaleText->Bind(wxEVT_KILL_FOCUS, [this](wxFocusEvent& e) {
      applySpectrumSettings(); e.Skip(); });
   left->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent& e) {
      p_spectrum->setReversed(e.IsChecked());
      ewxConfig::getConfig(INIFILE)->Write("NMode/WavenumberHighOnLeft",
                                           e.IsChecked());
   });
   stk->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent& e) {
      p_spectrum->setShowSticks(e.IsChecked());
      ewxConfig::getConfig(INIFILE)->Write("NMode/ShowSticks", e.IsChecked());
   });
   popout->Bind(wxEVT_BUTTON,
                [this](wxCommandEvent&) { openSpectrumWindow(); });
   reset->Bind(wxEVT_BUTTON,
               [this](wxCommandEvent&) { p_spectrum->resetZoom(); });
}


/**
 * The spectrum in a frame of its own: a second canvas on the panel's
 * model, zoom and selection, always in the full two-pane layout.  A
 * child of the Builder's frame, so it goes when the Builder does.
 */
void NModePanel::openSpectrumWindow()
{
   if (p_spectrumFrame) {
      p_spectrumFrame->Raise();
      return;
   }
   wxFrame *frame = new wxFrame(wxGetTopLevelParent(this), wxID_ANY,
                                "Vibrational Frequencies: spectrum",
                                wxDefaultPosition, wxSize(1000, 700));
   SpectrumCanvas *canvas = new SpectrumCanvas(frame);
   canvas->shareWith(*p_spectrum);
   canvas->setClickHandler(this);
   canvas->setCompactBelow(0);
   wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
   sizer->Add(canvas, 1, wxEXPAND);
   frame->SetSizer(sizer);
   p_spectrumFrame = frame;
   p_spectrumPop = canvas;
   frame->Show(true);
}


/**
 * Width and scale from their entry fields, kept within what means
 * something: a width of 0 to 200 cm-1 (0 is sticks only), a positive
 * scale.  A field left unreadable goes back to what is in force.
 */
void NModePanel::applySpectrumSettings()
{
   if (p_spectrum == 0 || p_fwhmText == 0 || p_scaleText == 0) return;
   double fwhm = p_spectrum->spectrum().fwhm();
   double scale = p_spectrum->spectrum().scale();
   double v;
   if (p_fwhmText->GetValue().ToDouble(&v) && v >= 0) {
      fwhm = v > 200 ? 200 : v;
      if (fwhm > 0 && fwhm < 0.1) fwhm = 0.1;
   }
   if (p_scaleText->GetValue().ToDouble(&v) && v > 0) scale = v;
   p_fwhmText->ChangeValue(wxString::Format("%g", fwhm));
   p_scaleText->ChangeValue(wxString::Format("%g", scale));

   const bool scaleChanged = scale != p_spectrum->spectrum().scale();
   if (fwhm != p_spectrum->spectrum().fwhm()) p_spectrum->setFwhm(fwhm);
   if (scaleChanged) p_spectrum->setScale(scale);

   ewxConfig *config = ewxConfig::getConfig(INIFILE);
   config->Write("NMode/FWHM", fwhm);
   config->Write("NMode/Scale", scale);
}


void NModePanel::modeClicked(int mode)
{
   if (mode < 0 || mode >= p_grid->GetNumberRows()) return;
   selectMode(mode);
}

void NModePanel::showAnimationMode()
{
   p_styleSizer->Show(p_animateSizer,1);
   p_styleSizer->Show(p_vectorSizer,0);
   p_styleSizer->Layout();

   SGContainer& sg = getFW().getSceneGraph();
   sg.getNMRoot()->whichChild.setValue(SO_SWITCH_NONE);
   sg.getNMVecRoot()->whichChild.setValue(SO_SWITCH_NONE);
   sg.getcsSwitch()->whichChild.setValue(SO_SWITCH_ALL);
}

void NModePanel::showVectorMode()
{
   p_styleSizer->Show(p_animateSizer,0);
   p_styleSizer->Show(p_vectorSizer,1);
   p_styleSizer->Layout();

   stop();

   // Have to update because animations overwrite the cs.
   updateVectors();

   SGContainer& sg = getFW().getSceneGraph();
   sg.getNMRoot()->whichChild.setValue(SO_SWITCH_NONE);
   sg.getNMVecRoot()->whichChild.setValue(SO_SWITCH_ALL);
   sg.getcsSwitch()->whichChild.setValue(SO_SWITCH_ALL);
}



int NModePanel::getSelectedMode() const
{
   /* Couldn't get these methods to work so keep a variable 
   wxArrayInt selections = p_grid->GetSelectedRows();
   cout << "selected rows " << selections.size() << endl;
   int sel = 0;
   if (selections.size() > 0) 
      sel = selections[0];
   */

   wxString str =  p_grid->GetCellValue(p_selectedRow,0);
   return atoi(str.c_str());
}



bool NModePanel::fillGraph()
{
  IPropCalculation *expt = getCalculation();
  if (expt == 0) {
    wxFAIL_MSG( wxT("No calculation pointer available.") );
    return false;
  }

  PropVector *vec =  (PropVector*) expt->getProperty("VIBFREQ");
  PropVector *ivec = (PropVector*) expt->getProperty("VIBIR");
  PropVector *rvec = (PropVector*) expt->getProperty("VIBRAM");
  PropVecString *symvec = (PropVecString*) expt->getProperty("VIBSYM");

  if (vec == 0) {
    wxFAIL_MSG( wxT("No frequency data available.") );
    return false;
  }

  //  A property the code did not produce gets no pane, rather than a
  //  flat set of unit spikes standing in for it (see fillTable()).
  vector<double> freq, ir, raman;
  vector<string> irrep;
  const int n = vec->rows();
  for (int i = 0; i < n; i++) {
    freq.push_back(vec->value(i));
    if (ivec != 0 && i < ivec->rows()) ir.push_back(ivec->value(i));
    if (rvec != 0 && i < rvec->rows()) raman.push_back(rvec->value(i));
    irrep.push_back(symvec != 0 && i < symvec->rows() ? symvec->value(i)
                                                      : string());
  }

  VibSpectrum spectrum;
  spectrum.setModes(freq, irrep);
  if (ivec != 0) spectrum.setIntensities(VIB_IR, ir, ivec->units());
  if (rvec != 0) spectrum.setIntensities(VIB_RAMAN, raman, rvec->units());
  p_spectrum->setSpectrum(spectrum);

  return ivec != 0 || rvec != 0;
}


/**
 * ECCE_SPECTRUM_DUMP=<path> (#214): write what the canvas holds and where
 * it drew each stick, paint the panel's own canvas to <path>.png, and
 * click the stick of ECCE_SPECTRUM_CLICK=<mode, 1-based> through the
 * canvas's hit test.  For headless checks against the numbers in the
 * code's own output; ECCE_EXIT_AFTER_DUMP=1 then closes the window.
 */
void NModePanel::dumpSpectrumIfRequested()
{
   const char *path = getenv("ECCE_SPECTRUM_DUMP");
   if (path == 0 || p_spectrum == 0) return;

   const wxSize size(1000, 700);
   wxBitmap bitmap(size.x, size.y, 24);
   wxMemoryDC dc(bitmap);
   p_spectrum->paintOnto(dc, size);

   FILE *out = fopen(path, "w");
   if (out != 0) {
      const VibSpectrum& s = p_spectrum->spectrum();
      fprintf(out, "modes %d fwhm %g scale %g\n", s.modeCount(), s.fwhm(),
              s.scale());
      for (int k = 0; k < 2; k++) {
         if (!s.has((VibKind)k)) continue;
         fprintf(out, "axis %s %s\n", k == VIB_IR ? "ir" : "raman",
                 s.axisLabel((VibKind)k).c_str());
         const vector<VibStick> st = s.sticks((VibKind)k);
         for (size_t i = 0; i < st.size(); i++) {
            wxPoint at(-1, -1);
            p_spectrum->stickPosition((VibKind)k, st[i].mode, &at);
            fprintf(out, "stick %s %d %.8f %.10g %s %d %d%s\n",
                    k == VIB_IR ? "ir" : "raman", st[i].mode + 1,
                    st[i].wavenumber, st[i].intensity,
                    st[i].irrep.empty() ? "-" : st[i].irrep.c_str(), at.x,
                    at.y, st[i].imaginary ? " imaginary" : "");
         }
      }
      //  ECCE_SPECTRUM_WINDOW=1: open the pop-out and click in IT.
      SpectrumCanvas *clicker = p_spectrum;
      if (getenv("ECCE_SPECTRUM_WINDOW") != 0) {
         openSpectrumWindow();
         clicker = p_spectrumPop;
         wxBitmap shot(1000, 700, 24);
         wxMemoryDC wdc(shot);
         clicker->paintOnto(wdc, wxSize(1000, 700));
         wdc.SelectObject(wxNullBitmap);
      }
      const char *click = getenv("ECCE_SPECTRUM_CLICK");
      if (click != 0) {
         const int mode = atoi(click) - 1;
         wxPoint at;
         const VibKind kind = s.has(VIB_IR) ? VIB_IR : VIB_RAMAN;
         if (clicker->stickPosition(kind, mode, &at)) {
            clicker->clickAt(wxPoint(at.x, at.y + 3));
            fprintf(out, "click mode %d: panel mode %d, table row %d, "
                         "canvas selection %d\n", mode + 1, p_mode + 1,
                    p_selectedRow + 1, p_spectrum->selected() + 1);
         } else {
            fprintf(out, "click mode %d: no stick\n", mode + 1);
         }
      }
      fclose(out);
   }
   dc.SelectObject(wxNullBitmap);
   bitmap.SaveFile(wxString(path) + ".png", wxBITMAP_TYPE_PNG);

   if (getenv("ECCE_EXIT_AFTER_DUMP") != 0) {
      //  Optionally later, so a screenshot of the window can be taken first.
      const char *delay = getenv("ECCE_EXIT_AFTER_DUMP_DELAY_MS");
      const int ms = delay != 0 ? atoi(delay) : 0;
      if (ms > 0) {
         wxTimer *timer = new wxTimer();   // the process ends with it
         timer->Bind(wxEVT_TIMER, [](wxTimerEvent&) {
            wxWindow *top = wxTheApp->GetTopWindow();
            if (top != 0) top->Close(true);
         });
         timer->StartOnce(ms);
      } else {
         wxTheApp->CallAfter([]() {
            wxWindow *top = wxTheApp->GetTopWindow();
            if (top != 0) top->Close(true);
         });
      }
   }
}


/**
 * Fill the table and initialize the UI.
 * This code copied from motool_cdlg but the graph code is currently excluded.
 */
void NModePanel::fillTable()
{

   IPropCalculation *expt = getCalculation();

   // Clear table
   p_grid->ClearGrid();
   p_grid->DeleteCols(0,p_grid->GetNumberCols());
   p_grid->DeleteRows(0,p_grid->GetNumberRows());

   // Get the required properties 
   PropVector *vec =  (PropVector*) expt->getProperty("VIBFREQ");
   PropVector *ivec = (PropVector*) expt->getProperty("VIBIR");
   PropVector *rvec = (PropVector*) expt->getProperty("VIBRAM");

   if (vec != (PropVector*)0) {
      wxListItem itemCol;
      itemCol.SetText(_T("Frequency"));

      //label = "Frequency\n" + vec->units();
      p_grid->InsertCols(0,1);
      p_grid->SetColLabelValue(0,"Frequency\n" + vec->units());

      p_grid->InsertCols(1,1);
      p_grid->SetColLabelValue(1,"Sym");

      //  Only show a column for a property the code actually produced.
      //  MOPAC computes no Raman activities at all, so a permanently
      //  empty "Raman" column there is just a question the user cannot
      //  answer -- "is this blank because the job failed, or because
      //  the code cannot do it?". Reported live 2026-09-22.
      //
      //  Kept general rather than special-casing MOPAC: the panel
      //  already knows, because getProperty() returns null for a
      //  property that was never extracted. Any code missing either one
      //  now simply has no column for it.
      //
      //  The indices are tracked rather than hardcoded, because
      //  skipping Infrared while keeping Raman would otherwise leave
      //  the Raman values written to a column that does not exist.
      p_irColumn = -1;
      p_ramanColumn = -1;
      int nextCol = 2;
      if (ivec != (PropVector*)0) {
         p_grid->InsertCols(nextCol,1);
         p_grid->SetColLabelValue(nextCol,"Infrared\n"+ivec->units());
         p_irColumn = nextCol++;
      }
      if (rvec != (PropVector*)0) {
         p_grid->InsertCols(nextCol,1);
         p_grid->SetColLabelValue(nextCol,"Raman\n"+rvec->units());
         p_ramanColumn = nextCol++;
      }


      int nRows = vec->rows();

      //  The "Sym" column used to show VIBFREQ's ROW LABELS, which most
      //  parsers emit as plain mode numbers -- so it displayed 1, 2,
      //  3... instead of Mulliken symbols. Reported for MOPAC, but ORCA
      //  emits numeric row labels too, so it was never code-specific.
      //
      //  Meanwhile VIBSYM, which mopac.desc and orca.desc both extract,
      //  was not read by this panel at all: a correctly parsed property
      //  with no consumer. Read it here, and keep the row labels as the
      //  fallback for a code that carries its symmetries there instead.
      PropVecString *symvec = (PropVecString*) expt->getProperty("VIBSYM");
      const vector<string> *syms = vec->rowLabels();
      INTERNALEXCEPTION(syms,"No symmetry labels - gotta have 'em");

      p_grid->InsertRows(0,nRows);

      for (int idx=0; idx<nRows; idx++) {
         p_grid->SetCellValue(idx,0,
               wxString::Format (PrefLabels::DOUBLEFORMAT, vec->value(idx)));
         if (symvec != (PropVecString*)0 && idx < symvec->rows()) {
            p_grid->SetCellValue(idx,1,symvec->value(idx).c_str());
         } else if (syms && syms->size()>0) {
            p_grid->SetCellValue(idx,1,(*syms)[idx].c_str());
         }

         if (ivec != (PropVector*)0 && p_irColumn >= 0) {
            p_grid->SetCellValue(idx,p_irColumn,
                  wxString::Format (PrefLabels::DOUBLEFORMAT, ivec->value(idx)));
         }

         if (rvec != (PropVector*)0 && p_ramanColumn >= 0) {
            //  Its OWN column. This used to write into column 2 -- the
            //  Infrared column -- so for any code emitting both VIBIR
            //  and VIBRAM the Raman value overwrote the infrared one and
            //  the Raman column was NEVER populated, for every code.
            p_grid->SetCellValue(idx,p_ramanColumn,
                  wxString::Format (PrefLabels::DOUBLEFORMAT, rvec->value(idx)));
         }
      }
   }
   p_grid->AutoSize();
}



/**
 * Show the visualization for the specified mode.
 */
void NModePanel::showMode(int index)
{
   // OnModeSelection() already guards against the premature
   // wxEVT_GRID_SELECT_CELL that wxGrid::SetTable() fires synchronously
   // during CreateControls() (see the comment there). This second check
   // is defense-in-depth for any other path into showMode() -- e.g.
   // selectMode() re-selecting a grid row -- that could reach here before
   // Create() has finished building sibling controls this function
   // depends on (p_slider, ...).
   if (!p_isValid) {
      return;
   }
   if (!checkStructure()) return;

   p_currentStep = 0;
   p_mode = index;
   if (p_spectrum != 0) p_spectrum->setSelected(index);

   // Restore to proper fragment so commands use correct coordinates
   selectFragStep(-1);

   WxVizToolFW& fw = getFW();
   IPropCalculation *expt = getCalculation();
#ifndef INSTALL
   ICalculation *escalc = dynamic_cast<ICalculation*>(expt);
   INTERNALEXCEPTION(escalc, "Cannot down cast to ICalucation");
#endif
   SGContainer& sg = fw.getSceneGraph();

   wxButton *btn = (wxButton*)FindWindow(ID_BUTTON_NMODE_VECCOLOR);
   wxString color = btn->GetBackgroundColour().GetAsString(wxC2S_HTML_SYNTAX);

   wxCheckBox *tgl = (wxCheckBox*)FindWindow(ID_CHECKBOX_NMODE_VECSIGN);
   bool sign = tgl->IsChecked();

   // Note: we want to get the suggested slider value back from the command
   // and set it in the ui.
   Command *cmd = new NModeVectCmd("Normal Mode Vectors", &sg, expt);
   //cmd->getParameter("Amplitude")->setDouble(-1.); 
   cmd->getParameter("Mode")->setInteger(p_mode);
   cmd->getParameter("Color")->setString(color.ToStdString());
   cmd->getParameter("Sign")->setBoolean(sign);
   fw.execute(cmd);
   p_vecAmplitude = cmd->getParameter("Amplitude")->getDouble();


   Command *cmd2 = new NModeTraceCmd("Normal Mode Animation", &sg, expt);
   // No UI to support this right now...
   //cmd->getParameter("NumAnimations")->setInteger, num);
   //cmd->getParameter("Amplitude")->setInteger, num);
   cmd2->getParameter("Mode")->setInteger(p_mode);
   fw.execute(cmd2);
   p_aniAmplitude = cmd2->getParameter("Amplitude")->getDouble();

   setSlider();

   processStep(p_currentStep);

   /*
   cmdMgr()->setParam("CmdAddNormalMode","Mode",num);
   num = p_numAnimations;
   cmdMgr()->setParam("CmdAddNormalMode", "NumAnimations", num);
   viewer()->getSel()->deselectAll();
   cmdMgr()->execute("CmdAddNormalMode",cvsg());

   */
   // TODO....
   //sg.getNMVecRoot()->whichChild.setValue(SO_SWITCH_ALL);


}

void NModePanel::setSlider()
{
   wxRadioBox *radbox = (wxRadioBox*)FindWindow(ID_RADIOBOX_NMODE_VIZTYPE);
   if (radbox->GetSelection() == 0) {
      p_slider->SetValue(p_aniAmplitude);
   } else {
      p_slider->SetValue(p_vecAmplitude);
   }
}

void NModePanel::processStep(int step)
{
   if (!checkStructure()) return;
   p_currentStep = step;

   WxVizToolFW& fw = getFW();
   IPropCalculation *expt = getCalculation();
#ifndef INSTALL
   ICalculation *escalc = dynamic_cast<ICalculation*>(expt);
   INTERNALEXCEPTION(escalc, "Cannot down cast to ICalucation");
#endif
   SGContainer& sg = fw.getSceneGraph();

   Command *cmd = new NModeStepCmd("Normal Mode Step", &sg, expt);
   cmd->getParameter("Index")->setInteger(step);

   fw.execute(cmd);

   // Issue #99, same fault as the geometry trace: Open Inventor's redraw
   // sensor is a one-shot that re-arms on render, and with a render
   // callback installed nothing re-arms it -- so after the first step the
   // scene manager's render callback is never entered again and the view
   // freezes while the data keeps moving. Proved live on the geometry
   // trace by instrumenting SoWxRenderArea::renderCB; see
   // GeomTracePropertyPanel::processStep() for the captured log.
   //
   // This path has the identical shape (step command mutates atom
   // coordinates, then relies on notification), and vibration animation
   // was never verified end to end because the display-mode switch that
   // reveals the Play button was itself broken until recently. Fixing it
   // here too rather than waiting to reproduce the same thing twice.
   fw.getViewer().refreshRenderArea();
}

void NModePanel::nextStep()
{
   int step = p_currentStep + 1;
   if (step >= p_numAnimations) {
      step = 0;
   }
   processStep(step);

}

void NModePanel::previousStep()
{
   int step = p_currentStep - 1;
   if (step < 0) {
      step = p_numAnimations-1;
   }
   processStep(step);

}

void NModePanel::start()
{
   if (!checkStructure()) return;
   if (!p_animating) {
      p_animating = true;
      p_timer->StartOnce(animationDelay());
   }
}

void NModePanel::stop()
{
   p_animating = false;
   p_timer->Stop();
}

// One-shot, re-armed after each step: a repeating timer shorter than a
// step (20 ms against a step plus render) is always due, and GTK never
// gets to its lower-priority redraw, so the spectrum's selection and every
// other widget stopped repainting while the animation ran.
int NModePanel::animationDelay()
{
   ewxTextCtrl *text = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_NMODE_DELAY);
   int ms = text ? text->getValueAsInt() : 20;
   return ms > 0 ? ms : 1;
}


/**
 * Selects the specified mode.
 * The visualization is also updated (showMode)
 */
void NModePanel::selectMode(int index)
{
   p_selectedRow = index;
   p_grid->SelectRow(index);

   // This doesn't seem to work
   p_grid->MakeCellVisible(index,0);

   showMode(index);
}


/**
 * This is the callback for user selection from the MO menu.
 */
void NModePanel::OnMenuClick( wxCommandEvent& event )
{
   if (event.GetId() == GRAPH) {
       showGraph();
   } else if (event.GetId() == TABLE) {
       showTable();
   }
   ewxConfig *config = ewxConfig::getConfig(INIFILE);
   config->Write("NMode/View", event.GetId());

   event.Skip();
}


wxWindow* NModePanel::GetTearableContent()
{
  wxToolBar *tb = new wxToolBar(this, wxID_ANY,
          wxDefaultPosition, wxDefaultSize,
          wxTB_VERTICAL|wxTB_TEXT|wxTB_NOICONS,
          "NModePanel Menu");
     
  if (isGraphShown()) {
     tb->AddTool(TABLE,"Show Table",wxNullBitmap,"Switch to Table View");
  } else {
     tb->AddTool(GRAPH,"Show Graph",wxNullBitmap,"Switch to Graph View");
  }

  return tb;
}


void NModePanel::OnTextctrlNmodeDelayEnter( wxCommandEvent& event )
{
   ewxTextCtrl *text = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_NMODE_DELAY);
   ewxConfig *config = ewxConfig::getConfig(INIFILE);
   config->Write("NMode/Delay",text->getValueAsInt());

   if (p_animating) {
      stop();
      start();
   }
}




void NModePanel::OnEndSliderMotion(wxScrollEvent& event)
{
   if (!checkStructure()) {
      event.Skip();
      return;
   }
   double value = static_cast<double>(p_slider->GetFloatValue());

   WxVizToolFW& fw = getFW();
   IPropCalculation *expt = getCalculation();
#ifndef INSTALL
   ICalculation *escalc = dynamic_cast<ICalculation*>(expt);
   INTERNALEXCEPTION(escalc, "Cannot down cast to ICalucation");
#endif
   SGContainer& sg = fw.getSceneGraph();

//TODO toggle the scene graph switches
   wxRadioBox *radbox = (wxRadioBox*)FindWindow(ID_RADIOBOX_NMODE_VIZTYPE);
   if (radbox->GetSelection() == 0) {
      ; // animation
      bool restart = p_animating;
      stop();
      Command *cmd = new NModeTraceCmd("Normal Mode Animation", &sg, expt);
      // No UI to support this right now...
      //cmd->getParameter("NumAnimations")->setInteger, num);
      cmd->getParameter("Mode")->setInteger(p_mode);
      cmd->getParameter("Amplitude")->setDouble(value);
      fw.execute(cmd);
      if (restart) {
         start();
      }
   } else {


      updateVectors();
   }
   event.Skip();
}


void NModePanel::OnSliderTextEnter(wxCommandEvent& event)
{
  wxScrollEvent dummy;
  OnEndSliderMotion(dummy);
  event.Skip();
}




void NModePanel::receiveFocus()
{
   if (!checkStructure()) return;
   wxRadioBox *radbox = (wxRadioBox*)FindWindow(ID_RADIOBOX_NMODE_VIZTYPE);
   p_lastRadioSel = radbox->GetSelection();
   if (p_lastRadioSel == 0) {
      showAnimationMode();
   } else {
      showVectorMode();
   }
   setSlider();
}

void NModePanel::loseFocus()
{
   SGContainer& sg = getFW().getSceneGraph();
   sg.getNMVecRoot()->whichChild.setValue(SO_SWITCH_NONE);
   sg.getNMRoot()->whichChild.setValue(SO_SWITCH_NONE);
   sg.getcsSwitch()->whichChild.setValue(SO_SWITCH_ALL);
   stop();
   // Restore after the animation, never over an edited structure.
   if (checkStructure()) selectFragStep(-1);
}

void NModePanel::OnBitmapbuttonStartClick( wxCommandEvent& event )
{
   start();
   event.Skip();
}


void NModePanel::OnBitmapbuttonStopClick( wxCommandEvent& event )
{
   stop();
   event.Skip();
}


void NModePanel::OnButtonNmodeVeccolorClick( wxCommandEvent& event )
{
   wxButton *btn = (wxButton*)FindWindow(ID_BUTTON_NMODE_VECCOLOR);
   wxColour color = btn->GetBackgroundColour();

   wxPoint pos = wxGetMousePosition();
   ewxColorDialog dlg(color, this, wxID_ANY, "ECCE Vector Color", pos);
   if (dlg.ShowModal() == wxID_OK) {
      wxString bgcolor = dlg.GetColor().GetAsString(wxC2S_HTML_SYNTAX);

      wxCheckBox *tgl = (wxCheckBox*)FindWindow(ID_CHECKBOX_NMODE_VECSIGN);
      bool sign = tgl->IsChecked();

      updateVectors(p_mode, p_slider->GetFloatValue(), sign, bgcolor.ToStdString());

      btn->SetBackgroundColour(bgcolor);

      ewxConfig *config = ewxConfig::getConfig(INIFILE);
      config->Write("NMode/Color", bgcolor);

   }


   event.Skip();
}


void NModePanel::OnCheckboxNmodeVecsignClick( wxCommandEvent& event )
{
   /*
   wxCheckBox *tgl = (wxCheckBox*)FindWindow(ID_CHECKBOX_NMODE_VECSIGN);
   bool sign = tgl->IsChecked();

   wxButton *btn = (wxButton*)FindWindow(ID_BUTTON_NMODE_VECCOLOR);
   wxString color = btn->GetBackgroundColour().GetAsString(wxC2S_HTML_SYNTAX);

   updateVectors(p_mode, p_slider->GetFloatValue(), sign, color.c_str());
   */
   updateVectors();

   event.Skip();
}

void NModePanel::OnRadioboxSelected( wxCommandEvent& event )
{
   wxRadioBox *radbox = (wxRadioBox*)FindWindow(ID_RADIOBOX_NMODE_VIZTYPE);
   p_lastRadioSel = radbox->GetSelection();
   setSlider();
   if (p_lastRadioSel == 0) {
      showAnimationMode();
   } else {
      showVectorMode();
   }
   event.Skip();
}

/**
 * Confirmed live (strace on an instrumented build, synthetic click
 * synced with the trace window): clicking the Animation/Vector radio
 * box toggles the native GTK widget's own selected bullet, but
 * OnRadioboxSelected() below is never entered -- zero writes from any
 * of its instrumented call sites, for either the real click or a
 * synced synthetic one. wxEVT_COMMAND_RADIOBOX_SELECTED is not
 * reaching this object under wx3.2/GTK3 for this control.
 *
 * This handler was added as a suspected fix: EVT_UPDATE_UI is a
 * wx-level idle-time poll, independent of whatever GTK signal wiring
 * is failing for the click event, so in theory it should still detect
 * radbox->GetSelection() changing even when the native event doesn't
 * arrive. Built, packaged, and live-tested (2026-09-17) -- no visible
 * difference; the Animation/Vector display still doesn't switch. That
 * means either this EVT_UPDATE_UI handler also isn't firing for this
 * control, or the display-switch code path itself (showAnimationMode/
 * showVectorMode) isn't the actual mechanism behind what's visible in
 * the panel, or the installed package still wasn't picking up this
 * binary for some reason not yet ruled out. Not root-caused. Next
 * session: don't assume this fix works, and don't re-spend time
 * re-deriving the "native event never arrives" fact above -- it's
 * solid (syscall-level trace, synced click) -- but treat the
 * EVT_UPDATE_UI theory and everything below it as unverified.
 */
void NModePanel::OnRadioboxUpdateUI( wxUpdateUIEvent& event )
{
   wxRadioBox *radbox = (wxRadioBox*)FindWindow(ID_RADIOBOX_NMODE_VIZTYPE);
   //  An edit while the panel has the viewer: disable it, or show the
   //  modes again once the structure is the calculation's again (undo).
   if (hasFocus() && p_structureOk != modesApply()) {
      if (checkStructure()) receiveFocus();
      return;
   }
   int sel = radbox->GetSelection();
   if (sel != p_lastRadioSel) {
      p_lastRadioSel = sel;
      setSlider();
      if (sel == 0) {
         showAnimationMode();
      } else {
         showVectorMode();
      }
   }
}

void NModePanel::OnModeSelection( wxGridEvent& event )
{
   // wxGrid::CreateGrid() (NModesGUI::CreateControls(), building the
   // placeholder 5x5 grid) fires a real EVT_GRID_SELECT_CELL synchronously
   // as part of construction, well before Create() has gotten back from
   // NModesGUI::Create() to build this panel's own sibling controls (the
   // color button, sign checkbox, ...). showMode() unconditionally
   // dereferences those via FindWindow(), so handling this premature event
   // segfaults on a null FindWindow() result -- confirmed live (crash in
   // wxWindowBase::GetBackgroundColour() called on a null button pointer,
   // via OnModeSelection -> showMode, entered from inside
   // NModesGUI::CreateControls() -> wxGrid::CreateGrid() ->
   // UpdateCurrentCellOnRedim() -> SetCurrentCell() -> SendEvent()).
   // p_isValid (already a member, previously set but never checked) is
   // the guard: Create() flips it true only once every control this
   // handler touches actually exists. The real, wanted mode-0 selection
   // happens moments later via Create()'s own initialize() -> showMode()
   // call, once p_isValid is true, so simply ignoring this early one
   // loses nothing.
   if (!p_isValid) {
      event.Skip();
      return;
   }
   p_mode = p_selectedRow = event.GetRow();
   showMode(p_mode);
   event.Skip();
}


void NModePanel::updateVectors()
{
   if (!checkStructure()) return;
   wxCheckBox *tgl = (wxCheckBox*)FindWindow(ID_CHECKBOX_NMODE_VECSIGN);
   bool sign = tgl->IsChecked();

   wxButton *btn = (wxButton*)FindWindow(ID_BUTTON_NMODE_VECCOLOR);
   wxString color = btn->GetBackgroundColour().GetAsString(wxC2S_HTML_SYNTAX);

   double value = static_cast<double>(p_slider->GetFloatValue());

   updateVectors(p_mode, value, sign, color.ToStdString());
}


void NModePanel::updateVectors(int mode, 
                               float amplitude,
                               bool sign,
                               string color)
{
   WxVizToolFW& fw = getFW();
   SGContainer& sg = fw.getSceneGraph();
   IPropCalculation *expt = getCalculation();

   NModeVectCmd *cmd = new NModeVectCmd("Normal Mode Vectors", &sg, expt);
   cmd->getParameter("Mode")->setInteger(mode);
   cmd->getParameter("Amplitude")->setDouble(amplitude);
   cmd->getParameter("Color")->setString(color.c_str());
   cmd->getParameter("Sign")->setBoolean(sign);
   fw.execute(cmd);
}
