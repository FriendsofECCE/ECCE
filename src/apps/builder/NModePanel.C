#include "util/UnitDisplay.H"
#include "util/VibFrequency.H"
#include "util/Ecce.H"
#include "dsm/ICalculation.H"
#include "dsm/TaskJob.H"
#include "tdat/TRunType.H"
#include "dsm/VDoc.H"
#include <cmath>
#include <iostream>
  using std::cout;
  using std::cerr;
  using std::endl;
#include <algorithm>
  using std::fill;
#include <limits>
#include <functional>

#include <sstream>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <wx/checkbox.h>
#include <wx/tglbtn.h>
#include <wx/radiobut.h>
#include <wx/statbox.h>
#include <wx/dialog.h>
#include <wx/choicdlg.h>
#include <wx/modalhook.h>
#include <wx/listbox.h>
#include <wx/utils.h>
#include <wx/radiobox.h>
#include <wx/dcclient.h>
#include <wx/dcscreen.h>
#include <wx/app.h>
#include <wx/frame.h>
#include <wx/grid.h>
#include <wx/msgdlg.h>
#include <wx/button.h>
#include <wx/control.h>
#include <wx/settings.h>
#include <wx/dcmemory.h>
#include <wx/link.h>
#include <wx/listctrl.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/tooltip.h>
#include <wx/tokenzr.h>
#include <wx/uiaction.h>
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
#include <wx/clipbrd.h>
#include <wx/file.h>
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
#include "wxgui/WindowShot.H"

#include "viz/NModeStepCmd.H"
#include "viz/NModeTraceCmd.H"
#include "viz/NModeVectCmd.H"
#include "viz/SGContainer.H"
#include "viz/SGFragment.H"

#include "wxviz/SGSelection.H"
#include "wxviz/WxVizToolFW.H"

#include "NModePanel.H"

static wxString activityText(const VibModeInfo& v);

namespace {
//  Height of a label's text as drawn, all its lines.
int multiLineHeight(wxStaticText *t)
{
   wxClientDC dc(t);
   dc.SetFont(t->GetFont());
   return dc.GetMultiLineTextExtent(t->GetLabel()).y;
}
}


//  Red that stays readable on light and dark table backgrounds.
static wxColour imaginaryColour()
{
   return wxColour(214, 39, 40);
}

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
   unsubscribe();
   delete p_timer;
   //  Destroy callbacks while their panel is still alive; frame destruction
   //  itself is deferred by wxWidgets.
   if (p_spectrumPop) p_spectrumPop->setClickHandler(0);
   if (p_spectrumFrame) {
      p_spectrumFrame->DestroyChildren();
      p_spectrumFrame->Destroy();
   }
}



bool NModePanel::Create(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id, const wxPoint& pos,
        const wxSize& size, long style, const wxString& name)
{
   if (!NModesGUI::Create(calculation, parent, id, pos, size, style )) {
      wxFAIL_MSG( wxT("NModePanel creation failed") );
      return false;
   }
   subscribe();

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
   //  Results, not input: a key press (space) must not open and clear a cell.
   p_grid->EnableEditing(false);
   //  Columns can be dragged wider; a dragged Mode column keeps its width.
   p_grid->EnableDragColSize(true);
   p_grid->Bind(wxEVT_GRID_COL_SIZE, [this](wxGridSizeEvent& e) {
      if (e.GetRowOrCol() == p_modeColumn) {
         p_modeUserWidth = p_grid->GetColSize(p_modeColumn);
         fitModeColumn();
      }
      e.Skip();
   });

   //  Two lines under the table for the selected mode: wrapped to the
   //  width the dock gives, a tiny minimum width so it never widens the
   //  panel; the whole text is its tooltip.
   {
      wxSizer *gs = p_grid->GetContainingSizer();
      //  A thin-bordered box, like the plot's: text about the selection is
      //  never loose among the controls.
      p_modeBox = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                              wxBORDER_SIMPLE);
      p_modeBox->SetBackgroundColour(GetBackgroundColour().ChangeLightness(106));
      p_modeDetail = new wxStaticText(p_modeBox, wxID_ANY, "");
      p_modeDetail->SetMinSize(wxSize(50, 2*p_modeDetail->GetCharHeight() + 2));
      wxBoxSizer *boxSizer = new wxBoxSizer(wxVERTICAL);
      boxSizer->Add(p_modeDetail, 1, wxEXPAND | wxALL, 4);
      p_modeBox->SetSizer(boxSizer);
      wxBoxSizer *col = new wxBoxSizer(wxVERTICAL);
      gs->Detach(p_grid);
      col->Add(p_grid, 1, wxEXPAND);
      col->Add(p_modeBox, 0, wxEXPAND | wxTOP, 3);
      gs->Insert(0, col, 1, wxGROW | wxALL, 3);
      //  A window made now is shown whatever its sizer item's state is.
      p_modeBox->Show(!isGraphShown());
   }
   p_modeDetail->Bind(wxEVT_SIZE, [this](wxSizeEvent& e) {
      setWrapped(p_modeDetail, p_detailParts);
      e.Skip();
   });
   p_graphDetail->Bind(wxEVT_SIZE, [this](wxSizeEvent& e) {
      wrapText(p_graphDetail, p_graphText);
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
   p_slider->SetRange(0.0, MAX_SCALE);
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

   buildLayout();

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
      if (p_grid->GetCellValue(r, 0).ToDouble(&f) && (VibFrequency::isReal(f) || VibFrequency::isImaginary(f))) {
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
         syncViewControls();
      }
   }

   getFW().getViewer().getSel()->deselectAll();
   getFW().getViewer().viewAll();

   dumpSpectrumIfRequested();
   dumpTableIfRequested();
   dumpCompositionIfRequested();
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


bool NModePanel::scriptSpectrumClick(int mode, double *mismatch,
                                     int *selected)
{
   if (!p_isValid || !isGraphShown()) return false;
   *mismatch = p_spectrum->screenMismatch();
   const VibSpectrum& s = p_spectrum->spectrum();
   const VibKind kind = s.has(VIB_IR) ? VIB_IR : VIB_RAMAN;
   wxPoint at;
   if (!p_spectrum->stickPosition(kind, mode, &at)) return false;
   //  The press and release of a left click, a few pixels below the top
   //  of the stick, through the canvas's own handlers.
   const wxEventType types[] = {wxEVT_LEFT_DOWN, wxEVT_LEFT_UP};
   for (wxEventType t : types) {
      wxMouseEvent e(t);
      e.SetPosition(wxPoint(at.x, at.y + 3));
      e.SetEventObject(p_spectrum);
      e.m_leftDown = t == wxEVT_LEFT_DOWN;
      p_spectrum->GetEventHandler()->ProcessEvent(e);
   }
   *selected = p_spectrum->selected();
   return true;
}


void NModePanel::showTable()
{
   p_gridPlotSizer->Show((size_t)0,true);
   p_gridPlotSizer->Show((size_t)1,false);
   p_gridPlotSizer->Layout();
   syncViewControls();
}

void NModePanel::showGraph()
{
   p_gridPlotSizer->Show((size_t)1,true);
   p_gridPlotSizer->Show((size_t)0,false);
   p_gridPlotSizer->Layout();
   syncViewControls();
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
 * The plot: the spectrum canvas and the selected mode under it.  Item 1 of
 * p_gridPlotSizer, which showGraph()/showTable() rely on.  IR and Raman are
 * stacked here; the settings live in the Spectrum window.
 */
void NModePanel::createSpectrumPane()
{
   ewxConfig *config = ewxConfig::getConfig(INIFILE);
   double fwhm = 15.0, scale = 1.0;
   bool reversed = true, sticks = true;
   config->Read("NMode/FWHM", &fwhm, 15.0);
   //  The factor is deliberately not remembered: a forgotten 0.5 would
   //  silently shift every later spectrum.
   config->Read("NMode/WavenumberHighOnLeft", &reversed, true);
   config->Read("NMode/ShowSticks", &sticks, true);
   if (fwhm < 0 || fwhm > 200) fwhm = 15.0;

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
   //  In a box of its own, paler than the controls and thin-bordered, so
   //  it is not taken for one of them; it wraps and is never ellipsized.
   p_graphBox = new wxPanel(pane, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                            wxBORDER_SIMPLE);
   p_graphBox->SetBackgroundColour(pane->GetBackgroundColour().ChangeLightness(106));
   p_graphDetail = new wxStaticText(p_graphBox, wxID_ANY, "");
   p_graphDetail->SetMinSize(wxSize(50, 2*p_graphDetail->GetCharHeight() + 2));
   wxBoxSizer *boxSizer = new wxBoxSizer(wxVERTICAL);
   boxSizer->Add(p_graphDetail, 1, wxEXPAND | wxALL, 4);
   p_graphBox->SetSizer(boxSizer);
   col->Add(p_graphBox, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 6);

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
   pane->Bind(wxEVT_SIZE, [col, noRoom, this](wxSizeEvent& e) {
      const int h = e.GetSize().y;
      const bool plot = h >= 110, text = h >= 150;
      if (col->IsShown(p_spectrum) != plot || col->IsShown(p_graphBox) != text) {
         col->Show(p_spectrum, plot);
         col->Show(p_graphBox, text);
         col->Show(noRoom, !plot);
      }
      e.Skip();
   });

   pane->SetSizer(col);
   p_gridPlotSizer->Add(pane, 1, wxGROW);

   //  The wrap sizers under the plot learn their width only in a layout, so
   //  the first one reserved them several rows (on MSW about 100 px) and the
   //  plot did not fit; a second pass at the same size gives it back.
   Bind(wxEVT_SIZE, [this](wxSizeEvent& e) {
      e.Skip();
      CallAfter([this]() { Layout(); });
   });
}


/**
 * The rest of the panel, top to bottom: Spectrum | Table, the plot (made
 * above), the "Mode in the viewer" group and the row of windows.  The old
 * Animation/Vector and Graph/Table radio boxes stay as hidden state holders,
 * so the script hooks and handlers that drive them keep working.
 */
void NModePanel::buildLayout()
{
   wxSizer *top = GetSizer();
   FindWindow(ID_RADIOBOX_NMODE_VIZTYPE)->Hide();
   FindWindow(ID_RADIOBOX_NMODE_DATAVIEW)->Hide();

   wxBoxSizer *tr = new wxBoxSizer(wxHORIZONTAL);
   p_viewSpectrum = new wxToggleButton(this, wxID_ANY, "Spectrum");
   p_viewTable = new wxToggleButton(this, wxID_ANY, "Table");
   auto choose = [this](int i) {
      wxRadioBox *v = (wxRadioBox*)FindWindow(ID_RADIOBOX_NMODE_DATAVIEW);
      v->SetSelection(i);
      wxCommandEvent ev(wxEVT_RADIOBOX, v->GetId());
      ev.SetEventObject(v);
      ev.SetInt(i);
      v->GetEventHandler()->ProcessEvent(ev);
   };
   p_viewSpectrum->Bind(wxEVT_TOGGLEBUTTON, [choose](wxCommandEvent&) { choose(0); });
   p_viewTable->Bind(wxEVT_TOGGLEBUTTON, [choose](wxCommandEvent&) { choose(1); });
   p_viewSpectrum->SetToolTip("The IR and Raman spectra, one over the other.");
   p_viewTable->SetToolTip("The frequencies as a table.");
   p_exportBtn = new wxButton(this, wxID_ANY, "Export .csv...");
   p_exportBtn->SetToolTip("Save the spectrum (sticks and broadened curve) "
                           "as a comma-separated file for a spreadsheet or "
                           "plotting program.");
   p_exportBtn->Bind(wxEVT_BUTTON,
                     [this](wxCommandEvent& e) { exportSpectrumDialog(e.GetString()); });
   tr->Add(p_viewSpectrum, 0, wxALL, 3);
   tr->Add(p_viewTable, 0, wxALL, 3);
   tr->AddStretchSpacer(1);
   tr->Add(p_exportBtn, 0, wxALIGN_CENTER_VERTICAL | wxALL, 3);
   top->Insert(0, tr, 0, wxEXPAND);

   //  Detach the old rows' contents and regroup them.
   wxSizer *box5 = 0;
   for (wxSizerItem *it : top->GetChildren())
      if (it->GetSizer() && it->GetSizer()->GetItem(p_)) box5 = it->GetSizer();
   box5->Detach(p_);
   box5->Detach(p_styleSizer);
   p_->Detach(p_sliderSizer);
   p_styleSizer->Detach(p_animateSizer);
   p_styleSizer->Detach(p_vectorSizer);

   wxStaticBoxSizer *group = new wxStaticBoxSizer(wxVERTICAL, this, "Mode in the viewer");
   wxStaticBox *sb = group->GetStaticBox();
   //  The controls moved into the box must be its children, or the box
   //  is drawn over them.
   std::function<void(wxSizer*)> adopt = [&](wxSizer *z) {
      for (wxSizerItem *it : z->GetChildren()) {
         if (it->GetWindow()) it->GetWindow()->Reparent(sb);
         else if (it->GetSizer()) adopt(it->GetSizer());
      }
   };
   adopt(p_sliderSizer);
   adopt(p_animateSizer);
   adopt(p_vectorSizer);
   wxWrapSizer *kind = new wxWrapSizer(wxHORIZONTAL);
   p_showCheck = new wxCheckBox(sb, wxID_ANY, "Show");
   p_showCheck->SetValue(true);
   p_showCheck->SetToolTip("Draw the selected mode in the viewer. Off shows "
                           "the molecule alone.");
   p_animRadio = new wxRadioButton(sb, wxID_ANY, "Animation", wxDefaultPosition,
                                   wxDefaultSize, wxRB_GROUP);
   p_vecRadio = new wxRadioButton(sb, wxID_ANY, "Vectors");
   kind->Add(p_showCheck, 0, wxALIGN_CENTER_VERTICAL | wxALL, 3);
   kind->Add(p_animRadio, 0, wxALIGN_CENTER_VERTICAL | wxALL, 3);
   kind->Add(p_vecRadio, 0, wxALIGN_CENTER_VERTICAL | wxALL, 3);
   adoptViewerToggle(p_showCheck);
   auto pick = [this](int i) {
      wxRadioBox *v = (wxRadioBox*)FindWindow(ID_RADIOBOX_NMODE_VIZTYPE);
      v->SetSelection(i);
      wxCommandEvent ev(wxEVT_RADIOBOX, v->GetId());
      ev.SetEventObject(v);
      ev.SetInt(i);
      v->GetEventHandler()->ProcessEvent(ev);
   };
   p_animRadio->Bind(wxEVT_RADIOBUTTON, [pick](wxCommandEvent&) { pick(0); });
   p_vecRadio->Bind(wxEVT_RADIOBUTTON, [pick](wxCommandEvent&) { pick(1); });
   group->Add(kind, 0, wxEXPAND);

   p_viewerRow = new wxBoxSizer(wxHORIZONTAL);
   p_viewerRow->Add(p_sliderSizer, 0, wxALIGN_CENTER_VERTICAL);
   p_viewerRow->Add(p_animateSizer, 0, wxALIGN_CENTER_VERTICAL);
   p_viewerRow->Add(p_vectorSizer, 0, wxALIGN_CENTER_VERTICAL);
   group->Add(p_viewerRow, 0, wxEXPAND);
   top->Add(group, 0, wxEXPAND | wxALL, 3);

   FindWindow(ID_CHECKBOX_NMODE_VECSIGN)->SetToolTip(
      "Show the displacement in the opposite direction (the negative of the mode).");

   wxWrapSizer *bottom = new wxWrapSizer(wxHORIZONTAL);
   wxButton *window = new wxButton(this, wxID_ANY, "Open in window...");
   window->SetToolTip("The IR and Raman spectra in their own window, with "
                      "width, scale factor, comparison and export.");
   window->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { openSpectrumWindow(); });
   wxButton *composition = new wxButton(this, wxID_ANY, "Composition...");
   composition->SetToolTip("What each mode is made of: its share along "
                           "bond stretches, angle bends and torsions.");
   composition->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
      wxString err;
      if (!showComposition(-1, &err))
         wxMessageBox(err, "Mode composition", wxOK | wxICON_INFORMATION, this);
   });
   wxButton *displace = new wxButton(this, wxID_ANY, "Displace...");
   displace->SetToolTip("New calculation(s) with the structure displaced "
                        "along the selected mode or all imaginary modes: "
                        "the way off a saddle point.");
   displace->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { displaceDialog(); });
   for (wxButton *b : {window, composition, displace})
      bottom->Add(b, 0, wxALIGN_CENTER_VERTICAL | wxALL, 3);
   top->Add(bottom, 0, wxEXPAND | wxALL, 0);

   showStyle(((wxRadioBox*)FindWindow(ID_RADIOBOX_NMODE_VIZTYPE))->GetSelection() == 0);
   syncViewControls();
}


