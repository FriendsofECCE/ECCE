#include <iostream>
  using std::cout;
  using std::cerr;
  using std::endl;
#include <limits>

#include <wx/link.h>

#include "util/PreferenceLabels.H"
#include "util/Spectrum.H"

#include "tdat/PropVector.H"
#include "tdat/TProperty.H"

#include "dsm/PropFactory.H"
#include "dsm/ICalculation.H"

#include "wxgui/ewxConfig.H"
#include "wxgui/ewxColor.H"
#include "wxgui/ewxGrid.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/WxColorSchemeButton.H"

#include "inv/ChemKit/ChemColor.H"

#include "viz/CSLoadColorsCmd.H"

#include "wxviz/WxVizToolFW.H"

#include "VecAtomSpectrum.H"

  using std::find;

//TODO
// Support labeling of selected atoms (low)
// Support log scale toggle
// Resize the available width (not sure if it can be done)


static const char *INIFILE = "wxbuilder.ini";


// ----------------------------------------------------------------------------
static const int gslices = 20;


BEGIN_EVENT_TABLE( VecAtomSpectrum, VizPropertyPanel )
   EVT_GRID_RANGE_SELECT(VecAtomSpectrum::OnGridRangeSelect)
END_EVENT_TABLE()


IMPLEMENT_DYNAMIC_CLASS(VecAtomSpectrum, VizPropertyPanel)


VecAtomSpectrum::VecAtomSpectrum()
  : VizPropertyPanel(),
    TearableContentProvider(),
    p_grid(NULL),
    p_spectrum(NULL),
    p_minLabel(NULL),
    p_maxLabel(NULL),
    p_internalSelect(false),
    p_spectrumRange(0),
    p_defRed(0),
    p_defGreen(0),
    p_defBlue(0),
    p_red(0),
    p_green(0),
    p_blue(0)
{
   subscribe();
   p_colorTglId = wxNewId();
   p_LogScaleTglId = wxNewId();
}


VecAtomSpectrum::VecAtomSpectrum(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id,
        const wxPoint& pos, const wxSize& size, long style,
        const wxString& name)
  : VizPropertyPanel(),
    TearableContentProvider(),
    p_grid(NULL),
    p_spectrum(NULL),
    p_minLabel(NULL),
    p_maxLabel(NULL),
    p_internalSelect(false),
    p_spectrumRange(0),
    p_defRed(0),
    p_defGreen(0),
    p_defBlue(0),
    p_red(0),
    p_green(0),
    p_blue(0)
{
   Create(calculation, parent, id, pos, size, style, name);
   subscribe();
   p_colorTglId = wxNewId();
   p_LogScaleTglId = wxNewId();
}


VecAtomSpectrum::~VecAtomSpectrum()
{
   clean();

   if (p_defRed) { delete [] p_defRed; p_defRed = 0; }
   if (p_defGreen) { delete [] p_defGreen; p_defGreen = 0; }
   if (p_defBlue) { delete [] p_defBlue; p_defBlue = 0; }
   unsubscribe();

   GetEventHandler()->Disconnect(p_colorTglId, wxEVT_COMMAND_TOOL_CLICKED,
            wxCommandEventHandler(VecAtomSpectrum::OnColorAtomClick));
   GetEventHandler()->Disconnect(p_LogScaleTglId, wxEVT_COMMAND_TOOL_CLICKED,
            wxCommandEventHandler(VecAtomSpectrum::OnLogScaleClick));
}


void VecAtomSpectrum::clean()
{
   if (p_red) { delete [] p_red; p_red = 0; }
   if (p_green) { delete [] p_green; p_green = 0; }
   if (p_blue) { delete [] p_blue; p_blue = 0; }


}

string VecAtomSpectrum::getConfigKey()
{
  string ret;
  set<string> keys = getPropertyNames();
  if (keys.size() > 0) {
     ret = "Spectrum/" + *keys.begin();
  }
  return ret;
}


