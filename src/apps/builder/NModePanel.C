#include "util/UnitDisplay.H"
#include <cmath>
#include <iostream>
  using std::cout;
  using std::cerr;
  using std::endl;
#include <algorithm>
  using std::fill;
#include <limits>

#include <wx/checkbox.h>
#include <wx/control.h>
#include <wx/settings.h>
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
#include "dsm/ChemistryTask.H"
#include "dsm/JCode.H"
#include "util/EcceURL.H"
#include <wx/filedlg.h>
#include "dsm/IPropCalculation.H"
#include "dsm/EDSIFactory.H"
#include "dsm/Resource.H"

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

   //  Two lines under the table for the selected mode: wrapped to the
   //  width the dock gives, a tiny minimum width so it never widens the
   //  panel; the whole text is its tooltip.
   {
      wxSizer *gs = p_grid->GetContainingSizer();
      p_modeDetail = new wxStaticText(this, wxID_ANY, "");
      p_modeDetail->SetMinSize(wxSize(50, 2*p_modeDetail->GetCharHeight() + 2));
      wxBoxSizer *col = new wxBoxSizer(wxVERTICAL);
      gs->Detach(p_grid);
      col->Add(p_grid, 1, wxEXPAND);
      col->Add(p_modeDetail, 0, wxEXPAND | wxTOP, 3);
      gs->Insert(0, col, 1, wxGROW | wxALL, 3);
      //  A window made now is shown whatever its sizer item's state is.
      p_modeDetail->Show(!isGraphShown());
   }
   p_modeDetail->Bind(wxEVT_SIZE, [this](wxSizeEvent& e) {
      setWrapped(p_modeDetail, p_detailParts);
      e.Skip();
   });
   p_graphDetail->Bind(wxEVT_SIZE, [this](wxSizeEvent& e) {
      setWrapped(p_graphDetail, p_graphParts);
      e.Skip();
   });
   p_grid->GetGridWindow()->Bind(wxEVT_MOTION, &NModePanel::OnGridMotion, this);
   p_grid->GetGridColLabelWindow()->Bind(wxEVT_MOTION, &NModePanel::OnLabelMotion, this);
   p_grid->Bind(wxEVT_SIZE, [this](wxSizeEvent& e) { fitModeColumn(); e.Skip(); });

   wxString color = config->Read("NMode/Color", "yellow");
   ewxButton *btn = (ewxButton*)FindWindow(ID_BUTTON_NMODE_VECCOLOR);
   btn->SetBackgroundColour(wxColour(color));

   // Add the slider
   ewxPanel *p = new ewxPanel(this);
   p_sliderSizer->Add(p, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);
   p_slider = new SliderCombo(p, ID_SLIDER);
   p_slider->SetRange((float)0.0, (float)30.0);
   p_slider->SetToolTip("Length of the displacement arrows, or the "
                        "amplitude of the animation");


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

   //  Translations and rotations (~0 cm-1) come first in the output; open
   //  on the first real vibration, not a zero-scale translation.
   for (int r = 0; r < p_grid->GetNumberRows(); r++) {
      double f = 0;
      if (p_grid->GetCellValue(r, 0).ToDouble(&f) && fabs(f) >= 10.0) {
         p_mode = r;
         break;
      }
   }
   selectMode(p_mode);
   p_initialMode = p_mode;

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
   dumpTableIfRequested();
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

   //  The selected mode, frequency first, wrapped into two lines under the
   //  plot (see showModeDetail()); every line here is taken from the plot.
   p_graphDetail = new wxStaticText(pane, wxID_ANY, "");
   p_graphDetail->SetMinSize(wxSize(50, 2*p_graphDetail->GetCharHeight() + 2));
   col->Add(p_graphDetail, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 6);

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

   row->Add(new wxStaticText(pane, wxID_ANY, "Freq. factor:"),
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
   //  The settings row goes first: at the Viewer's first size on a laptop
   //  screen (1366x768) the pane is about 140 px, room for the plot alone.
   pane->Bind(wxEVT_SIZE, [pane, col, row, noRoom, this](wxSizeEvent& e) {
      const int h = e.GetSize().y;
      const bool plot = h >= 110, settings = h >= 190, text = h >= 150;
      if (col->IsShown(p_spectrum) != plot || col->IsShown(row) != settings ||
          col->IsShown(p_graphDetail) != text) {
         col->Show(p_spectrum, plot);
         col->Show(row, settings);
         col->Show(p_graphDetail, text);
         col->Show(noRoom, !plot);
      }
      e.Skip();
   });

   pane->SetSizer(col);
   p_gridPlotSizer->Add(pane, 1, wxGROW);

   //  The window is most useful beside the Table view, where the plot's own
   //  button is hidden: offer it next to the Graph/Table choice as well.
   wxWindow *viewBox = FindWindow(ID_RADIOBOX_NMODE_DATAVIEW);
   wxSizer *viewSizer = viewBox ? viewBox->GetContainingSizer() : 0;
   if (viewSizer) {
      size_t at = 0;
      for (wxSizerItemList::compatibility_iterator n =
             viewSizer->GetChildren().GetFirst(); n; n = n->GetNext(), at++)
         if (n->GetData()->GetWindow() == viewBox) break;
      wxButton *spectrum = new wxButton(viewBox->GetParent(), wxID_ANY,
                                        "Spectrum...");
      spectrum->SetToolTip("The IR and Raman spectra in their own window.");
      viewSizer->Insert(at + 1, spectrum, 0, wxALIGN_CENTER_VERTICAL | wxALL, 3);
      spectrum->Bind(wxEVT_BUTTON,
                     [this](wxCommandEvent&) { openSpectrumWindow(); });
      //  Beside the Graph/Table choice, not in the plot's settings row,
      //  where it wrapped onto a line of its own and took the plot's height.
      wxButton *exportCsv = new wxButton(viewBox->GetParent(), wxID_ANY,
                                         "Export .csv...");
      exportCsv->SetToolTip("Save the spectrum (sticks and broadened "
                            "curve) as a comma-separated file for a "
                            "spreadsheet or plotting program.");
      viewSizer->Insert(at + 2, exportCsv, 0, wxALIGN_CENTER_VERTICAL | wxALL, 3);
      exportCsv->Bind(wxEVT_BUTTON,
                      [this](wxCommandEvent&) { exportSpectrumDialog(); });
      wxButton *compare = new wxButton(viewBox->GetParent(), wxID_ANY,
                                       "Compare...");
      compare->SetToolTip("Draw the IR or Raman spectrum of another "
                          "calculation in this folder as a second curve.");
      viewSizer->Insert(at + 3, compare, 0, wxALIGN_CENTER_VERTICAL | wxALL, 3);
      compare->Bind(wxEVT_BUTTON,
                    [this](wxCommandEvent&) { compareDialog(); });
      //  Enabled, not hidden, so the row does not reflow at narrow widths.
      p_compareRemove = new wxButton(viewBox->GetParent(), wxID_ANY,
                                     "Remove comparison");
      p_compareRemove->SetToolTip("Remove the second curve and show this "
                                  "calculation only.");
      p_compareRemove->Enable(false);
      viewSizer->Insert(at + 4, p_compareRemove, 0,
                        wxALIGN_CENTER_VERTICAL | wxALL, 3);
      p_compareRemove->Bind(wxEVT_BUTTON,
                    [this](wxCommandEvent&) { compareWith(string()); });
   }

   //  The wrap sizers under the plot learn their width only in a layout, so
   //  the first one reserved them several rows (on MSW about 100 px) and the
   //  plot did not fit; a second pass at the same size gives it back.
   Bind(wxEVT_SIZE, [this](wxSizeEvent& e) {
      e.Skip();
      CallAfter([this]() { Layout(); });
   });

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


VibKind NModePanel::spectrumKindShown() const
{
   return p_spectrum ? p_spectrum->compactPane() : VIB_IR;
}

bool NModePanel::exportSpectrumCsv(const wxString& path, VibKind kind)
{
   IPropCalculation *calc = getCalculation();
   if (p_spectrum == 0 || calc == 0) return false;
   const VibSpectrum& spec = p_spectrum->spectrum();
   if (!spec.has(kind)) kind = spec.has(VIB_IR) ? VIB_IR : VIB_RAMAN;

   SpectrumCsvInfo info;
   info.calculation = calc->getURL().getFilePathTail();
   ChemistryTask *task = dynamic_cast<ChemistryTask*>(calc);
   const JCode *code = task ? task->application() : 0;
   info.code = code ? code->name() : string();
   info.compareCode = p_compareCode;

   const string text = vibSpectrumCsv(spec, kind, info,
                                      p_spectrum->axis().fullLo(),
                                      p_spectrum->axis().fullHi(), 1.0);
   FILE *out = fopen(path.utf8_str(), "wb");
   if (out == 0) return false;
   const bool ok = fwrite(text.data(), 1, text.size(), out) == text.size();
   return fclose(out) == 0 && ok;
}

void NModePanel::exportSpectrumDialog()
{
   IPropCalculation *calc = getCalculation();
   if (calc == 0) return;
   wxFileDialog dlg(this, "Export spectrum",  wxEmptyString,
                    wxString::FromUTF8(calc->getURL().getFilePathTail().c_str()) +
                    "-spectrum.csv", "CSV files (*.csv)|*.csv",
                    wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
   if (dlg.ShowModal() != wxID_OK) return;
   if (!exportSpectrumCsv(dlg.GetPath(), spectrumKindShown()))
      wxMessageBox("The file could not be written.", "Export spectrum",
                   wxOK | wxICON_ERROR, this);
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
   p_scaleLabel->SetLabel(_("Amplitude:"));
   p_styleSizer->Layout();

   SGContainer& sg = getFW().getSceneGraph();
   sg.getNMRoot()->whichChild.setValue(SO_SWITCH_NONE);
   sg.getNMVecRoot()->whichChild.setValue(SO_SWITCH_NONE);
   sg.getcsSwitch()->whichChild.setValue(SO_SWITCH_ALL);

   // Choosing Animation plays the selected mode; there is no Start button.
   if (p_active) start();
}

void NModePanel::showVectorMode()
{
   p_styleSizer->Show(p_animateSizer,0);
   p_styleSizer->Show(p_vectorSizer,1);
   p_scaleLabel->SetLabel(_("Arrow length:"));
   p_styleSizer->Layout();

   stop();

   // The animation leaves the atoms displaced; Vector draws the equilibrium.
   if (checkStructure()) selectFragStep(-1);

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



//  The spectrum a calculation holds, for this panel's own calculation and
//  for one it is compared with.  False without frequencies.
static bool spectrumOf(IPropCalculation *expt, VibSpectrum& spectrum,
                       bool *hasIntensities = 0)
{
  PropVector *vec =  (PropVector*) expt->getProperty("VIBFREQ");
  PropVector *ivec = (PropVector*) expt->getProperty("VIBIR");
  PropVector *rvec = (PropVector*) expt->getProperty("VIBRAM");
  PropVecString *symvec = (PropVecString*) expt->getProperty("VIBSYM");
  if (vec == 0) return false;

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

  spectrum.setModes(freq, irrep);
  if (ivec != 0) spectrum.setIntensities(VIB_IR, ir, ivec->units());
  if (rvec != 0) spectrum.setIntensities(VIB_RAMAN, raman, rvec->units());
  if (hasIntensities != 0) *hasIntensities = ivec != 0 || rvec != 0;
  return true;
}


bool NModePanel::fillGraph()
{
  IPropCalculation *expt = getCalculation();
  if (expt == 0) {
    wxFAIL_MSG( wxT("No calculation pointer available.") );
    return false;
  }

  VibSpectrum spectrum;
  bool any = false;
  if (!spectrumOf(expt, spectrum, &any)) {
    wxFAIL_MSG( wxT("No frequency data available.") );
    return false;
  }
  spectrum.setLabel(expt->getURL().getFilePathTail());
  if (p_compare) spectrum.setCompare(*p_compare, p_compareName);
  p_spectrum->setSpectrum(spectrum);

  return any;
}


/**
 * The spectrum of another calculation in the same folder as a second curve
 * (#173).  `name` is the calculation's name; empty removes the second curve.
 * False if no calculation of that name holds IR or Raman data.
 */
bool NModePanel::compareWith(const string& name)
{
  IPropCalculation *calc = getCalculation();
  if (calc == 0 || p_spectrum == 0) return false;
  if (name.empty()) {
    p_compare.reset();
    p_compareName.clear();
    p_compareCode.clear();
    if (p_compareRemove) p_compareRemove->Enable(false);
    fillGraph();
    return true;
  }
  const vector<IPropCalculation*> found = comparableCalculations();
  for (size_t i = 0; i < found.size(); i++) {
    if (found[i]->getURL().getFilePathTail() != name) continue;
    std::shared_ptr<VibSpectrum> other(new VibSpectrum());
    if (!spectrumOf(found[i], *other) ||
        !(other->has(VIB_IR) || other->has(VIB_RAMAN))) return false;
    p_compare = other;
    p_compareName = name;
    ChemistryTask *task = dynamic_cast<ChemistryTask*>(found[i]);
    const JCode *code = task ? task->application() : 0;
    p_compareCode = code ? code->name() : string();
    if (p_compareRemove) p_compareRemove->Enable(true);
    fillGraph();
    return true;
  }
  return false;
}


/** The other calculations in this one's folder that have IR or Raman data. */
vector<IPropCalculation*> NModePanel::comparableCalculations()
{
  vector<IPropCalculation*> out;
  IPropCalculation *calc = getCalculation();
  if (calc == 0) return out;
  Resource *folder = EDSIFactory::getResource(calc->getURL().getParent());
  vector<Resource*> *kids = folder ? folder->getChildren() : 0;
  if (kids == 0) return out;
  const string own = calc->getURL().getFilePathTail();
  for (size_t i = 0; i < kids->size(); i++) {
    IPropCalculation *other = dynamic_cast<IPropCalculation*>((*kids)[i]);
    if (other == 0 || (*kids)[i]->getName() == own) continue;
    if (other->getProperty("VIBFREQ") == 0) continue;
    if (other->getProperty("VIBIR") == 0 && other->getProperty("VIBRAM") == 0)
      continue;
    out.push_back(other);
  }
  return out;
}


void NModePanel::compareDialog()
{
  wxBusyCursor busy;
  const vector<IPropCalculation*> found = comparableCalculations();
  if (found.empty()) {
    wxMessageBox("No other calculation in this folder has IR or Raman data.",
                 "Compare spectra", wxOK | wxICON_INFORMATION, this);
    return;
  }
  wxArrayString names;
  names.Add("(none: this calculation only)");
  for (size_t i = 0; i < found.size(); i++)
    names.Add(wxString::FromUTF8(found[i]->getURL().getFilePathTail().c_str()));
  const int at = wxGetSingleChoiceIndex(
      "Draw the spectrum of this calculation as a second curve, with the "
      "same scaling factor and line width:", "Compare spectra", names, this);
  if (at < 0) return;
  compareWith(at == 0 ? string() : found[at - 1]->getURL().getFilePathTail());
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

   //  ECCE_SPECTRUM_COMPARE=<calculation>: the second curve before painting;
   //  ECCE_SPECTRUM_CSV=<path>: the CSV an export would write.
   const char *other = getenv("ECCE_SPECTRUM_COMPARE");
   const bool compared = other != 0 && compareWith(other);
   const char *csv = getenv("ECCE_SPECTRUM_CSV");
   if (csv != 0) exportSpectrumCsv(csv, spectrumKindShown());

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
      if (other != 0) {
         fprintf(out, "compare %s %s\n", other, compared ? "set" : "FAILED");
         for (int k = 0; k < 2; k++) {
            const vector<VibStick> cs = s.compareSticks((VibKind)k);
            for (size_t i = 0; i < cs.size(); i++)
               fprintf(out, "compare-stick %s %d %.8f %.10g\n",
                       k == VIB_IR ? "ir" : "raman", cs[i].mode + 1,
                       cs[i].wavenumber, cs[i].intensity);
         }
      }
      //  ECCE_SPECTRUM_REMOVE_COMPARE=1: click "Remove comparison" and
      //  report the button and the second curve before and after.
      if (other != 0 && getenv("ECCE_SPECTRUM_REMOVE_COMPARE") != 0 &&
          p_compareRemove != 0) {
         auto second = [&]() {
            return (int)(p_spectrum->spectrum().compareSticks(VIB_IR).size() +
                         p_spectrum->spectrum().compareSticks(VIB_RAMAN).size());
         };
         fprintf(out, "remove-button before enabled %d compare-sticks %d\n",
                 p_compareRemove->IsEnabled() ? 1 : 0, second());
         wxCommandEvent ev(wxEVT_BUTTON, p_compareRemove->GetId());
         ev.SetEventObject(p_compareRemove);
         p_compareRemove->GetEventHandler()->ProcessEvent(ev);
         fprintf(out, "remove-button after enabled %d compare-sticks %d\n",
                 p_compareRemove->IsEnabled() ? 1 : 0, second());
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
 * ECCE_NMODE_TABLE_DUMP=<path> (#367): switch to the table, and once the
 * window has been laid out write what it shows: the panel's and the grid's
 * sizes, each column's width, each cell as displayed and whether its text
 * fits, and ECCE's analysis of every mode.  For headless checks that the
 * docked panel shows the Mode cells without widening.
 */
void NModePanel::dumpTableIfRequested()
{
   const char *path = getenv("ECCE_NMODE_TABLE_DUMP");
   if (path == 0) return;
   showTable();
   const string target(path);
   wxTimer *timer = new wxTimer();   // its own handler; ends with the process
   timer->Bind(wxEVT_TIMER, [this, target](wxTimerEvent&) {
      fitModeColumn();
      FILE *out = fopen(target.c_str(), "w");
      if (out == 0) return;
      wxClientDC dc(p_grid->GetGridWindow());
      dc.SetFont(p_grid->GetDefaultCellFont());
      fprintf(out, "panel %d %d min %d %d\n", GetSize().x, GetSize().y,
              GetEffectiveMinSize().x, GetEffectiveMinSize().y);
      fprintf(out, "grid %d %d min %d %d\n", p_grid->GetClientSize().x,
              p_grid->GetClientSize().y, p_grid->GetEffectiveMinSize().x,
              p_grid->GetEffectiveMinSize().y);
      fprintf(out, "group %s\n", p_vib.group.c_str());
      int total = 0;
      for (int c = 0; c < p_grid->GetNumberCols(); c++) {
         total += p_grid->GetColSize(c);
         wxString label = p_grid->GetColLabelValue(c);
         label.Replace("\n", " ");
         fprintf(out, "col %d %d %s\n", c, p_grid->GetColSize(c),
                 (const char*)label.utf8_str());
      }
      fprintf(out, "colstotal %d\n", total);
      for (int r = 0; r < p_grid->GetNumberRows(); r++) {
         for (int c = 0; c < p_grid->GetNumberCols(); c++) {
            const wxString v = p_grid->GetCellValue(r, c);
            const bool fits = dc.GetTextExtent(v).x + 4 <= p_grid->GetColSize(c);
            const bool cut = c == p_modeColumn && r < (int)p_modeFull.size() &&
                             v != p_modeFull[r];
            fprintf(out, "cell %d %d %d %d %s\n", r + 1, c, fits ? 1 : 0,
                    cut ? 1 : 0, (const char*)v.utf8_str());
         }
         if (r < (int)p_vib.modes.size()) {
            const VibModeInfo& m = p_vib.modes[r];
            fprintf(out, "mode %d irrep %s ir %d raman %d known %d label %s | %s\n",
                    r + 1, m.irrep.empty() ? "-" : m.irrep.c_str(), m.ir,
                    m.raman, m.activityKnown, m.label.empty() ? "-" : m.label.c_str(),
                    m.detail.c_str());
         }
      }
      fprintf(out, "initial %d %s\n", p_initialMode + 1,
              p_initialMode >= 0 && p_initialMode < p_grid->GetNumberRows()
                 ? (const char*)p_grid->GetCellValue(p_initialMode, 0).utf8_str() : "");
      fprintf(out, "axis %g %g\n", p_spectrum->axis().lo(), p_spectrum->axis().hi());
      //  The selected row's detail line, as the panel shows it.
      selectMode(std::min(p_grid->GetNumberRows() - 1, 7));
      fprintf(out, "detail %s\n", (const char*)p_detailText.utf8_str());
      wxString shown = p_modeDetail ? p_modeDetail->GetLabel() : wxString();
      shown.Replace("\n", "|");
      fprintf(out, "detailshown %s\n", (const char*)shown.utf8_str());
      fclose(out);
      if (getenv("ECCE_EXIT_AFTER_DUMP") != 0)
         wxTheApp->CallAfter([]() {
            wxWindow *top = wxTheApp->GetTopWindow();
            if (top != 0) top->Close(true);
         });
   });
   timer->StartOnce(2500);
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
      analyseModes();

      p_grid->InsertCols(0,1);
      p_grid->SetColLabelValue(0,wxString::FromUTF8("Frequency\n" + unitLabel(vec->units())));

      p_symColumn = 1;
      p_grid->InsertCols(1,1);
      p_grid->SetColLabelValue(1,"Sym");

      //  ECCE's own description of the motion, for every code (#367).
      p_modeColumn = 2;
      p_grid->InsertCols(2,1);
      p_grid->SetColLabelValue(2,"Mode");

      //  Only show a column for a property the code actually produced:
      //  MOPAC computes no Raman activities, and a permanently empty
      //  column reads as a failed job.  The indices are tracked because
      //  skipping Infrared while keeping Raman shifts the other.
      p_irColumn = -1;
      p_ramanColumn = -1;
      int nextCol = 3;
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

      p_grid->InsertRows(0,nRows);
      p_modeFull.assign(nRows, wxString());

      for (int idx=0; idx<nRows; idx++) {
         p_grid->SetCellValue(idx,0,
               wxString::Format (PrefLabels::DOUBLEFORMAT, vec->value(idx)));

         //  The code's own label wins where it prints one (Gaussian,
         //  MOPAC: VIBSYM); otherwise ECCE's, which every code gets.
         if (idx < (int)p_codeSym.size() && !p_codeSym[idx].empty()) {
            p_grid->SetCellValue(idx,p_symColumn,wxString::FromUTF8(p_codeSym[idx]));
         } else if (idx < (int)p_vib.modes.size()) {
            p_grid->SetCellValue(idx,p_symColumn,
                                 wxString::FromUTF8(p_vib.modes[idx].irrep));
         }
         if (idx < (int)p_vib.modes.size())
            p_modeFull[idx] = wxString::FromUTF8(p_vib.modes[idx].label);

         if (ivec != (PropVector*)0 && p_irColumn >= 0) {
            p_grid->SetCellValue(idx,p_irColumn,
                  wxString::Format (PrefLabels::DOUBLEFORMAT, ivec->value(idx)));
         }

         if (rvec != (PropVector*)0 && p_ramanColumn >= 0) {
            p_grid->SetCellValue(idx,p_ramanColumn,
                  wxString::Format (PrefLabels::DOUBLEFORMAT, rvec->value(idx)));
         }
      }
   }

   //  Columns only: AutoSize() would also resize the grid window, and a
   //  docked panel must keep the width the dock gave it (the Mode column
   //  gives way instead, see fitModeColumn()).
   p_grid->AutoSizeColumns(false);
   p_grid->SetColLabelSize(wxGRID_AUTOSIZE);
   fitModeColumn();
}


void NModePanel::analyseModes()
{
   p_vib = VibResult();
   p_codeSym.clear();
   IPropCalculation *expt = getCalculation();
   PropVector *vec = (PropVector*) expt->getProperty("VIBFREQ");
   PropVecTable *vib = dynamic_cast<PropVecTable*>(expt->getProperty("VIB"));
   PropVecString *symvec = (PropVecString*) expt->getProperty("VIBSYM");
   if (symvec != 0)
      for (int i = 0; i < symvec->rows(); i++) p_codeSym.push_back(symvec->value(i));
   if (vec == 0 || vib == 0) return;

   Fragment frag;
   if (!expt->getFragment(frag)) return;
   const int nat = (int)frag.numAtoms(), nm = vec->rows();
   if (nat == 0 || vib->rows() != nat || vib->tables() != nm) return;

   vector<double> freqs;
   vector< vector<double> > modes(nm, vector<double>(3*nat));
   for (int m = 0; m < nm; m++) {
      freqs.push_back(vec->value(m));
      for (int a = 0; a < nat; a++)
         for (int k = 0; k < 3; k++) modes[m][3*a+k] = vib->value(m, a, k);
   }
   try {
      VibAnalysis::analyse(frag, freqs, modes, p_vib);
   } catch (...) {
      p_vib = VibResult();
   }
}


//  The Mode column takes what is left of the grid's width after the
//  numeric columns, up to a cap, and ellipsizes the text beyond it: a
//  docked panel is as wide as the dock makes it and must not grow.
void NModePanel::fitModeColumn()
{
   if (p_grid == 0 || p_modeColumn < 0 || p_modeFull.empty() ||
       p_grid->GetNumberCols() <= p_modeColumn) return;

   wxClientDC dc(p_grid->GetGridWindow());
   dc.SetFont(p_grid->GetDefaultCellFont());
   const int pad = 12, minWidth = 56, cap = 190;
   int longest = dc.GetTextExtent("Mode").x;
   for (size_t i = 0; i < p_modeFull.size(); i++)
      longest = std::max(longest, dc.GetTextExtent(p_modeFull[i]).x);

   int others = 0;
   for (int c = 0; c < p_grid->GetNumberCols(); c++)
      if (c != p_modeColumn) others += p_grid->GetColSize(c);
   const int scroll = wxSystemSettings::GetMetric(wxSYS_VSCROLL_X, p_grid);
   const int avail = p_grid->GetClientSize().x - others - scroll - 4;

   int width = std::min(longest + pad, cap);
   if (avail > 0) width = std::min(width, std::max(avail, minWidth));
   width = std::max(width, minWidth);
   p_grid->SetColSize(p_modeColumn, width);
   for (size_t i = 0; i < p_modeFull.size() && (int)i < p_grid->GetNumberRows(); i++)
      p_grid->SetCellValue((int)i, p_modeColumn,
         wxControl::Ellipsize(p_modeFull[i], dc, wxELLIPSIZE_END, width - pad));
}


static wxString activityText(const VibModeInfo& v)
{
   if (!v.activityKnown) return wxString();
   if (v.ir && v.raman) return "IR active, Raman active";
   if (v.ir) return "IR active, Raman inactive";
   if (v.raman) return "IR inactive, Raman active";
   return "IR inactive, Raman inactive";
}


wxString NModePanel::cellTip(int row, int col) const
{
   if (row < 0 || row >= (int)p_vib.modes.size()) return wxString();
   const VibModeInfo& v = p_vib.modes[row];
   if (col == p_modeColumn) {
      wxString t = wxString::FromUTF8(v.detail);
      const wxString act = activityText(v);
      if (!act.empty()) t += (t.empty() ? "" : "\n") + act;
      if (!v.remark.empty()) t += (t.empty() ? "" : "\n") + wxString::FromUTF8(v.remark);
      return t;
   }
   if (col == p_symColumn) {
      wxString t;
      if (!v.irrep.empty() && v.irrep != "?") {
         t = wxString::FromUTF8(v.irrep);
         if (!p_vib.group.empty()) t += " in " + wxString::FromUTF8(p_vib.group);
         const wxString act = activityText(v);
         if (!act.empty()) t += "\n" + act;
      } else if (v.irrep == "?") {
         t = "Symmetry not clear (the mode is not a pure irrep within the "
             "numerical noise)";
      }
      if (row < (int)p_codeSym.size() && !p_codeSym[row].empty())
         t += (t.empty() ? "" : "\n") + wxString("The code's own label: ") +
              wxString::FromUTF8(p_codeSym[row]);
      return t;
   }
   return wxString();
}


void NModePanel::OnGridMotion(wxMouseEvent& event)
{
   int x, y;
   p_grid->CalcUnscrolledPosition(event.GetX(), event.GetY(), &x, &y);
   wxWindow *win = p_grid->GetGridWindow();
   const wxString tip = cellTip(p_grid->YToRow(y), p_grid->XToCol(x));
   if (win->GetToolTipText() != tip) win->SetToolTip(tip.empty() ? NULL : new wxToolTip(tip));
   event.Skip();
}


void NModePanel::OnLabelMotion(wxMouseEvent& event)
{
   wxWindow *win = p_grid->GetGridColLabelWindow();
   int x, y;
   p_grid->CalcUnscrolledPosition(event.GetX(), 0, &x, &y);
   const int col = p_grid->XToCol(x);
   wxString tip;
   if (col == p_modeColumn) {
      tip = wxString::FromUTF8(
         "What the motion is, from the bonds, angles and torsions it "
         "changes.\n\xCE\xBD\xCC\x83 stretch, \xCE\xB4 bend, \xCF\x81 rock, "
         "\xCF\x89 wag, \xCF\x84 torsion, \xCE\xB3 out-of-plane bend; "
         "s symmetric, as antisymmetric.\n"
         "Two terms: both contribute; \xE2\x80\xA6 more. Hover a row for "
         "the shares.");
   } else if (col == p_symColumn) {
      tip = "Irreducible representation of the mode in the molecule's point "
            "group, found by ECCE from the mode's displacements.\n"
            "Where the code prints its own label, that is shown instead.";
   }
   if (win->GetToolTipText() != tip) win->SetToolTip(tip.empty() ? NULL : new wxToolTip(tip));
   event.Skip();
}


//  Two lines at most, broken at the " . " separators first and inside a part
//  only between words. When that does not fit, the last part (activity) is
//  dropped, then the description is cut after a whole word; the tooltip
//  keeps the full text.
static bool layoutParts(wxDC& dc, const std::vector<wxString>& parts, int width,
                        wxString& out)
{
   std::vector<wxString> lines;
   wxString cur;
   for (size_t i = 0; i < parts.size(); i++) {
      const wxString piece = parts[i] + (i + 1 < parts.size() ? wxString::FromUTF8(" \xC2\xB7") : wxString());
      const wxString joined = cur.empty() ? piece : cur + " " + piece;
      if (dc.GetTextExtent(joined).x <= width) { cur = joined; continue; }
      if (!cur.empty()) { lines.push_back(cur); cur.clear(); }
      if (dc.GetTextExtent(piece).x <= width) { cur = piece; continue; }
      wxString rest = piece;
      while (!rest.empty()) {
         wxString line;
         while (!rest.empty()) {
            const size_t cut = rest.find(' ');
            const wxString word = cut == wxString::npos ? rest : rest.substr(0, cut);
            const wxString t = line.empty() ? word : line + " " + word;
            if (!line.empty() && dc.GetTextExtent(t).x > width) break;
            line = t;
            rest = cut == wxString::npos ? wxString() : rest.substr(cut + 1);
         }
         if (rest.empty()) cur = line; else lines.push_back(line);
      }
   }
   if (!cur.empty()) lines.push_back(cur);
   if (lines.size() > 2) return false;
   out.clear();
   for (size_t i = 0; i < lines.size(); i++) out += (i ? "\n" : "") + lines[i];
   return true;
}


void NModePanel::setWrapped(wxStaticText *label, const std::vector<wxString>& parts)
{
   if (label == 0) return;
   wxString full;
   for (size_t i = 0; i < parts.size(); i++)
      full += (i ? wxString::FromUTF8(" \xC2\xB7 ") : wxString()) + parts[i];
   label->SetToolTip(full.empty() ? NULL : new wxToolTip(full));
   int width = label->GetClientSize().x;
   //  Test hook: the width of the narrowest dock, without resizing windows.
   if (const char *w = getenv("ECCE_NMODE_DETAIL_WIDTH")) width = atoi(w);
   wxString shown = full;
   if (width > 40 && !full.empty()) {
      wxClientDC dc(label);
      dc.SetFont(label->GetFont());
      std::vector<wxString> p = parts;
      bool ok = layoutParts(dc, p, width, shown);
      if (!ok && p.size() > 2) { p.pop_back(); ok = layoutParts(dc, p, width, shown); }
      //  Shorten the last remaining part by whole words, with an ellipsis.
      while (!ok && !p.empty()) {
         wxString& last = p.back();
         last.Replace(wxString::FromUTF8("\xE2\x80\xA6"), "");
         const size_t cut = last.rfind(' ');
         if (cut == wxString::npos) {
            if (p.size() == 1) { shown = wxControl::Ellipsize(last, dc, wxELLIPSIZE_END, width); break; }
            p.pop_back();
         } else {
            last = last.substr(0, cut) + wxString::FromUTF8("\xE2\x80\xA6");
         }
         ok = layoutParts(dc, p, width, shown);
      }
   }
   if (label->GetLabel() != shown) label->SetLabel(shown);
}


static wxString activityShort(const VibModeInfo& v)
{
   if (!v.activityKnown) return wxString();
   if (v.ir && v.raman) return "IR, Raman";
   if (v.ir) return "IR";
   if (v.raman) return "Raman";
   return "inactive";
}


void NModePanel::showModeDetail(int row)
{
   if (p_modeDetail == 0) return;
   wxString freq, text;
   std::vector<wxString> vparts;
   if (row >= 0 && row < (int)p_vib.modes.size() && p_grid != 0 &&
       row < p_grid->GetNumberRows()) {
      const VibModeInfo& v = p_vib.modes[row];
      wxString sym = p_grid->GetNumberCols() > p_symColumn && p_symColumn >= 0
                        ? p_grid->GetCellValue(row, p_symColumn) : wxString();
      //  The table keeps the user's number format; the label is read, not
      //  compared, so one decimal is enough.
      double f = 0.0;
      wxString cell = p_grid->GetCellValue(row, 0);
      freq = (cell.ToCDouble(&f) ? wxString::Format("%.1f", f) : cell) +
             wxString::FromUTF8(" cm\xE2\x81\xBB\xC2\xB9");
      const wxString parts[3] = { sym, wxString::FromUTF8(v.detail),
                                  activityShort(v) };
      for (int i = 0; i < 3; i++)
         if (!parts[i].empty()) {
            text += (text.empty() ? "" : wxString::FromUTF8(" \xC2\xB7 ")) + parts[i];
            vparts.push_back(parts[i]);
         }
   }
   p_detailText = text;
   p_detailParts = vparts;
   setWrapped(p_modeDetail, vparts);
   p_graphParts = vparts;
   if (!freq.empty()) p_graphParts.insert(p_graphParts.begin(), freq);
   setWrapped(p_graphDetail, p_graphParts);
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
   showModeDetail(index);

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

   // Another mode while Animation is chosen: play that one, also after a pause.
   wxRadioBox *vizbox = (wxRadioBox*)FindWindow(ID_RADIOBOX_NMODE_VIZTYPE);
   if (vizbox && vizbox->GetSelection() == 0 && p_active) {
      p_paused = false;
      start();
   }

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
      // Calculations without a geometry trace have no step to go back to.
      if (p_home.empty()) {
         SGFragment *frag = getFW().getSceneGraph().getFragment();
         for (size_t i = 0; frag && i < frag->numAtoms(); i++) {
            const double *x = frag->atomRef(i)->coordinates();
            p_home.insert(p_home.end(), x, x + 3);
         }
      }
      p_animating = true;
      p_timer->StartOnce(animationDelay());
   }
}

void NModePanel::stop()
{
   p_animating = false;
   p_paused = false;
   p_timer->Stop();
   restoreHome();
}

void NModePanel::restoreHome()
{
   if (p_home.empty()) return;
   std::vector<double> home;
   home.swap(p_home);
   SGContainer& sg = getFW().getSceneGraph();
   SGFragment *frag = sg.getFragment();
   // An edited structure is not ours to move back.
   if (!frag || frag->numAtoms() * 3 != home.size() || !modesApply()) return;
   for (size_t i = 0; i < frag->numAtoms(); i++)
      frag->atomRef(i)->coordinates(&home[3*i]);
   sg.touchChemDisplay(frag);
   frag->touch();
   sg.adjustMeasures();
}

bool NModePanel::togglePause()
{
   if (p_animating) {
      p_animating = false;
      p_paused = true;
      p_timer->Stop();
      return true;
   }
   if (!p_paused) return false;
   p_paused = false;
   start();
   return true;
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
   // hasFocus() turns true only after this returns.
   p_active = true;
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
   p_active = false;
   SGContainer& sg = getFW().getSceneGraph();
   sg.getNMVecRoot()->whichChild.setValue(SO_SWITCH_NONE);
   sg.getNMRoot()->whichChild.setValue(SO_SWITCH_NONE);
   sg.getcsSwitch()->whichChild.setValue(SO_SWITCH_ALL);
   stop();
   // Restore after the animation, never over an edited structure.
   if (checkStructure()) selectFragStep(-1);
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