/// The toggles, the viewer-group radios and the export button follow the
/// hidden radio boxes, however those were changed.
void NModePanel::syncViewControls()
{
   if (p_viewSpectrum == 0) return;
   const bool graph = isGraphShown();
   p_viewSpectrum->SetValue(graph);
   p_viewTable->SetValue(!graph);
   p_exportBtn->Show(!graph);
   wxRadioBox *viz = (wxRadioBox*)FindWindow(ID_RADIOBOX_NMODE_VIZTYPE);
   if (viz != 0 && p_animRadio != 0) {
      p_animRadio->SetValue(viz->GetSelection() == 0);
      p_vecRadio->SetValue(viz->GetSelection() != 0);
   }
   Layout();
}


void NModePanel::showStyle(bool animation)
{
   if (p_viewerRow == 0) return;
   p_viewerRow->Show(p_animateSizer, animation);
   p_viewerRow->Show(p_vectorSizer, !animation);
   p_viewerRow->Layout();
   Layout();
}


/**
 * The Spectrum window's controls, above the plot: the settings of the
 * spectrum, and the other spectra drawn with it.
 */
void NModePanel::createWindowControls(wxWindow *pane, wxBoxSizer *col)
{
   const double fwhm = p_spectrum->spectrum().fwhm();
   const double scale = p_spectrum->spectrum().scale();
   const bool reversed = p_spectrum->reversed(), sticks = p_spectrum->showSticks();
   const int gap = 6;

   wxStaticBoxSizer *g1 = new wxStaticBoxSizer(wxHORIZONTAL, pane, "Spectrum");
   wxWindow *w1 = g1->GetStaticBox();
   wxStaticBoxSizer *g2 = new wxStaticBoxSizer(wxHORIZONTAL, pane, "Other spectra");
   wxWindow *w2 = g2->GetStaticBox();
   auto add = [gap](wxSizer *s, wxWindow *w, int left = 0) {
      s->Add(w, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, left ? left : gap / 2);
   };

   add(g1, new wxStaticText(w1, wxID_ANY, "Width"));
   wxTextCtrl *width = new wxTextCtrl(w1, wxID_ANY,
                  wxString::Format("%g", fwhm), wxDefaultPosition,
                  wxSize(52, -1), wxTE_PROCESS_ENTER);
   width->SetToolTip(wxString::FromUTF8(
      "Full width at half maximum of each band, in cm\xE2\x81\xBB\xC2\xB9 "
      "(0 to 200).\nCondensed-phase bands are typically 5 to 30 wide.\n"
      "0 shows sticks only.\nA narrow width gives sharp lines at the band "
      "origins; it does not reproduce the rotational structure of a "
      "gas-phase spectrum."));
   add(g1, width);
   add(g1, new wxStaticText(w1, wxID_ANY, wxString::FromUTF8("cm\xE2\x81\xBB\xC2\xB9")));
   add(g1, new wxStaticText(w1, wxID_ANY, "Scale factor"), gap * 2);
   wxTextCtrl *factor = new wxTextCtrl(w1, wxID_ANY,
                  wxString::Format("%g", scale), wxDefaultPosition,
                  wxSize(56, -1), wxTE_PROCESS_ENTER);
   factor->SetToolTip(wxString::FromUTF8(
      "Multiplies every frequency. Harmonic frequencies from most "
      "methods are 3 to 5 % too high; a typical factor is 0.96."));
   add(g1, factor);
   wxCheckBox *stk = new wxCheckBox(w1, wxID_ANY, "Sticks");
   stk->SetValue(sticks);
   add(g1, stk, gap * 2);
   wxCheckBox *left = new wxCheckBox(w1, wxID_ANY, "High on left");
   left->SetValue(reversed);
   add(g1, left, gap * 2);
   add(g1, new wxStaticText(w1, wxID_ANY, "Range"), gap * 2);
   wxTextCtrl *lo = new wxTextCtrl(w1, wxID_ANY, "", wxDefaultPosition,
                             wxSize(60, -1), wxTE_PROCESS_ENTER);
   wxTextCtrl *hi = new wxTextCtrl(w1, wxID_ANY, "", wxDefaultPosition,
                             wxSize(60, -1), wxTE_PROCESS_ENTER);
   lo->SetToolTip("Lowest wavenumber shown, cm-1; Enter applies the range.");
   hi->SetToolTip("Highest wavenumber shown, cm-1; Enter applies the range.");
   add(g1, lo);
   add(g1, new wxStaticText(w1, wxID_ANY, "to"));
   add(g1, hi);
   wxButton *reset = new wxButton(w1, wxID_ANY, "Reset zoom");
   reset->SetToolTip("Show the whole spectrum (or double-click the plot, "
                     "or press Home).\nClick a peak to show its mode. Drag "
                     "to zoom to a band, mouse wheel to zoom, right-drag "
                     "to pan.");
   add(g1, reset, gap);
   //  Range is applied by Enter in either field or when the second loses
   //  focus; a drag on the plot sets the same fields.
   auto doZoom = [this, lo, hi]() {
      double a, b;
      if (lo->GetValue().ToCDouble(&a) && hi->GetValue().ToCDouble(&b))
         zoomSpectrum(a, b);
   };
   hi->Bind(wxEVT_KILL_FOCUS, [doZoom](wxFocusEvent& e) { doZoom(); e.Skip(); });
   hi->Bind(wxEVT_TEXT_ENTER, [doZoom](wxCommandEvent&) { doZoom(); });
   lo->Bind(wxEVT_TEXT_ENTER, [doZoom](wxCommandEvent&) { doZoom(); });

   wxButton *compare = new wxButton(w2, wxID_ANY, "Compare...");
   compare->SetToolTip("Another calculation in this folder: draw its "
                       "spectrum with this one, or match its bands with "
                       "this one's modes.");
   compare->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { compareDialog(); });
   add(g2, compare, gap);
   add(g2, new wxStaticText(w2, wxID_ANY, "(overlay or mode matching)"));
   wxCheckBox *difference = new wxCheckBox(w2, wxID_ANY, "Difference");
   difference->SetToolTip("Draw the second calculation's curve minus this "
                          "one's (choose Compare... first), with the same "
                          "line width and scaling factor.");
   difference->SetValue(p_showDiff);
   difference->Enable(p_compare != 0);
   difference->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent& e) {
      setShowDifference(e.IsChecked());
   });
   add(g2, difference, gap * 2);
   wxButton *expBtn = new wxButton(w2, wxID_ANY, "Experimental...");
   expBtn->SetToolTip("Draw a measured spectrum from a CSV file (wavenumber "
                      "and absorbance or transmittance), scaled to this "
                      "calculation's tallest peak in view.");
   expBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { importExperimentalDialog(); });
   add(g2, expBtn, gap);
   wxButton *remove = new wxButton(w2, wxID_ANY, "Remove");
   remove->SetToolTip("Remove the other spectra: the comparison, the "
                      "difference and the experimental curve.");
   remove->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { removeOtherSpectra(); });
   remove->Enable(hasOtherSpectra());
   add(g2, remove, gap);
   g2->AddStretchSpacer(1);
   wxButton *csv = new wxButton(w2, wxID_ANY, "Export .csv...");
   csv->Bind(wxEVT_BUTTON, [this](wxCommandEvent& e) { exportSpectrumDialog(e.GetString()); });
   add(g2, csv, gap);

   col->Add(g1, 0, wxEXPAND | wxALL, 3);
   col->Add(g2, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 3);

   width->SetName("spectrum-width");
   factor->SetName("spectrum-factor");
   lo->SetName("spectrum-low");
   hi->SetName("spectrum-high");
   left->SetName("spectrum-direction");
   stk->SetName("spectrum-sticks");
   difference->SetName("spectrum-difference");
   reset->SetName("spectrum-reset");
   expBtn->SetName("spectrum-experimental");
   compare->SetName("spectrum-compare");
   remove->SetName("spectrum-remove");
   csv->SetName("spectrum-export-csv");
   auto apply = [this, width, factor]() { applySpectrumSettings(width, factor); };
   for (wxTextCtrl *field : {width, factor}) {
      field->Bind(wxEVT_TEXT_ENTER, [apply](wxCommandEvent&) { apply(); });
      field->Bind(wxEVT_KILL_FOCUS, [apply](wxFocusEvent& e) { apply(); e.Skip(); });
   }
   left->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent& e) {
      p_spectrum->setReversed(e.IsChecked());
      ewxConfig::getConfig(INIFILE)->Write("NMode/WavenumberHighOnLeft", e.IsChecked());
   });
   stk->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent& e) {
      p_spectrum->setShowSticks(e.IsChecked());
      ewxConfig::getConfig(INIFILE)->Write("NMode/ShowSticks", e.IsChecked());
   });
   reset->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { p_spectrum->resetZoom(); });

   //  Read back the shared state, including mouse zooms on the plot.
   //  Leave a field being edited alone until Enter or focus loss applies it.
   auto syncText = [](wxTextCtrl *field, std::function<double()> value) {
      field->SetExtraStyle(field->GetExtraStyle() | wxWS_EX_PROCESS_UI_UPDATES);
      field->Bind(wxEVT_UPDATE_UI, [field, value](wxUpdateUIEvent&) {
         const wxString text = wxString::Format("%g", value());
         if (!field->HasFocus() && field->GetValue() != text) field->ChangeValue(text);
      });
   };
   syncText(width, [this]() { return p_spectrum->spectrum().fwhm(); });
   syncText(factor, [this]() { return p_spectrum->spectrum().scale(); });
   syncText(lo, [this]() { return p_spectrum->axis().lo(); });
   syncText(hi, [this]() { return p_spectrum->axis().hi(); });
   for (wxWindow *w : std::initializer_list<wxWindow*>{left, stk, difference, remove})
      w->SetExtraStyle(w->GetExtraStyle() | wxWS_EX_PROCESS_UI_UPDATES);
   left->Bind(wxEVT_UPDATE_UI, [this](wxUpdateUIEvent& e) { e.Check(p_spectrum->reversed()); });
   stk->Bind(wxEVT_UPDATE_UI, [this](wxUpdateUIEvent& e) { e.Check(p_spectrum->showSticks()); });
   difference->Bind(wxEVT_UPDATE_UI, [this](wxUpdateUIEvent& e) {
      e.Check(p_showDiff);
      e.Enable(p_compare != nullptr);
   });
   remove->Bind(wxEVT_UPDATE_UI, [this](wxUpdateUIEvent& e) {
      e.Enable(hasOtherSpectra());
   });
}