bool VecAtomSpectrum::Create(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id,
        const wxPoint& pos, const wxSize& size, long style,
        const wxString& name)
{
   if (!VizPropertyPanel::Create(calculation, parent, id, pos, size, style,
            name)) {
      wxFAIL_MSG( wxT("VecAtomSpectrum creation failed") );
      return false;
   }

   if (!GetSizer())
      SetSizer(new wxBoxSizer(wxVERTICAL));

   //
   // Create simple gui with a spectrum and a table 
   //

   // Sizer for spectrum
   wxSizer *spectrumSizer = new wxBoxSizer(wxHORIZONTAL);
   GetSizer()->Add(spectrumSizer,0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

   p_spectrum = new WxColorSchemeButton(this, -1, wxDefaultPosition, wxSize(100,20));
   fillSpectrumColors();
   p_minLabel = new ewxStaticText(this, wxID_ANY, "-10.0");
   spectrumSizer->Add(p_minLabel,0,wxALIGN_CENTER_VERTICAL,1);
   spectrumSizer->Add(p_spectrum, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);
   p_maxLabel = new ewxStaticText(this, wxID_ANY, "10.0");
   spectrumSizer->Add(p_maxLabel,0,wxALIGN_CENTER_VERTICAL,1);


   wxSizer *gridSizer = new wxBoxSizer(wxHORIZONTAL);
   //GetSizer()->Add(treeSizer,0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);
   GetSizer()->Add(gridSizer,1, wxGROW|wxALL, 0);

   p_grid = new ewxGrid( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxHSCROLL|wxVSCROLL );
   p_grid->SetDefaultColSize(50);
   p_grid->SetDefaultRowSize(25);
   p_grid->SetColLabelSize(0);
   p_grid->SetRowLabelSize(0);
   p_grid->SetMinSize(wxSize(200,175));
   p_grid->CreateGrid(3, 3, wxGrid::wxGridSelectRows);
   p_grid->EnableEditing(false);
   gridSizer->Add(p_grid, 1, wxGROW|wxALL, 3);

   fillUI();
   p_grid->ClearSelection();

   GetEventHandler()->Connect(p_colorTglId, wxEVT_COMMAND_TOOL_CLICKED,
            wxCommandEventHandler(VecAtomSpectrum::OnColorAtomClick));
   GetEventHandler()->Connect(p_LogScaleTglId, wxEVT_COMMAND_TOOL_CLICKED,
            wxCommandEventHandler(VecAtomSpectrum::OnLogScaleClick));


   GetSizer()->Fit(this);
   GetSizer()->SetSizeHints(this);

   return true;
}



void VecAtomSpectrum::fillUI()
{

  // TODO delete rows /columns
  p_grid->ClearGrid();
  p_grid->DeleteCols(0,p_grid->GetNumberCols());
  p_grid->DeleteRows(0,p_grid->GetNumberRows());

  // TODO insert labels

  IPropCalculation *calc = getCalculation();

  // Get the main property
  string key = *(getPropertyNames().begin());
  TProperty *prop = (TProperty*) calc->getProperty(key);
  PropVector *vec = dynamic_cast<PropVector*>(prop);
  if (vec != (PropVector*)0) {

     // Get unit information
     const Property_Ref *propRef = PropFactory::getPropRef(vec->type());
     string unit;
     if (propRef != (Property_Ref*)0) {
        unit = vec->units();
        if (unit != "") unit = "\n" + unit;
     }
     int nRows = vec->rows();

     SGFragment *frag = getFW().getSceneGraph().getFragment() ;
     if (frag) {
        double value;
        p_grid->InsertRows(0,nRows);
        p_grid->InsertCols(0,3);
        double min = std::numeric_limits<double>::max();
        double max = std::numeric_limits<double>::min();

        int nPos=0, nNeg=0;

        wxString cellv;
        for (int idx=0; idx<nRows; idx++) {
           value = vec->value(idx);
           if (value > max) max = value;
           if (value < min) min = value;
           if (value < 0) nNeg++;
           if (value > 0) nPos++;

           cellv = wxString::Format (_T("%d"), idx);
           p_grid->SetCellValue(idx, 0, cellv);

           cellv = frag->atomRef(idx)->atomicSymbol().c_str();
           p_grid->SetCellValue(idx, 1, cellv);

           cellv = wxString::Format (_T(PrefLabels::DOUBLEFORMAT), value);
           p_grid->SetCellValue(idx, 2, cellv);

        }

        // Sometimes especially with two atoms, we found the charges on both
        // atoms to be the same.  Sometimes they were both 0 and it crashed.
        // So here we try to do something simple to ensure that min
        // and max are never the same.
        if (min == max) {
           if (min == 0.) {
              max = 1.0;
           } else {
              if (max < 0. ) {
                 max = 0.;
              } else {
                 min = 0.;
              }
           }
        }

        if (nNeg > 0 && nPos > 0)
           p_spectrumRange = 0;
        else if (nNeg > 0)
           p_spectrumRange = -1;
        else
           p_spectrumRange = 1;

        fillSpectrumColors();
        setSpectrumRange(min,max);
        computeColors(vec, min, max);

     }
   }
   p_grid->AutoSize();
}

void VecAtomSpectrum::eventMCB(const Event& event)
{
   string name = event.getName();
   string value = event.getValue();
   if (name == "SelectionChanged") {
      if (!p_internalSelect) {
         SGFragment *frag = getFW().getSceneGraph().getFragment();
         int numSel = frag->m_atomHighLight.size();
         p_internalSelect = true;
         p_grid->ClearSelection();
         for (int idx=0; idx<numSel; idx++) {
            p_grid->SelectRow(frag->m_atomHighLight[idx],true);
         }
         p_internalSelect = false;
      }
   }
}


/**
 * Process a range selection.
 * CTRL selects come in one per row making this a bit tricky.
 * Bizarre selection patterns typically seems to result in a unselect of the
 * full range followed by a select of the new range making it easier.
 */
void VecAtomSpectrum::OnGridRangeSelect(wxGridRangeSelectEvent& event)
{
   if (p_internalSelect) return;

   SGFragment *frag = getFW().getSceneGraph().getFragment();
   vector<int> bonds;

   //cout << "VecAtomSpectrum selection" << event.GetTopRow() << "," << event.GetBottomRow() << " " << event.Selecting() << frag->m_atomHighLight.size() << endl;
   if (event.Selecting()) {
      if (event.ControlDown()) {
         // add to selection - top and bottom should be the same
         frag->m_atomHighLight.push_back(event.GetTopRow());
      } else {
         // a row or possible a block of rows
         frag->m_atomHighLight.clear();
         for (int idx=event.GetTopRow(); idx<=event.GetBottomRow(); idx++) {
            frag->m_atomHighLight.push_back(idx);
         }
      }
      p_internalSelect = true;
      selectCommand(frag->m_atomHighLight,bonds);
      p_internalSelect = false;
   } else {
      // Unselect - rebuild the vector minus the deselection atoms
      vector<int> atoms;
      for (int idx=frag->m_atomHighLight.size()-1; idx>=0; idx--) {
         int curatom = frag->m_atomHighLight[idx];
         if (curatom < event.GetTopRow() || curatom > event.GetBottomRow()) {
            atoms.push_back(curatom);
         }
      }
      if (atoms.size() != frag->m_atomHighLight.size()) {
         p_internalSelect = true;
         selectCommand(atoms,bonds);
         p_internalSelect = false;
      }
   }
   event.Skip();
}


void VecAtomSpectrum::setSpectrumRange(double min,double max)
{

   p_minLabel->SetLabel(wxString::Format (_T("% .3lf"), min));
   p_maxLabel->SetLabel(wxString::Format (_T("% .3lf"), max));
}

void VecAtomSpectrum::fillSpectrumColors()
{
   int max = 100;
   int step = max/gslices;
   vector<wxColour> colors;
   Spectrum s(1, max, gslices);
   float r,g,b;
   for (int x=1; x<max; x+=step) {
      if (p_spectrumRange == 0) {
         s.getPosNegMidpointValue(x, r, g, b);
      } else if (p_spectrumRange < 0) {
         s.getNegValue(x, r, g, b);
      } else if (p_spectrumRange > 0) {
         s.getPosValue(x, r, g, b);
      } else {
         s.getPosNegMidpointValue(x, r, g, b);
      }
      colors.push_back(ewxColor(r, g, b));
   }

   p_spectrum->setColors(colors);
}

void VecAtomSpectrum::computeColors(const PropVector *vec, double min, double max)
{
   clean();

   if (vec) {
      double val;

      int nRows = vec->rows();
      p_red = new float[nRows];
      p_green = new float[nRows];
      p_blue = new float[nRows];

      bool useLog;
      ewxConfig *config = ewxConfig::getConfig(INIFILE);
      config->Read((getConfigKey()+"/Log").c_str(),&useLog, false);


      Spectrum s(min, max, gslices);
      float red,green,blue;
      for (int idx=0; idx<nRows; idx++) {
         val = vec->value(idx);
         if (useLog) val = log10(val);

         s.getPosNegMidpointValue(val, red, green, blue);
         if (p_spectrumRange == 0) {
            s.getPosNegMidpointValue(val, red, green, blue);
         } else if (p_spectrumRange < 0) {
            s.getNegValue(val, red, green, blue);
         } else if (p_spectrumRange > 0) {
            s.getPosValue(val, red, green, blue);
         } else {
            s.getPosNegMidpointValue(val, red, green, blue);
         }

         p_red[idx] = red;
         p_green[idx] = green;
         p_blue[idx] = blue;

      }
   }

}


/**
 * Pull in existing color scheme so we can restore it later when we concede
 * control.
 */
void VecAtomSpectrum::saveColorTable()
{
   if (p_defRed == (float*)0) {
      p_defRed = new float[::TPTnumAtoms];
      p_defGreen = new float[::TPTnumAtoms];
      p_defBlue = new float[::TPTnumAtoms];

      ChemColor *colors = getFW().getSceneGraph().getColorNode();
      for (int idx=0; idx<TPTnumAtoms; idx++) {
         const SbColor& cvec = colors->atomColor[idx];
         p_defRed[idx] = cvec[0];
         p_defGreen[idx] = cvec[1];
         p_defBlue[idx] = cvec[2];
      }
   }
}


void VecAtomSpectrum::restoreColorTable()
{
   if (p_defRed) {
      WxVizToolFW& fw = getFW();
      Command *cmd = new CSLoadColorsCmd("Restore Color", &fw.getSceneGraph());

      for (int idx=0; idx<TPTnumAtoms; idx++) {
         cmd->getParameter("atomIdx")->setInteger(idx);
         cmd->getParameter("red")->setDouble(p_defRed[idx]);
         cmd->getParameter("green")->setDouble(p_defGreen[idx]);
         cmd->getParameter("blue")->setDouble(p_defBlue[idx]);
         fw.execute(cmd);
      }

      fw.getSceneGraph().getColorNode()->atomColorBinding.setValue(
            ChemColor::ATOM_PER_ATOM_INDEXED);
   }
}


void VecAtomSpectrum::setColorTable()
{
   WxVizToolFW& fw = getFW();
   Command *cmd = new CSLoadColorsCmd("Set Color", &fw.getSceneGraph());


   SGFragment *frag = getFW().getSceneGraph().getFragment() ;
   int nAtoms = frag->numAtoms();
   for (int idx=0; idx<nAtoms; idx++) {
      cmd->getParameter("atomIdx")->setInteger(idx);
      cmd->getParameter("red")->setDouble(p_red[idx]);
      cmd->getParameter("green")->setDouble(p_green[idx]);
      cmd->getParameter("blue")->setDouble(p_blue[idx]);
      fw.execute(cmd);
   }
   fw.getSceneGraph().getColorNode()->atomColorBinding.setValue(ChemColor::ATOM_PER_ATOM);
}




void VecAtomSpectrum::receiveFocus()
{
  saveColorTable();

  bool showColor;
  ewxConfig *config = ewxConfig::getConfig(INIFILE);
  config->Read((getConfigKey()+"/ShowColors").c_str(),&showColor,true);
  if (showColor) setColorTable();

}


void VecAtomSpectrum::loseFocus()
{
  bool showColor;
  ewxConfig *config = ewxConfig::getConfig(INIFILE);
  config->Read((getConfigKey()+"/ShowColors").c_str(),&showColor,true);

  if (showColor) restoreColorTable();
}


wxWindow* VecAtomSpectrum::GetTearableContent()
{
  //return new ewxPanel(this);
  wxToolBar *tb = new wxToolBar(this, wxID_ANY,
          wxDefaultPosition, wxDefaultSize,
          wxTB_VERTICAL|wxNO_BORDER|wxTB_TEXT|wxTB_NOICONS,
          "Spectrum Property Menu");

  tb->AddCheckTool(p_colorTglId, "Color Atoms", wxNullBitmap, wxNullBitmap);
  tb->AddCheckTool(p_LogScaleTglId, "Color Using Log Scale", wxNullBitmap, wxNullBitmap);

  ewxConfig *config = ewxConfig::getConfig(INIFILE);
  bool showColor;
  config->Read((getConfigKey()+"/ShowColors").c_str(),&showColor,true);
  tb->ToggleTool(p_colorTglId, showColor);

  if (p_spectrumRange > 0) {
     // We don't let them pick the log option if numbers < 0
     bool useLog;
     config->Read((getConfigKey()+"/Log").c_str(),&useLog, false);
     tb->ToggleTool(p_LogScaleTglId, useLog);
  }

  tb->Realize();

  return tb;
}


void VecAtomSpectrum::OnColorAtomClick(wxCommandEvent& event)
{
   ewxConfig *config = ewxConfig::getConfig(INIFILE);
   config->Write((getConfigKey()+"/ShowColors").c_str(), event.IsChecked());

   if (event.IsChecked()) {
     setColorTable();
   } else { 
     restoreColorTable();
   }

   event.Skip();
}

void VecAtomSpectrum::OnLogScaleClick(wxCommandEvent& event)
{
   ewxConfig *config = ewxConfig::getConfig(INIFILE);
   config->Write((getConfigKey()+"/Log").c_str(), event.IsChecked());
   fillUI();

   event.Skip();
}