void NModePanel::removeOtherSpectra()
{
   setShowDifference(false);
   if (!p_expX.empty()) importExperimental("", "");
   compareWith("");
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
   wxPanel *panel = new wxPanel(frame);
   SpectrumCanvas *canvas = new SpectrumCanvas(panel);
   canvas->shareWith(*p_spectrum);
   canvas->setClickHandler(this);
   canvas->setCompactBelow(0);
   canvas->setStrip(true);
   wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
   createWindowControls(panel, sizer);
   sizer->Add(canvas, 1, wxEXPAND);
   wxBoxSizer *foot = new wxBoxSizer(wxHORIZONTAL);
   foot->AddStretchSpacer(1);
   wxButton *comp = new wxButton(panel, wxID_ANY, "Composition...");
   wxButton *close = new wxButton(panel, wxID_ANY, "Close");
   comp->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
      wxString err;
      if (!showComposition(-1, &err))
         wxMessageBox(err, "Mode composition", wxOK | wxICON_INFORMATION, this);
   });
   close->Bind(wxEVT_BUTTON, [frame](wxCommandEvent&) { frame->Close(); });
   foot->Add(comp, 0, wxALL, 4);
   foot->Add(close, 0, wxALL, 4);
   sizer->Add(foot, 0, wxEXPAND);
   panel->SetSizer(sizer);
   wxBoxSizer *outer = new wxBoxSizer(wxVERTICAL);
   outer->Add(panel, 1, wxEXPAND);
   frame->SetSizer(outer);
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

void NModePanel::exportSpectrumDialog(const wxString& requestedPath)
{
   IPropCalculation *calc = getCalculation();
   if (calc == 0) return;
   wxString path = requestedPath;
   if (path.empty()) {
      wxFileDialog dlg(this, "Export spectrum",  wxEmptyString,
                    wxString::FromUTF8(calc->getURL().getFilePathTail().c_str()) +
                    "-spectrum.csv", "CSV files (*.csv)|*.csv",
                    wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
      if (dlg.ShowModal() != wxID_OK) return;
      path = dlg.GetPath();
   }
   if (!exportSpectrumCsv(path, spectrumKindShown()))
      wxMessageBox("The file could not be written.", "Export spectrum",
                   wxOK | wxICON_ERROR, this);
}

/**
 * Width and scale from their entry fields, kept within what means
 * something: a width of 0 to 200 cm-1 (0 is sticks only), a positive
 * scale.  A field left unreadable goes back to what is in force.
 */
void NModePanel::applySpectrumSettings(wxTextCtrl *width, wxTextCtrl *factor)
{
   if (p_spectrum == 0 || (width == 0 && factor == 0)) return;
   double fwhm = p_spectrum->spectrum().fwhm();
   double scale = p_spectrum->spectrum().scale();
   //  A decimal comma is read as a point, whatever the locale.
   auto parse = [](const wxString& t, double *out) {
      wxString c = t;
      c.Replace(",", ".");
      return c.Trim(true).Trim(false).ToCDouble(out);
   };
   double v;
   if (width != 0 && parse(width->GetValue(), &v) && v >= 0) {
      fwhm = v > 200 ? 200 : v;
      if (fwhm > 0 && fwhm < 0.1) fwhm = 0.1;
   }
   if (factor != 0 && parse(factor->GetValue(), &v) && v > 0) scale = v;
   if (width != 0) width->ChangeValue(wxString::Format("%g", fwhm));
   if (factor != 0) factor->ChangeValue(wxString::Format("%g", scale));

   const bool scaleChanged = scale != p_spectrum->spectrum().scale();
   if (fwhm != p_spectrum->spectrum().fwhm()) p_spectrum->setFwhm(fwhm);
   if (scaleChanged) {
      p_spectrum->setScale(scale);
      //  The detail line quotes the scaled value too.
      if (p_grid != 0 && p_spectrum->selected() >= 0)
         showModeDetail(p_spectrum->selected());
   }

   ewxConfig *config = ewxConfig::getConfig(INIFILE);
   config->Write("NMode/FWHM", fwhm);
}


void NModePanel::modeClicked(int mode)
{
   if (mode < 0 || mode >= p_grid->GetNumberRows()) return;
   selectMode(mode);
}

void NModePanel::showAnimationMode()
{
   showStyle(true);
   p_scaleLabel->SetLabel(_("Amplitude:"));

   SGContainer& sg = getFW().getSceneGraph();
   sg.getNMRoot()->whichChild.setValue(SO_SWITCH_NONE);
   sg.getNMVecRoot()->whichChild.setValue(SO_SWITCH_NONE);
   sg.getcsSwitch()->whichChild.setValue(SO_SWITCH_ALL);

   // Choosing Animation plays the selected mode; there is no Start button.
   if (p_active) start();
}

void NModePanel::showVectorMode()
{
   showStyle(false);
   p_scaleLabel->SetLabel(_("Arrow length:"));

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
  spectrum.setDifference(p_showDiff);
  if (p_expX.size() > 1) spectrum.setExperimental(p_expX, p_expY, p_expLabel);
  const int selected = p_spectrum->selected();
  const SpectrumAxis axis = p_spectrum->axis();
  p_spectrum->setSpectrum(spectrum);
  p_spectrum->setSelected(selected);
  if (axis.zoomed()) p_spectrum->zoomRange(axis.lo(), axis.hi());

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
    fillGraph();
    return true;
  }
  return false;
}


/** The other calculations in this one's folder that have IR or Raman data. */
vector<IPropCalculation*> NModePanel::comparableCalculations(bool needSpectrum)
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
    if (needSpectrum && other->getProperty("VIBIR") == 0 &&
        other->getProperty("VIBRAM") == 0)
      continue;
    if (!needSpectrum && other->getProperty("VIB") == 0) continue;
    out.push_back(other);
  }
  return out;
}


void NModePanel::compareDialog()
{
  wxBusyCursor busy;
  const vector<IPropCalculation*> spectra = comparableCalculations();
  const vector<IPropCalculation*> modes = comparableCalculations(false);
  if (spectra.empty() && modes.empty()) {
    wxMessageBox("No other calculation in this folder has IR, Raman or "
                 "normal-mode data.", "Compare", wxOK | wxICON_INFORMATION, this);
    return;
  }
  //  One list: each calculation once per thing that can be done with it.
  wxArrayString names;
  vector<std::pair<IPropCalculation*, bool>> what;   // calculation, overlay?
  names.Add("(none: this calculation only)");
  what.push_back({0, true});
  auto nameOf = [](IPropCalculation *c) {
    return wxString::FromUTF8(c->getURL().getFilePathTail().c_str());
  };
  for (IPropCalculation *c : spectra) {
    names.Add(nameOf(c) + "  (overlay spectrum)");
    what.push_back({c, true});
  }
  for (IPropCalculation *c : modes) {
    names.Add(nameOf(c) + "  (match modes)");
    what.push_back({c, false});
  }
  const int at = wxGetSingleChoiceIndex(
      "Draw another calculation's spectrum as a second curve, with the same "
      "scaling factor and line width, or match its bands with this one's "
      "modes:", "Compare", names, this);
  if (at < 0) return;
  if (at == 0) { compareWith(string()); return; }
  if (what[at].second)
    compareWith(what[at].first->getURL().getFilePathTail());
  else
    matchBandsDialog(what[at].first);
}


/** Structure, normal modes and IR intensities in ecce-bandshift's file form. */
bool NModePanel::writeModesFile(IPropCalculation *calc, const string& path,
                                wxString *error)
{
   PropVector *vec = calc ? (PropVector*) calc->getProperty("VIBFREQ") : 0;
   PropVector *ir = calc ? (PropVector*) calc->getProperty("VIBIR") : 0;
   PropVecTable *vib = calc ? dynamic_cast<PropVecTable*>(calc->getProperty("VIB")) : 0;
   Fragment frag;
   if (!vec || !vib || !calc->getFragment(frag)) {
      if (error) *error = "This calculation has no stored normal modes and structure.";
      return false;
   }
   const int nat = (int)frag.numAtoms(), nm = vec->rows();
   if (nat == 0 || vib->rows() != nat || vib->tables() != nm) {
      if (error) *error = "The normal modes do not match the structure.";
      return false;
   }
   std::ofstream f(path);
   f.precision(10);
   f << nat << "\n";
   for (int a = 0; a < nat; a++) {
      const double *x = frag.atomRef(a)->coordinates();
      f << frag.atomRef(a)->atomicSymbol() << " " << x[0] << " " << x[1]
        << " " << x[2] << "\n";
   }
   f << "modes " << nm << "\n";
   for (int m = 0; m < nm; m++) {
      f << vec->value(m) << "\n";
      for (int a = 0; a < nat; a++)
         f << vib->value(m, a, 0) << " " << vib->value(m, a, 1) << " "
           << vib->value(m, a, 2) << "\n";
   }
   if (ir && ir->rows() == nm) {
      f << "ir " << nm << "\n";
      for (int m = 0; m < nm; m++) f << ir->value(m) << "\n";
   }
   return (bool)f;
}


//  The arithmetic is ecce-bandshift's (scripts/ecce_bandshift.py).
bool NModePanel::matchBands(const string& otherName, const string& atoms,
                            wxString *error)
{
   IPropCalculation *self = getCalculation(), *other = 0;
   const vector<IPropCalculation*> found = comparableCalculations(false);
   for (size_t i = 0; i < found.size(); i++)
      if (found[i]->getURL().getFilePathTail() == otherName) other = found[i];
   wxString err;
   if (!self || !other) {
      if (error) *error = "No such calculation with stored normal modes.";
      return false;
   }
   namespace fs = std::filesystem;
   std::error_code ec;
   fs::path tmp = fs::temp_directory_path(ec) /
                  ("ecce-bandshift-" + std::to_string((long)wxGetProcessId()));
   fs::create_directories(tmp, ec);
   string text;
   int status = -1;
   if (writeModesFile(self, (tmp / "a").string(), error) &&
       writeModesFile(other, (tmp / "b").string(), error)) {
      const char *home = getenv("ECCE_HOME");
      string script = home && *home ? string(home) + "/bin/ecce-bandshift"
                                    : string("ecce-bandshift");
      string args = "'" + (tmp / "a").string() + "' '" + (tmp / "b").string() +
                    "' --lines" +
                    (atoms.empty() ? string() : " --atoms '" + atoms + "'");
#ifdef _WIN32
      fs::path py = fs::path(Ecce::ecceHome()) / "python" / "python.exe";
      string pyExe = fs::exists(py) ? py.string() : string("python.exe");
      string cmd = "\"" + pyExe + "\" \"" + script + "\" " + args;
#else
      string cmd = "'" + script + "' " + args + " 2>&1";
#endif
      status = Ecce::readCommand(cmd, text);
   } else {
      fs::remove_all(tmp, ec);
      return false;
   }
   fs::remove_all(tmp, ec);
   if (status != 0) {
      if (error) *error = wxString::FromUTF8(text.c_str());
      return false;
   }

   if (!p_bandFrame) {
      wxFrame *frame = new wxFrame(wxGetTopLevelParent(this), wxID_ANY,
                                   "Vibrational Frequencies: matched bands",
                                   wxDefaultPosition, wxSize(760, 420));
      wxPanel *pane = new wxPanel(frame);
      wxBoxSizer *top = new wxBoxSizer(wxVERTICAL);
      p_bandNote = new wxStaticText(pane, wxID_ANY, "");
      top->Add(p_bandNote, 0, wxEXPAND | wxALL, 6);
      p_bandGrid = new wxGrid(pane, wxID_ANY);
      p_bandGrid->CreateGrid(0, 8);
      p_bandGrid->EnableEditing(false);
      p_bandGrid->SetRowLabelSize(0);
      p_bandGrid->SetSelectionMode(wxGrid::wxGridSelectRows);
      const char *cols[] = {"Mode A", "cm-1 A", "IR A", "Mode B", "cm-1 B",
                            "IR B", "Shift", "Overlap"};
      for (int c = 0; c < 8; c++) p_bandGrid->SetColLabelValue(c, cols[c]);
      p_bandGrid->Bind(wxEVT_GRID_SELECT_CELL, [this](wxGridEvent& e) {
         long m = 0;
         if (e.GetRow() >= 0 && p_bandGrid->GetCellValue(e.GetRow(), 0).ToLong(&m) &&
             p_grid && m >= 1 && m <= p_grid->GetNumberRows())
            selectMode((int)m - 1);
         e.Skip();
      });
      top->Add(p_bandGrid, 1, wxEXPAND | wxALL, 6);
      pane->SetSizer(top);
      wxBoxSizer *fs2 = new wxBoxSizer(wxVERTICAL);
      fs2->Add(pane, 1, wxEXPAND);
      frame->SetSizer(fs2);
      p_bandFrame = frame;
   }
   p_bandFrame->SetTitle(wxString::FromUTF8(
      ("Vibrational Frequencies: bands of " + self->getURL().getFilePathTail() +
       " matched with " + otherName).c_str()));
   if (p_bandGrid->GetNumberRows() > 0)
      p_bandGrid->DeleteRows(0, p_bandGrid->GetNumberRows());
   wxString note, unmatched;
   std::istringstream in(text);
   string line;
   while (std::getline(in, line)) {
      if (!line.empty() && line.back() == '\r') line.pop_back();
      vector<string> f;
      std::stringstream ls(line);
      string cell;
      while (std::getline(ls, cell, '\t')) f.push_back(cell);
      if (f.size() >= 4 && f[0] == "header")
         note = wxString::Format("%s common atoms, fit RMSD %s A, overlap "
                                 "threshold %s. Shift is B minus A, A being "
                                 "this calculation.",
                                 f[1], f[2], f[3]);
      else if (f.size() == 9 && f[0] == "match") {
         const int r = p_bandGrid->GetNumberRows();
         p_bandGrid->AppendRows(1);
         auto fmt = [](const string& v, const char *pat) {
            return v == "-" ? wxString("-") : wxString::Format(pat, atof(v.c_str()));
         };
         p_bandGrid->SetCellValue(r, 0, f[1]);
         p_bandGrid->SetCellValue(r, 1, fmt(f[2], "%.1f"));
         p_bandGrid->SetCellValue(r, 2, fmt(f[3], "%.1f"));
         p_bandGrid->SetCellValue(r, 3, f[4]);
         p_bandGrid->SetCellValue(r, 4, fmt(f[5], "%.1f"));
         p_bandGrid->SetCellValue(r, 5, fmt(f[6], "%.1f"));
         p_bandGrid->SetCellValue(r, 6, wxString::Format("%+.1f", atof(f[7].c_str())));
         p_bandGrid->SetCellValue(r, 7, wxString::Format("%.3f", atof(f[8].c_str())));
         for (int c = 0; c < 8; c++)
            p_bandGrid->SetCellAlignment(r, c, wxALIGN_RIGHT, wxALIGN_CENTRE);
      } else if (f.size() == 6 && f[0] == "unmatched") {
         unmatched += wxString::Format("%s%s mode %s (%.1f cm-1)", unmatched.empty() ? "" : "; ",
                                       f[1], f[2], atof(f[3].c_str()));
      }
   }
   if (!unmatched.empty()) note += "\nUnmatched: " + unmatched + ".";
   p_bandNote->SetLabel(note);
   p_bandGrid->AutoSizeColumns();
   p_bandFrame->Layout();
   p_bandFrame->Show(true);
   p_bandFrame->Raise();
   return true;
}


wxString NModePanel::bandTableText() const
{
   wxString out;
   if (!p_bandGrid) return out;
   for (int r = 0; r < p_bandGrid->GetNumberRows(); r++) {
      for (int c = 0; c < p_bandGrid->GetNumberCols(); c++)
         out += (c ? "\t" : "") + p_bandGrid->GetCellValue(r, c);
      out += "\n";
   }
   return out;
}


int NModePanel::scriptBandClick(int row)
{
   if (!p_bandGrid || row < 0 || row >= p_bandGrid->GetNumberRows()) return -1;
   p_bandGrid->SelectRow(row);
   p_bandGrid->SetGridCursor(row, 0);
   wxGridEvent ev(wxID_ANY, wxEVT_GRID_SELECT_CELL, p_bandGrid, row, 0);
   p_bandGrid->GetEventHandler()->ProcessEvent(ev);
   return p_selectedRow + 1;
}


void NModePanel::setShowDifference(bool on)
{
   p_showDiff = on;
   fillGraph();
}


void NModePanel::zoomSpectrum(double lo, double hi)
{
   if (p_spectrum == 0) return;
   p_spectrum->zoomRange(lo, hi);
}


//  The reading, format detection and T -> A conversion are
//  ecce-spectrum's (scripts/ecce_spectrum.py), so the plot and the command
//  line cannot disagree.
bool NModePanel::importExperimental(const string& path, const string& kind,
                                    wxString *error)
{
   if (path.empty()) {
      p_expX.clear();
      p_expY.clear();
      p_expLabel.clear();
      fillGraph();
      return true;
   }
   const char *home = getenv("ECCE_HOME");
   string script = home && *home ? string(home) + "/bin/ecce-spectrum"
                                 : string("ecce-spectrum");
   string args = "--parse-exp '" + path + "' --exp-kind " + (kind.empty() ? "auto" : kind);
#ifdef _WIN32
   namespace fs = std::filesystem;
   fs::path py = fs::path(Ecce::ecceHome()) / "python" / "python.exe";
   string pyExe = fs::exists(py) ? py.string() : string("python.exe");
   string cmd = "\"" + pyExe + "\" \"" + script + "\" " + args;
#else
   string cmd = "'" + script + "' " + args + " 2>&1";
#endif
   string text;
   if (Ecce::readCommand(cmd, text) != 0) {
      if (error) *error = wxString::FromUTF8(text.c_str());
      return false;
   }
   vector<double> x, y;
   string detected, line;
   std::istringstream in(text);
   while (std::getline(in, line)) {
      if (line.compare(0, 5, "kind\t") == 0) { detected = line.substr(5); continue; }
      double a, b;
      if (sscanf(line.c_str(), "%lf %lf", &a, &b) == 2) { x.push_back(a); y.push_back(b); }
   }
   if (x.size() < 2) {
      if (error) *error = "No spectrum found in the file.";
      return false;
   }
   p_expX = x;
   p_expY = y;
   string name = path;
   const size_t at = name.find_last_of("/\\");
   if (at != string::npos) name = name.substr(at + 1);
   p_expLabel = name + " (" + detected + ")";
   fillGraph();
   return true;
}


void NModePanel::importExperimentalDialog()
{
   if (!p_expX.empty()) {
      const int r = wxMessageBox("Remove the experimental spectrum? No keeps "
                                 "it and lets you choose another file.",
                                 "Experimental spectrum",
                                 wxYES_NO | wxCANCEL | wxICON_QUESTION, this);
      if (r == wxCANCEL) return;
      if (r == wxYES) { importExperimental("", ""); return; }
   }
   wxFileDialog dlg(this, "Experimental spectrum (wavenumber, absorbance or "
                    "transmittance)", wxEmptyString, wxEmptyString,
                    "Spectrum files (*.csv;*.txt;*.dat)|*.csv;*.txt;*.dat|All files|*",
                    wxFD_OPEN | wxFD_FILE_MUST_EXIST);
   if (dlg.ShowModal() != wxID_OK) return;
   wxString err;
   if (!importExperimental((const char*)dlg.GetPath().utf8_str(), "auto", &err))
      wxMessageBox(err, "Experimental spectrum", wxOK | wxICON_ERROR, this);
}


void NModePanel::matchBandsDialog(IPropCalculation *other)
{
  string atoms;
  Fragment fa, fb;
  if (getCalculation()->getFragment(fa) && other->getFragment(fb) &&
      fa.numAtoms() != fb.numAtoms()) {
    wxString a = wxGetTextFromUser(
        wxString::Format("This calculation has %d atoms and the other %d. The "
                         "common atoms, this one's first, 1-based: for example "
                         "1-12:5-16 maps this calculation's atoms 1-12 to the "
                         "other's 5-16.", (int)fa.numAtoms(), (int)fb.numAtoms()),
        "Match bands: common atoms", "", this);
    if (a.empty()) return;
    atoms = (const char*)a.utf8_str();
  }
  wxString err;
  if (!matchBands(other->getURL().getFilePathTail(), atoms, &err))
    wxMessageBox(err, "Match bands", wxOK | wxICON_INFORMATION, this);
}


/**
 * Script hook: type text into the Freq. factor field and apply it.  Reports
 * the field, the scale in force, the sticks' wavenumbers and the line under
 * the plot.
 */
wxString NModePanel::scriptSpectrumFactor(const wxString& text)
{
   if (!p_isValid || p_spectrum == 0) return "no graph";
   wxTextCtrl *field = new wxTextCtrl(this, wxID_ANY, text);
   field->Hide();
   applySpectrumSettings(nullptr, field);
   const VibSpectrum& s = p_spectrum->spectrum();
   wxString out = wxString::Format("field '%s' scale %g sticks",
                                   field->GetValue(), s.scale());
   field->Destroy();
   for (int k = 0; k < 2; k++)
      if (s.has((VibKind)k)) {
         const vector<VibStick> st = s.sticks((VibKind)k);
         for (size_t i = 0; i < st.size(); i++)
            out += wxString::Format(" %d:%.4f", st[i].mode + 1, st[i].wavenumber);
      }
   wxString line;
   for (size_t i = 0; i < p_graphParts.size(); i++)
      line += (i ? " | " : "") + p_graphParts[i];
   return out + " detail '" + line + "'";
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
   if (path == 0 || p_spectrum == 0 || p_spectrumDumped) return;
   p_spectrumDumped = true;

   //  ECCE_SPECTRUM_COMPARE=<calculation>: the second curve before painting;
   //  ECCE_SPECTRUM_CSV=<path>: the CSV an export would write.
   const char *other = getenv("ECCE_SPECTRUM_COMPARE");
   const bool compared = other != 0 && compareWith(other);
   //  ECCE_SPECTRUM_EXPERIMENTAL=<file>[:<kind>], ECCE_SPECTRUM_DIFFERENCE=1
   //  and ECCE_SPECTRUM_ZOOM=<lo>,<hi> (#707) before the CSV and the paint.
   if (const char *e = getenv("ECCE_SPECTRUM_EXPERIMENTAL")) {
      string f = e, kind = "auto";
      const size_t colon = f.rfind(':');
      if (colon != string::npos && colon + 1 < f.size() &&
          (f.substr(colon + 1) == "absorbance" || f.substr(colon + 1) == "transmittance")) {
         kind = f.substr(colon + 1);
         f = f.substr(0, colon);
      }
      importExperimental(f, kind);
   }
   if (getenv("ECCE_SPECTRUM_DIFFERENCE") != 0) setShowDifference(true);
   if (const char *z = getenv("ECCE_SPECTRUM_ZOOM")) {
      double a, b;
      if (sscanf(z, "%lf,%lf", &a, &b) == 2) zoomSpectrum(a, b);
   }
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
      //  ECCE_SPECTRUM_REMOVE_COMPARE=1: remove the other spectra as the
      //  window's Remove does, and report whether there were any and the
      //  second curve before and after.
      if (other != 0 && getenv("ECCE_SPECTRUM_REMOVE_COMPARE") != 0) {
         auto second = [&]() {
            return (int)(p_spectrum->spectrum().compareSticks(VIB_IR).size() +
                         p_spectrum->spectrum().compareSticks(VIB_RAMAN).size());
         };
         fprintf(out, "remove-button before enabled %d compare-sticks %d\n",
                 hasOtherSpectra() ? 1 : 0, second());
         removeOtherSpectra();
         fprintf(out, "remove-button after enabled %d compare-sticks %d\n",
                 hasOtherSpectra() ? 1 : 0, second());
      }
      //  ECCE_SPECTRUM_WINDOW=1: open the pop-out and click in IT.
      SpectrumCanvas *clicker = p_spectrum;
      if (getenv("ECCE_SPECTRUM_WINDOW") != 0) {
         wxWindow *button = wxWindow::FindWindowByLabel("Open in window...", this);
         if (button) {
            wxCommandEvent event(wxEVT_BUTTON, button->GetId());
            event.SetEventObject(button);
            button->GetEventHandler()->ProcessEvent(event);
         }
         if (!p_spectrumPop) { fclose(out); return; }
         clicker = p_spectrumPop;
         wxBitmap shot(1000, 700, 24);
         wxMemoryDC wdc(shot);
         clicker->paintOnto(wdc, wxSize(1000, 700));
         wdc.SelectObject(wxNullBitmap);
      }
      const char *click = getenv("ECCE_SPECTRUM_CLICK");
      //  ECCE_SPECTRUM_HOVER=<mode>: the hover box's text for that mode.
      if (const char *hv = getenv("ECCE_SPECTRUM_HOVER")) {
         const int mode = atoi(hv) - 1;
         const VibKind kind = s.has(VIB_IR) ? VIB_IR : VIB_RAMAN;
         const std::vector<wxString> lines = p_spectrum->readoutLines(kind, mode);
         for (size_t i = 0; i < lines.size(); i++)
            fprintf(out, "hover %zu %s\n", i + 1, (const char*)lines[i].utf8_str());
      }
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
      //  The window as the user sees it, with its strip text and, for
      //  ECCE_SPECTRUM_HOVER, the hover box drawn at the selected stick.
      if (p_spectrumPop != 0) {
         wxBitmap shot(1000, 700, 24);
         wxMemoryDC wdc(shot);
         wxPoint at;
         const VibKind kind = s.has(VIB_IR) ? VIB_IR : VIB_RAMAN;
         const int sel = p_spectrumPop->selected();
         if (const char *hv = getenv("ECCE_SPECTRUM_HOVER")) {
            const int hm = atoi(hv) - 1;
            if (p_spectrumPop->stickPosition(kind, hm, &at))
               p_spectrumPop->setHover(kind, hm, wxPoint(at.x, at.y + 3));
         }
         p_spectrumPop->paintOnto(wdc, wxSize(1000, 700));
         wdc.SelectObject(wxNullBitmap);
         shot.SaveFile(wxString(path) + ".window.png", wxBITMAP_TYPE_PNG);
         fprintf(out, "strip selected %d\n", sel + 1);
         p_spectrumFrame->UpdateWindowUI(wxUPDATE_UI_RECURSE);
         const char *names[] = {"width", "factor", "low", "high", "direction", "sticks",
                                "difference", "reset", "experimental", "compare",
                                "remove", "export-csv"};
         for (const char *name : names) {
            wxWindow *control = wxWindow::FindWindowByName(wxString("spectrum-") + name,
                                                          p_spectrumFrame);
            fprintf(out, "setting %s %d\n", name, control && control->IsShownOnScreen());
         }
         // Exercise the buttons and their real dialogs without native UI automation.
         if (const char *target = getenv("ECCE_SPECTRUM_WINDOW_ACTIONS")) {
            struct Answers : wxModalDialogHook {
               wxString calculation;
               int choices = 0;
               int Enter(wxDialog *dialog) override {
                  if (auto *choice = dynamic_cast<wxSingleChoiceDialog*>(dialog)) {
                     for (wxWindow *child : choice->GetChildren()) {
                        auto *list = dynamic_cast<wxListBox*>(child);
                        if (!list) continue;
                        const int at = list->FindString(calculation + "  (overlay spectrum)");
                        if (at == wxNOT_FOUND) return wxID_CANCEL;
                        choice->SetSelection(at);
                        ++choices;
                        return wxID_OK;
                     }
                  }
                  return wxID_CANCEL;
               }
            } answers;
            answers.calculation = wxString::FromUTF8(target);
            answers.Register();
            auto button = [this](const char *name) {
               return wxWindow::FindWindowByName(wxString("spectrum-") + name,
                                                 p_spectrumFrame);
            };
            auto press = [](wxWindow *control, const wxString& path = wxString()) {
               if (!control || !control->IsEnabled()) return;
               wxCommandEvent event(wxEVT_BUTTON, control->GetId());
               event.SetEventObject(control);
               event.SetString(path);
               control->GetEventHandler()->ProcessEvent(event);
            };
            auto states = [&](const char *stage) {
               p_spectrumFrame->UpdateWindowUI(wxUPDATE_UI_RECURSE);
               const auto dock = p_spectrum->spectrum().compareSticks(VIB_IR);
               const auto pop = p_spectrumPop->spectrum().compareSticks(VIB_IR);
               bool same = dock.size() == pop.size();
               for (size_t i = 0; same && i < dock.size(); ++i)
                  same = dock[i].wavenumber == pop[i].wavenumber &&
                         dock[i].intensity == pop[i].intensity;
               fprintf(out, "window-compare %s dock %zu window %zu same %d remove %d/%d diff %d/%d\n",
                       stage, dock.size(), pop.size(), same,
                       p_compare != nullptr, button("remove")->IsEnabled(),
                       p_compare != nullptr, button("difference")->IsEnabled());
            };
            states("before");
            press(button("compare"));
            states("drawn");
            // Supply the destination through the command event so a headless
            // export uses the handler without the native file picker.
            press(button("export-csv"), wxString(path) + ".window.csv");
            press(wxWindow::FindWindowByLabel("Export .csv...", this),
                  wxString(path) + ".panel.csv");
            fprintf(out, "window-dialogs compare %d\n", answers.choices);
            press(button("remove"));
            states("removed");
            // Leave the comparison drawn for the window capture.
            press(button("compare"));
         }
         if (getenv("ECCE_SPECTRUM_SETTINGS")) {
            auto field = [this](const char *name) {
               return wxDynamicCast(wxWindow::FindWindowByName(
                  wxString("spectrum-") + name, p_spectrumFrame), wxTextCtrl);
            };
            auto command = [](wxWindow *control, wxEventType type, int value = 0) {
               wxCommandEvent event(type, control->GetId());
               event.SetEventObject(control);
               event.SetInt(value);
               control->GetEventHandler()->ProcessEvent(event);
            };
            const double oldWidth = s.fwhm(), oldScale = s.scale();
            field("width")->ChangeValue("23");
            field("factor")->ChangeValue("0.96");
            command(field("width"), wxEVT_TEXT_ENTER);
            UpdateWindowUI(wxUPDATE_UI_RECURSE);
            bool ok = s.fwhm() == 23 && s.scale() == 0.96;
            field("width")->ChangeValue("17");
            field("factor")->ChangeValue("1");
            command(field("width"), wxEVT_TEXT_ENTER);
            p_spectrumFrame->UpdateWindowUI(wxUPDATE_UI_RECURSE);
            ok = ok && s.fwhm() == 17 && s.scale() == 1;
            field("low")->ChangeValue("1000");
            field("high")->ChangeValue("2000");
            command(field("high"), wxEVT_TEXT_ENTER);
            UpdateWindowUI(wxUPDATE_UI_RECURSE);
            ok = ok && p_spectrum->axis().lo() == 1000 && p_spectrum->axis().hi() == 2000;
            for (const char *name : {"direction", "sticks"}) {
               wxWindow *control = wxWindow::FindWindowByName(wxString("spectrum-") + name,
                                                             p_spectrumFrame);
               command(control, wxEVT_CHECKBOX, 0);
            }
            ok = ok && !p_spectrum->reversed() && !p_spectrum->showSticks();
            //  A refresh for the difference curve must keep the selected mode and zoom.
            setShowDifference(true);
            ok = ok && p_spectrum->selected() == sel && p_spectrum->axis().lo() == 1000 &&
                 p_spectrum->axis().hi() == 2000;
            setShowDifference(false);
            wxWindow *reset = wxWindow::FindWindowByName("spectrum-reset", p_spectrumFrame);
            command(reset, wxEVT_BUTTON);
            ok = ok && !p_spectrum->axis().zoomed();
            fprintf(out, "settings-sync %d\n", ok);
            field("width")->ChangeValue(wxString::Format("%g", oldWidth));
            field("factor")->ChangeValue(wxString::Format("%g", oldScale));
            command(field("width"), wxEVT_TEXT_ENTER);
            p_spectrum->setReversed(true);
            p_spectrum->setShowSticks(true);
            wxMemoryDC updated(shot);
            p_spectrumPop->paintOnto(updated, wxSize(1000, 700));
            updated.SelectObject(wxNullBitmap);
            p_spectrum->setModeNotes({});
            wxMemoryDC unassigned(shot);
            p_spectrumPop->paintOnto(unassigned, wxSize(1000, 700));
            fprintf(out, "strip-unassigned %d\n", !p_spectrumPop->stripText().empty());
            std::vector<SpectrumModeNote> notes(p_vib.modes.size());
            for (size_t m = 0; m < notes.size(); ++m) {
               notes[m].label = p_vib.modes[m].label;
               notes[m].parts = p_vib.modes[m].parts;
               notes[m].irrep = p_vib.modes[m].irrep;
               notes[m].activity = std::string(activityText(p_vib.modes[m]).utf8_str());
            }
            p_spectrum->setModeNotes(notes);
            p_spectrumPop->paintOnto(unassigned, wxSize(1000, 700));
            unassigned.SelectObject(wxNullBitmap);
         }
         const std::vector<wxString>& st = p_spectrumPop->stripText();
         for (size_t i = 0; i < st.size(); i++)
            fprintf(out, "strip %zu %s\n", i, (const char*)st[i].utf8_str());
         const wxWeakRef<wxFrame> frame = p_spectrumFrame;
         const wxString framePath = wxString(path) + ".frame.png";
         CallAfter([frame, framePath]() {
            if (!frame) return;
            frame->Layout();
            frame->UpdateWindowUI(wxUPDATE_UI_RECURSE);
            frame->Refresh();
            frame->Update();
            ecceClientShot(frame, framePath);
         });
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
         {
            wxString tip = cellTip(r, p_modeColumn);
            tip.Replace("\n", "|");
            fprintf(out, "tip %d %s\n", r + 1, (const char*)tip.utf8_str());
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
      //  Whether the line sits in a bordered box that holds all of its text.
      if (p_modeBox && p_modeDetail)
        fprintf(out, "detailbox border %d inside %d textheight %d boxheight %d\n",
                (p_modeBox->GetWindowStyleFlag() & wxBORDER_SIMPLE) ? 1 : 0,
                p_modeDetail->GetParent() == p_modeBox ? 1 : 0,
                multiLineHeight(p_modeDetail),
                p_modeBox->GetSize().y);
      fclose(out);
      //  ECCE_NMODE_TABLE_DRAG=<dx>: drag the Mode column's right edge dx px
      //  with a real (X) mouse drag on the label row, then write the column
      //  widths and the Mode cells to <path>.drag.
      const char *drag = getenv("ECCE_NMODE_TABLE_DRAG");
      if (drag != 0 && p_modeColumn >= 0) {
         const int dx = atoi(drag);
         wxWindow *lw = p_grid->GetGridColLabelWindow();
         int edge = 0;
         for (int c = 0; c <= p_modeColumn; c++) edge += p_grid->GetColSize(c);
         const wxPoint from = lw->ClientToScreen(wxPoint(edge - 1, lw->GetSize().y / 2));
         wxUIActionSimulator sim;
         sim.MouseMove(from); wxYield();
         //  A tooltip may be up by the time a user presses.
         for (int i = 0; i < 20; i++) { wxMilliSleep(100); wxYield(); }
         sim.MouseDown(); wxYield(); wxMilliSleep(150);
         for (int i = 1; i <= 5; i++) {
            sim.MouseMove(from.x + dx * i / 5, from.y);
            wxYield(); wxMilliSleep(100);
         }
         sim.MouseUp(); wxYield(); wxMilliSleep(300);
         wxYield();
         if (FILE *d = fopen((target + ".drag").c_str(), "w")) {
            fprintf(d, "edge %d dx %d\n", edge, dx);
            for (int c = 0; c < p_grid->GetNumberCols(); c++)
               fprintf(d, "col %d %d\n", c, p_grid->GetColSize(c));
            fprintf(d, "modeuserwidth %d\n", p_modeUserWidth);
            for (int r = 0; r < p_grid->GetNumberRows() && r < 10; r++)
               fprintf(d, "cell %d %s\n", r + 1,
                       (const char*)p_grid->GetCellValue(r, p_modeColumn).utf8_str());
            fclose(d);
         }
      }
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
      std::vector<SpectrumModeNote> notes(p_vib.modes.size());
      for (size_t m = 0; m < notes.size(); m++) {
         notes[m].label = p_vib.modes[m].label;
         notes[m].irrep = p_vib.modes[m].irrep;
         notes[m].activity = std::string(activityText(p_vib.modes[m]).utf8_str());
         notes[m].parts = p_vib.modes[m].parts;
      }
      if (p_spectrum) p_spectrum->setModeNotes(notes);

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
         //  One decimal for a wavenumber, two for an intensity: that is all
         //  the methods give.
         p_grid->SetCellValue(idx,0, wxString::Format("%.1f", vec->value(idx)));

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
                  wxString::Format("%.2f", ivec->value(idx)));
         }

         if (rvec != (PropVector*)0 && p_ramanColumn >= 0) {
            p_grid->SetCellValue(idx,p_ramanColumn,
                  wxString::Format("%.2f", rvec->value(idx)));
         }

         //  An imaginary mode (stored as a negative wavenumber) decides
         //  whether the structure is a minimum or a transition state, so
         //  its whole row stands out.
         if (VibFrequency::isImaginary(vec->value(idx)))
            for (int c = 0; c < p_grid->GetNumberCols(); c++)
               p_grid->SetCellTextColour(idx, c, imaginaryColour());
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


//  The arithmetic is ecce-displace's (scripts/ecce_displace.py); this writes
//  its input, runs it and puts the answer into copies of the calculation.
string NModePanel::displaceCalculation(int mode, double amplitude,
                                       const string& sign, vector<string> *urls)
{
   IPropCalculation *calc = getCalculation();
   Resource *res = dynamic_cast<Resource*>(calc);
   PropVector *vec = calc ? (PropVector*) calc->getProperty("VIBFREQ") : 0;
   PropVecTable *vib = calc ? dynamic_cast<PropVecTable*>(calc->getProperty("VIB")) : 0;
   Fragment frag;
   if (!res || !vec || !vib || !calc->getFragment(frag))
      return "This calculation has no stored normal modes and structure.";
   const int nat = (int)frag.numAtoms(), nm = vec->rows();
   if (nat == 0 || vib->rows() != nat || vib->tables() != nm)
      return "The normal modes do not match the structure.";
   if (mode >= nm) return "No such mode.";
   if (!(amplitude > 0)) return "The amplitude must be positive.";

   namespace fs = std::filesystem;
   std::error_code ec;
   fs::path tmp = fs::temp_directory_path(ec) /
                  ("ecce-displace-" + std::to_string((long)wxGetProcessId()));
   fs::create_directories(tmp, ec);
   {
      std::ofstream f((tmp / "modes").string());
      f.precision(10);
      f << nat << "\n";
      for (int a = 0; a < nat; a++) {
         const double *x = frag.atomRef(a)->coordinates();
         f << frag.atomRef(a)->atomicSymbol() << " " << x[0] << " " << x[1]
           << " " << x[2] << "\n";
      }
      f << "modes " << nm << "\n";
      for (int m = 0; m < nm; m++) {
         f << vec->value(m) << "\n";
         for (int a = 0; a < nat; a++)
            f << vib->value(m, a, 0) << " " << vib->value(m, a, 1) << " "
              << vib->value(m, a, 2) << "\n";
      }
   }
   const char *home = getenv("ECCE_HOME");
   string script = home && *home ? string(home) + "/bin/ecce-displace"
                                 : string("ecce-displace");
   std::ostringstream amp;
   amp << amplitude;
   string args = "'" + (tmp / "modes").string() + "' " +
                 (mode < 0 ? string("--imaginary") : "--mode " + std::to_string(mode + 1)) +
                 " --amplitude " + amp.str() + " --sign '" + sign + "' --source '" +
                 res->getName() + "' --lines";
#ifdef _WIN32
   fs::path py = fs::path(Ecce::ecceHome()) / "python" / "python.exe";
   string pyExe = fs::exists(py) ? py.string() : string("python.exe");
   string cmd = "\"" + pyExe + "\" \"" + script + "\" " + args;
#else
   string cmd = "'" + script + "' " + args + " 2>/dev/null";
#endif
   string text;
   int status = Ecce::readCommand(cmd, text);
   fs::remove_all(tmp, ec);
   if (status != 0)
      return mode < 0 ? "This calculation has no imaginary mode."
                      : "The displacement could not be computed.";

   std::istringstream in(text);
   string line;
   vector<string> out;
   while (std::getline(in, line)) {
      if (!line.empty() && line.back() == '\r') line.pop_back();
      if (line.compare(0, 5, "note ") != 0) continue;
      const string note = line.substr(5);
      vector<double> xyz;
      for (int a = 0; a < nat && std::getline(in, line); a++) {
         std::istringstream ls(line);
         double x, y, z;
         ls >> x >> y >> z;
         xyz.push_back(x); xyz.push_back(y); xyz.push_back(z);
      }
      if ((int)xyz.size() != 3*nat) return "The displacement could not be read.";
      EcceURL url = res->getURL();
      if (!res->copy(url)) return "The calculation could not be copied: " + res->messages();
      Resource *made = EDSIFactory::getResource(url);
      PropertyTask *pt = dynamic_cast<PropertyTask*>(made);
      ICalculation *ic = dynamic_cast<ICalculation*>(made);
      TaskJob *job = dynamic_cast<TaskJob*>(made);
      Fragment *nf = pt ? pt->fragment() : 0;
      if (!nf || !ic || !job || (int)nf->numAtoms() != nat) {
         if (made) made->remove();
         return "The copy has no structure to displace.";
      }
      for (int a = 0; a < nat; a++)
         nf->atomRef(a)->coordinates(xyz[3*a], xyz[3*a+1], xyz[3*a+2]);
      pt->fragment(nf);
      TRunType rt;
      rt.name("Geometry");
      ic->runtype(rt);
      //  Settings of the frequency step make no sense for an optimisation.
      if (GUIValues *g = ic->guiparams()) {
         g->deletePrefix("ES.Runtype.Vibration.");
         ic->guiparams(g);
         delete g;
      }
      job->resetForRerun(ResourceDescriptor::STATE_CREATED);
      made->addProp(VDoc::getEcceNamespace() + ":annotation", note);
      EDSIFactory::notifyCreate(url.toString(), false);
      out.push_back(url.toString());
   }
   if (out.empty()) return "The displacement could not be read.";
   if (urls) *urls = out;
   return "";
}


void NModePanel::displaceDialog()
{
   wxDialog dlg(this, wxID_ANY, "Displace along a mode");
   wxBoxSizer *top = new wxBoxSizer(wxVERTICAL);
   wxArrayString what, sign;
   what.Add("Selected mode");
   what.Add("All imaginary modes");
   sign.Add("+");
   sign.Add("-");
   sign.Add("Both (two calculations)");
   wxRadioBox *w = new wxRadioBox(&dlg, wxID_ANY, "Along", wxDefaultPosition,
                                  wxDefaultSize, what, 1);
   wxRadioBox *sg = new wxRadioBox(&dlg, wxID_ANY, "Direction",
                                   wxDefaultPosition, wxDefaultSize, sign, 1);
   wxTextCtrl *amp = new wxTextCtrl(&dlg, wxID_ANY, "0.10");
   top->Add(w, 0, wxALL | wxEXPAND, 6);
   top->Add(sg, 0, wxALL | wxEXPAND, 6);
   wxBoxSizer *row = new wxBoxSizer(wxHORIZONTAL);
   row->Add(new wxStaticText(&dlg, wxID_ANY, "Largest atom displacement (\u00c5)"),
            0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
   row->Add(amp, 1);
   top->Add(row, 0, wxALL | wxEXPAND, 6);
   top->Add(dlg.CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxALL | wxEXPAND, 6);
   dlg.SetSizerAndFit(top);
   if (dlg.ShowModal() != wxID_OK) return;
   double a = 0;
   if (!amp->GetValue().ToDouble(&a)) a = -1;
   const char *signs[] = {"+", "-", "both"};
   vector<string> urls;
   string err = displaceCalculation(w->GetSelection() == 0 ? p_mode : -1, a,
                                    signs[sg->GetSelection()], &urls);
   wxString msg = err.empty()
      ? wxString::Format("Created %d new calculation(s) next to this one.", (int)urls.size())
      : wxString::FromUTF8(err);
   wxMessageBox(msg, "Displace along a mode", wxOK | wxICON_INFORMATION, this);
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
   if (p_modeUserWidth > 0) width = std::max(p_modeUserWidth, minWidth);
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
      wxString t = wxString::FromUTF8(v.label);
      if (!v.detail.empty()) t += (t.empty() ? "" : "\n") + wxString::FromUTF8(v.detail);
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


//  All the text, broken between words at the label's width and never cut:
//  the label takes as many lines as it needs and asks its box for them.
void NModePanel::wrapAll(wxStaticText *label, const std::vector<wxString>& parts)
{
   if (label == 0) return;
   wxString full;
   for (size_t i = 0; i < parts.size(); i++)
      full += (i ? wxString::FromUTF8(" \xC2\xB7 ") : wxString()) + parts[i];
   wrapText(label, full);
}

/// Wraps each paragraph (lines of `full` split at \n) to the label's width.
void NModePanel::wrapText(wxStaticText *label, const wxString& full)
{
   if (label == 0) return;
   label->SetToolTip(full.empty() ? NULL : new wxToolTip(full));
   int width = label->GetClientSize().x;
   if (const char *w = getenv("ECCE_NMODE_DETAIL_WIDTH")) width = atoi(w);
   wxClientDC dc(label);
   dc.SetFont(label->GetFont());
   wxString shown, line;
   int lines = 1;
   wxStringTokenizer paras(full, "\n", wxTOKEN_RET_EMPTY_ALL);
   bool firstPara = true;
   while (paras.HasMoreTokens()) {
      if (!firstPara) { shown += line + "\n"; line.clear(); lines++; }
      firstPara = false;
      wxStringTokenizer words(paras.GetNextToken(), " ");
      while (words.HasMoreTokens()) {
         const wxString word = words.GetNextToken();
         const wxString t = line.empty() ? word : line + " " + word;
         if (!line.empty() && width > 40 && dc.GetTextExtent(t).x > width) {
            shown += line + "\n";
            line = word;
            lines++;
         } else {
            line = t;
         }
      }
   }
   shown += line;
   if (label->GetLabel() != shown) label->SetLabel(shown);
   const int h = lines * dc.GetCharHeight() + 2;
   if (label->GetMinSize().y != h) {
      label->SetMinSize(wxSize(50, h));
      //  A layout inside this size event would run before the label knows
      //  its new height.
      CallAfter([this]() { if (p_graphBox) p_graphBox->GetParent()->Layout(); });
   }
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
      const double sc = p_spectrum != 0 ? p_spectrum->spectrum().scale() : 1.0;
      if (sc != 1.0 && cell.ToCDouble(&f))
         freq += wxString::Format(" (scaled %.1f)", f * sc);
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
   //  Under the plot: line 1 names the mode with the intensities that
   //  exist, line 2 the contributions.
   wxString l1, l2;
   if (!freq.empty() && row >= 0 && row < (int)p_vib.modes.size()) {
      const wxString dot = wxString::FromUTF8(" \xC2\xB7 ");
      const VibModeInfo& v = p_vib.modes[row];
      l1 = wxString::Format("Mode %d", row + 1) + dot + freq;
      wxString sym = p_symColumn >= 0 && p_symColumn < p_grid->GetNumberCols()
                        ? p_grid->GetCellValue(row, p_symColumn) : wxString();
      if (sym.empty()) sym = wxString::FromUTF8(v.irrep.c_str());
      if (!sym.empty()) l1 += dot + sym;
      if (p_spectrum != 0 && !p_spectrum->spectrum().electronic())
         for (int k = 0; k < 2; k++) {
            const VibSpectrum& sp = p_spectrum->spectrum();
            if (!sp.has((VibKind)k)) continue;
            for (const VibStick& st : sp.sticks((VibKind)k))
               if (st.mode == row)
                  l1 += dot + (k == VIB_IR ? "IR " : "Raman ") +
                        wxString::Format("%.1f ", st.intensity) +
                        wxString::FromUTF8(sp.unitLabel((VibKind)k).c_str());
         }
      for (const auto& part : v.parts)
         l2 += (l2.empty() ? "" : "  ") + wxString::FromUTF8(part.first.c_str()) +
               wxString::Format(" %.0f %%", part.second);
      if (!l2.empty()) l2 += "  (others < 5 %)";
   }
   p_graphText = l2.empty() ? l1 : l1 + "\n" + l2;
   wrapText(p_graphDetail, p_graphText);
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

// Larger arrows or amplitudes are absurd; a typed or stored value is cut to
// the slider's range instead of reaching the commands.
double NModePanel::sliderValue()
{
   double value = p_slider->GetFloatValue();
   const double top = maxScale();
   if (value < 0.0 || value > top) {
      value = value < 0.0 ? 0.0 : top;
      p_slider->SetValue(value);
   }
   return value;
}

double NModePanel::typeScale(double value)
{
   p_slider->SetValue(value);
   wxScrollEvent end;
   OnEndSliderMotion(end);
   return p_slider->GetFloatValue();
}

double NModePanel::maxScale()
{
   wxRadioBox *radbox = (wxRadioBox*)FindWindow(ID_RADIOBOX_NMODE_VIZTYPE);
   return radbox && radbox->GetSelection() == 0 ? MAX_AMPLITUDE : MAX_SCALE;
}

void NModePanel::setSlider()
{
   wxRadioBox *radbox = (wxRadioBox*)FindWindow(ID_RADIOBOX_NMODE_VIZTYPE);
   const double v = radbox->GetSelection() == 0 ? p_aniAmplitude
                                                : p_vecAmplitude;
   const double top = maxScale();
   p_slider->SetRange(0.0, top);
   p_slider->SetValue(v > top ? top : v);
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
   double value = sliderValue();

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

      updateVectors(p_mode, sliderValue(), sign, bgcolor.ToStdString());

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

   updateVectors(p_mode, sliderValue(), sign, color.c_str());
   */
   updateVectors();

   event.Skip();
}

void NModePanel::OnRadioboxSelected( wxCommandEvent& event )
{
   wxRadioBox *radbox = (wxRadioBox*)FindWindow(ID_RADIOBOX_NMODE_VIZTYPE);
   p_lastRadioSel = radbox->GetSelection();
   syncViewControls();
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
      syncViewControls();
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

   double value = sliderValue();

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


//  ------------------------------------------------------------------
//  Mode composition (#486)
//  ------------------------------------------------------------------

bool NModePanel::compositionRows(bool chBends, VibComposition& comp,
                                 vector<double>& freqs, wxString *error)
{
   auto fail = [&](const wxString& why) { if (error) *error = why; return false; };
   IPropCalculation *expt = getCalculation();
   PropVector *vec = expt ? (PropVector*) expt->getProperty("VIBFREQ") : 0;
   PropVecTable *vib = expt ? dynamic_cast<PropVecTable*>(expt->getProperty("VIB")) : 0;
   if (vec == 0 || vib == 0) return fail("This calculation has no normal modes.");
   Fragment frag;
   if (!expt->getFragment(frag)) return fail("The calculation's structure is not available.");
   const int nat = (int)frag.numAtoms(), nm = vec->rows();
   if (nat == 0 || vib->rows() != nat || vib->tables() != nm)
      return fail("The modes do not belong to the calculation's structure.");
   vector< vector<double> > modes(nm, vector<double>(3*nat));
   for (int m = 0; m < nm; m++) {
      freqs.push_back(vec->value(m));
      for (int a = 0; a < nat; a++)
         for (int k = 0; k < 3; k++) modes[m][3*a+k] = vib->value(m, a, k);
   }
   VibInput in;
   if (!VibAnalysis::input(frag, freqs, modes, in)) return fail("No structure.");
   p_compLabels.clear();
   for (size_t m = 0; m < p_vib.modes.size(); m++) p_compLabels.push_back(p_vib.modes[m].label);

   //  Torsions are in the automatic set because the table's description
   //  (VibAnalysis::describe) has them; without them a torsion is spread
   //  over bends and the two views name different motions.
   if (!VibAnalysis::composition(in, vector<VibCoordSpec>(), vector<int>(), comp, true, chBends))
      return fail("No internal coordinates could be built for this structure.");
   return true;
}


bool NModePanel::showComposition(int chBends, wxString *error)
{
   bool ch = true;
   ewxConfig::getConfig(INIFILE)->Read("NMode/CompositionCHBends", &ch, true);
   if (chBends >= 0) ch = chBends != 0;
   VibComposition comp;
   vector<double> freqs;
   if (!compositionRows(ch, comp, freqs, error)) return false;
   if (!p_compFrame) {
      wxFrame *frame = new wxFrame(wxGetTopLevelParent(this), wxID_ANY,
                                   "Vibrational Frequencies: mode composition",
                                   wxDefaultPosition, wxSize(820, 560));
      wxPanel *pane = new wxPanel(frame);
      wxBoxSizer *top = new wxBoxSizer(wxVERTICAL);
      wxStaticText *intro = new wxStaticText(pane, wxID_ANY,
         "Share of each mode along bond stretches, angle bends and torsions, "
         "normalised to 100 % over this set. Indicative, not a potential "
         "energy distribution.");
      intro->Wrap(780);
      top->Add(intro, 0, wxEXPAND | wxALL, 6);
      wxBoxSizer *row = new wxBoxSizer(wxHORIZONTAL);
      p_compCH = new wxCheckBox(pane, wxID_ANY, wxT("Include C\u2013H bends"));
      p_compCH->SetToolTip("Add the bends that move a hydrogen on carbon "
                           "(H-C-X and H-C-H) to the set of coordinates.");
      row->Add(p_compCH, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);
      wxButton *copy = new wxButton(pane, wxID_ANY, "Copy table");
      copy->SetToolTip("Copy the table to the clipboard, tab-separated.");
      row->Add(copy, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
      wxButton *exp = new wxButton(pane, wxID_ANY, "Export .csv...");
      row->Add(exp, 0, wxALIGN_CENTER_VERTICAL);
      top->Add(row, 0, wxEXPAND | wxLEFT | wxRIGHT, 6);
      p_compNote = new wxStaticText(pane, wxID_ANY, "");
      top->Add(p_compNote, 0, wxEXPAND | wxALL, 6);
      p_compGrid = new wxGrid(pane, wxID_ANY);
      p_compGrid->CreateGrid(0, 4);
      p_compGrid->EnableEditing(false);
      p_compGrid->SetRowLabelSize(0);
      //  The last column follows the window; the columns must not size it.
      p_compGrid->SetMinSize(wxSize(200, 100));
      p_compGrid->SetColLabelValue(0, "Mode");
      p_compGrid->SetColLabelValue(1, "cm-1");
      p_compGrid->SetColLabelValue(2, "Mode type");
      p_compGrid->SetColLabelValue(3, "Largest contributions");
      p_compGrid->GetGridWindow()->Bind(wxEVT_MOTION, [this](wxMouseEvent& e) {
         int x, y;
         p_compGrid->CalcUnscrolledPosition(e.GetX(), e.GetY(), &x, &y);
         const int r = p_compGrid->YToRow(y);
         const int c = p_compGrid->XToCol(x);
         wxString tip;
         if (r >= 0 && c >= 2) tip = p_compGrid->GetCellValue(r, c);
         wxWindow *win = p_compGrid->GetGridWindow();
         if (win->GetToolTipText() != tip) win->SetToolTip(tip.empty() ? NULL : new wxToolTip(tip));
         e.Skip();
      });
      p_compGrid->Bind(wxEVT_SIZE, [this](wxSizeEvent& e) { fitCompColumn(); e.Skip(); });
      top->Add(p_compGrid, 1, wxEXPAND | wxALL, 6);
      pane->SetSizer(top);
      wxBoxSizer *fs = new wxBoxSizer(wxVERTICAL);
      fs->Add(pane, 1, wxEXPAND);
      frame->SetSizer(fs);
      p_compFrame = frame;
      p_compCH->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) {
         ewxConfig::getConfig(INIFILE)->Write("NMode/CompositionCHBends", p_compCH->GetValue());
         wxString err;
         if (!showComposition(p_compCH->GetValue() ? 1 : 0, &err)) p_compNote->SetLabel(err);
      });
      copy->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
         if (wxTheClipboard->Open()) {
            wxTheClipboard->SetData(new wxTextDataObject(compositionText('\t')));
            wxTheClipboard->Close();
         }
      });
      exp->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
         wxFileDialog dlg(p_compFrame, "Export mode composition", "", "mode-composition.csv",
                          "CSV files (*.csv)|*.csv", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
         if (dlg.ShowModal() != wxID_OK) return;
         wxFile f(dlg.GetPath(), wxFile::write);
         if (!f.IsOpened() || !f.Write(compositionText(','), wxConvUTF8))
            wxMessageBox("The file could not be written.", "Mode composition",
                         wxOK | wxICON_ERROR, p_compFrame);
      });
   }
   p_compCH->SetValue(ch);
   p_comp = comp;
   p_compFreqs = freqs;
   fillComposition();
   p_compFrame->Show(true);
   p_compFrame->Raise();
   return true;
}


void NModePanel::fillComposition()
{
   if (!p_compGrid) return;
   if (p_compGrid->GetNumberRows() > 0)
      p_compGrid->DeleteRows(0, p_compGrid->GetNumberRows());
   for (size_t m = 0; m < p_comp.percent.size(); m++) {
      if (VibFrequency::isZero(p_compFreqs[m])) continue;   // translations, rotations
      vector<size_t> order(p_comp.names.size());
      for (size_t i = 0; i < order.size(); i++) order[i] = i;
      const vector<double>& pc = p_comp.percent[m];
      std::sort(order.begin(), order.end(),
                [&](size_t a, size_t b) { return pc[a] > pc[b]; });
      wxString text;
      //  On a large molecule every share can be below 5 %, so the top three
      //  are shown whatever their size; the figure says how small they are.
      for (size_t i = 0; i < order.size() && i < 4; i++) {
         if (i >= 3 && pc[order[i]] < 5.0) break;
         if (pc[order[i]] <= 0.0) break;
         if (!text.empty()) text += "; ";
         text += wxString::FromUTF8(p_comp.names[order[i]].c_str());
         text += wxString::Format(" %s %%", pc[order[i]] < 1.0 ? "<1" : wxString::Format("%.0f", pc[order[i]]));
      }
      const int r = p_compGrid->GetNumberRows();
      p_compGrid->AppendRows(1);
      p_compGrid->SetCellValue(r, 0, wxString::Format("%d", (int)m + 1));
      p_compGrid->SetCellValue(r, 1, wxString::Format("%.1f", p_compFreqs[m]));
      p_compGrid->SetCellValue(r, 2, m < p_compLabels.size()
                               ? wxString::FromUTF8(p_compLabels[m].c_str()) : wxString());
      p_compGrid->SetCellValue(r, 3, text);
      if (VibFrequency::isImaginary(p_compFreqs[m]))
         for (int c = 0; c < p_compGrid->GetNumberCols(); c++)
            p_compGrid->SetCellTextColour(r, c, imaginaryColour());
   }
   p_compGrid->SetColSize(0, 50);
   p_compGrid->SetColSize(1, 70);
   wxClientDC dc(p_compGrid->GetGridWindow());
   dc.SetFont(p_compGrid->GetDefaultCellFont());
   int labelW = dc.GetTextExtent("Mode type").x;
   for (int r = 0; r < p_compGrid->GetNumberRows(); r++)
      labelW = std::max(labelW, dc.GetTextExtent(p_compGrid->GetCellValue(r, 2)).x);
   p_compGrid->SetColSize(2, labelW + 16);
   fitCompColumn();
}


//  The last column takes what the other three leave, and follows the window.
void NModePanel::fitCompColumn()
{
   if (!p_compGrid || p_compGrid->GetNumberCols() < 4) return;
   int others = 0;
   for (int c = 0; c < 3; c++) others += p_compGrid->GetColSize(c);
   const int scroll = wxSystemSettings::GetMetric(wxSYS_VSCROLL_X, p_compGrid);
   p_compGrid->SetColSize(3, std::max(120, p_compGrid->GetClientSize().x - others - scroll - 2));
}


//  The table as the Copy and Export buttons give it; csv quotes the cells.
wxString NModePanel::compositionText(char sep) const
{
   auto cell = [sep](const wxString& v) {
      if (sep != ',') return v;
      wxString q = v;
      q.Replace("\"", "\"\"");
      return "\"" + q + "\"";
   };
   wxString out;
   for (int c = 0; c < p_compGrid->GetNumberCols(); c++)
      out += (c ? wxString(sep) : wxString()) +
             cell(c == 1 ? wxString(wxT("cm-1")) : p_compGrid->GetColLabelValue(c));
   out += "\n";
   for (int r = 0; r < p_compGrid->GetNumberRows(); r++) {
      for (int c = 0; c < p_compGrid->GetNumberCols(); c++)
         out += (c ? wxString(sep) : wxString()) + cell(p_compGrid->GetCellValue(r, c));
      out += "\n";
   }
   return out;
}


bool NModePanel::scriptComposition(int chBends, const wxString& textPath,
                                   const wxString& pngPath)
{
   wxString err;
   if (!showComposition(chBends, &err)) return false;
   FILE *out = fopen(textPath.utf8_str(), "w");
   if (out == 0) return false;
   fprintf(out, "coords");
   for (size_t i = 0; i < p_comp.names.size(); i++)
      fprintf(out, "\t%s", p_comp.names[i].c_str());
   fprintf(out, "\n");
   for (size_t m = 0; m < p_comp.percent.size(); m++) {
      fprintf(out, "%zu\t%.2f", m + 1, p_compFreqs[m]);
      for (size_t i = 0; i < p_comp.percent[m].size(); i++)
         fprintf(out, "\t%.2f", p_comp.percent[m][i]);
      fprintf(out, "\n");
   }
   fclose(out);
   //  ECCE_NMODE_COMPOSITION_GRID=<path>: the grid's column widths and
   //  cells; ECCE_NMODE_COMPOSITION_RESIZE=<w> first sets the window's
   //  width, so a growing window can be compared with the one before.
   static bool gridWritten = false;
   if (const char *gp = getenv("ECCE_NMODE_COMPOSITION_GRID")) {
      FILE *g = gridWritten ? 0 : fopen(gp, "w");
      gridWritten = true;
      if (g) {
         auto widths = [&](const char *tag) {
            for (int i = 0; i < 6; i++) { wxYield(); wxMilliSleep(150); }
            p_compFrame->Layout();
            wxYield();
            fprintf(g, "%s frame %d min %d grid %d cols", tag, p_compFrame->GetSize().x,
                    p_compFrame->GetEffectiveMinSize().x, p_compGrid->GetClientSize().x);
            for (int c = 0; c < p_compGrid->GetNumberCols(); c++)
               fprintf(g, " %d", p_compGrid->GetColSize(c));
            fprintf(g, "\n");
         };
         if (getenv("ECCE_NMODE_COMPOSITION_RESIZE"))
            p_compFrame->SetSize(820, p_compFrame->GetSize().y);
         widths("before");
         if (const char *w = getenv("ECCE_NMODE_COMPOSITION_RESIZE")) {
            p_compFrame->SetSize(atoi(w), p_compFrame->GetSize().y);
            widths("after");
         }
         fprintf(g, "ch\t%d\n", p_compCH && p_compCH->GetValue() ? 1 : 0);
         for (int r = 0; r < p_compGrid->GetNumberRows(); r++) {
            fprintf(g, "row");
            for (int c = 0; c < p_compGrid->GetNumberCols(); c++)
               fprintf(g, "\t%s", (const char*)p_compGrid->GetCellValue(r, c).utf8_str());
            fprintf(g, "\n");
         }
         fclose(g);
      }
   }
   //  ECCE_NMODE_COMPOSITION_COPY=<path>: the text the Copy button puts on
   //  the clipboard; <path>.csv is what Export writes.
   if (const char *cp = getenv("ECCE_NMODE_COMPOSITION_COPY")) {
      for (int pass = 0; pass < 2; pass++) {
         FILE *g = fopen((string(cp) + (pass ? ".csv" : "")).c_str(), "w");
         if (!g) continue;
         fputs((const char*)compositionText(pass ? ',' : '\t').utf8_str(), g);
         fclose(g);
      }
   }
   if (!pngPath.empty()) {
      p_compFrame->Layout();
      p_compFrame->Update();
      const wxSize sz = p_compFrame->GetClientSize();
      wxBitmap bmp(sz.x, sz.y, 24);
      wxMemoryDC mdc(bmp);
      wxYield();
      wxScreenDC cdc;
      const wxPoint at = p_compFrame->ClientToScreen(wxPoint(0, 0));
      mdc.Blit(0, 0, sz.x, sz.y, &cdc, at.x, at.y);
      mdc.SelectObject(wxNullBitmap);
      bmp.SaveFile(pngPath, wxBITMAP_TYPE_PNG);
   }
   return true;
}


/**
 * ECCE_NMODE_COMPOSITION_DUMP=<path> (#486): once the panel is up, open the
 * Mode composition window, write its numbers to <path> and its picture to
 * <path>.png.  ECCE_NMODE_COMPOSITION_CH sets the C-H bends checkbox.
 */
void NModePanel::dumpCompositionIfRequested()
{
   //  ECCE_NMODE_HIGHLIGHT=<path>, ECCE_NMODE_HIGHLIGHT_ATOMS="2 3" (1-based):
   //  colour the spectrum by those atoms as a viewer selection would, and
   //  write the key text and the spectrum to <path> and <path>.png.
   if (const char *hp = getenv("ECCE_NMODE_HIGHLIGHT")) {
      const string target(hp);
      const string atomText = getenv("ECCE_NMODE_HIGHLIGHT_ATOMS") ? getenv("ECCE_NMODE_HIGHLIGHT_ATOMS") : "";
      wxTimer *timer = new wxTimer();
      timer->Bind(wxEVT_TIMER, [this, target, atomText](wxTimerEvent&) {
         std::vector<int> atoms;
         std::istringstream is(atomText);
         int a;
         while (is >> a) atoms.push_back(a - 1);
         const std::string key = highlightAtoms(atoms);
         p_picked = atoms;
         if (FILE *out = fopen(target.c_str(), "w")) {
            fprintf(out, "key %s\n", key.c_str());
            for (size_t m = 0; m < p_spectrum->highlight().size(); m++)
               fprintf(out, "%zu\t%.2f\n", m + 1, p_spectrum->highlight()[m]);
            fclose(out);
         }
         wxBitmap shot(1000, 700, 24);
         wxMemoryDC mdc(shot);
         p_spectrum->paintOnto(mdc, wxSize(1000, 700));
         mdc.SelectObject(wxNullBitmap);
         shot.SaveFile(wxString::FromUTF8((target + ".png").c_str()), wxBITMAP_TYPE_PNG);
         if (getenv("ECCE_EXIT_AFTER_DUMP") != 0)
            wxTheApp->CallAfter([]() {
               wxWindow *top = wxTheApp->GetTopWindow();
               if (top != 0) top->Close(true);
            });
      });
      timer->StartOnce(2500);
   }
   const char *path = getenv("ECCE_NMODE_COMPOSITION_DUMP");
   if (path == 0) return;
   const string target(path);
   //  ECCE_NMODE_COMPOSITION_CH=0/1 sets the C-H bends checkbox.
   const char *chEnv = getenv("ECCE_NMODE_COMPOSITION_CH");
   const int ch = chEnv ? atoi(chEnv) : -1;
   wxTimer *timer = new wxTimer();
   timer->Bind(wxEVT_TIMER, [this, target, ch](wxTimerEvent&) {
      scriptComposition(ch, wxString::FromUTF8(target.c_str()),
                        wxString::FromUTF8((target + ".png").c_str()));
      if (getenv("ECCE_EXIT_AFTER_DUMP") != 0)
         wxTheApp->CallAfter([]() {
            wxWindow *top = wxTheApp->GetTopWindow();
            if (top != 0) top->Close(true);
         });
   });
   timer->StartOnce(2500);
}


std::string NModePanel::highlightAtoms(const std::vector<int>& atoms)
{
   std::vector<double> share;
   std::string label;
   if (atoms.size() >= 2) {
      VibComposition comp;
      vector<double> freqs;
      IPropCalculation *expt = getCalculation();
      PropVector *vec = expt ? (PropVector*) expt->getProperty("VIBFREQ") : 0;
      PropVecTable *vib = expt ? dynamic_cast<PropVecTable*>(expt->getProperty("VIB")) : 0;
      Fragment frag;
      if (vec && vib && expt->getFragment(frag) &&
          vib->rows() == (int)frag.numAtoms() && vib->tables() == vec->rows()) {
         const int nat = (int)frag.numAtoms(), nm = vec->rows();
         vector< vector<double> > modes(nm, vector<double>(3*nat));
         for (int m = 0; m < nm; m++) {
            freqs.push_back(vec->value(m));
            for (int a = 0; a < nat; a++)
               for (int k = 0; k < 3; k++) modes[m][3*a+k] = vib->value(m, a, k);
         }
         VibInput in;
         if (VibAnalysis::input(frag, freqs, modes, in))
            VibAnalysis::selectionShare(in, atoms, share, label);
      }
   }
   if (p_spectrum) p_spectrum->setHighlight(share, label);
   if (p_spectrumPop) p_spectrumPop->setHighlight(share, label);
   return share.empty() ? std::string() : label;
}


void NModePanel::eventMCB(const ecce::Event& event)
{
   if (event.getName() != "SelectionChanged" || !p_active || !p_structureOk) return;
   SGFragment *frag = getFW().getSceneGraph().getFragment();
   if (!frag) return;
   std::vector<int> picked;
   for (size_t i = 0; i < frag->m_atomHighLight.size(); i++) {
      const int a = frag->m_atomHighLight[i];
      if (a >= 0 && a < (int)frag->numAtoms() && frag->atomRef(a)->atomicSymbol() != "Nub")
         picked.push_back(a);
   }
   p_picked = picked;
   highlightAtoms(picked);
}
