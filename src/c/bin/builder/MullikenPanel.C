#include <iostream>
  using std::cout;
  using std::cerr;
  using std::endl;
#include <limits>

#include <wx/link.h>

#include "util/InternalException.H"
#include "util/PreferenceLabels.H"
#include "util/Spectrum.H"
#include "util/NullPointerException.H"
#include "util/EventDispatcher.H"
#include "util/Event.H"
  using namespace ecce;

#include "tdat/PropVector.H"

#include "dsm/TGBSConfig.H"
#include "dsm/ICalculation.H"
#include "dsm/PropFactory.H"

#include "wxgui/ewxColor.H"
#include "wxgui/ewxButton.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxMessageDialog.H"
#include "wxgui/TreeListCtrl.H"
#include "wxgui/WxColorSchemeButton.H"

#include "inv/ChemKit/ChemColor.H"

#include "viz/CSLoadColorsCmd.H"

#include "wxviz/WxVizToolFW.H"

#include "MullikenPanel.H"

  using std::find;

//TODO
// Support labeling of selected atoms (low)
// Support units in the widget - could not do "Value\nunit"
// Support flexible width columns



/**
 * Small data class to keep track of the atom index associated with each
 * tree item.
 */
class AtomIndex : public wxTreeItemData 
{
   public:
      int p_index;
      AtomIndex(int index) : wxTreeItemData(), p_index(index) {;} 
};


// ----------------------------------------------------------------------------
static const int gslices = 20;


const wxWindowID MullikenPanel::IDTREELIST = wxNewId();


BEGIN_EVENT_TABLE( MullikenPanel, VizPropertyPanel )
    EVT_TREE_SEL_CHANGED(MullikenPanel::IDTREELIST,
                         MullikenPanel::OnTreeSelChanged)
END_EVENT_TABLE()


wxFORCE_LINK_THIS_MODULE(MullikenPanel)


IMPLEMENT_DYNAMIC_CLASS(MullikenPanel, VizPropertyPanel)


MullikenPanel::MullikenPanel()
  : VizPropertyPanel(),
    p_treelist(NULL),
    p_spectrum(NULL),
    p_minLabel(NULL),
    p_maxLabel(NULL),
    p_defRed(0),
    p_defGreen(0),
    p_defBlue(0),
    p_red(0),
    p_green(0),
    p_blue(0),
    p_internalSelect(false)
{
   subscribe();
}


MullikenPanel::MullikenPanel(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id,
        const wxPoint& pos, const wxSize& size, long style,
        const wxString& name)
  : VizPropertyPanel(),
    p_treelist(NULL),
    p_spectrum(NULL),
    p_minLabel(NULL),
    p_maxLabel(NULL),
    p_defRed(0),
    p_defGreen(0),
    p_defBlue(0),
    p_red(0),
    p_green(0),
    p_blue(0),
    p_internalSelect(false)
{
  Create(calculation, parent, id, pos, size, style, name);
  subscribe();
}


MullikenPanel::~MullikenPanel()
{
   unsubscribe();
   clean();

   if (p_defRed) { delete [] p_defRed; p_defRed = 0; }
   if (p_defGreen) { delete [] p_defGreen; p_defGreen = 0; }
   if (p_defBlue) { delete [] p_defBlue; p_defBlue = 0; }
}


void MullikenPanel::clean()
{
   if (p_red) { delete [] p_red; p_red = 0; }
   if (p_green) { delete [] p_green; p_green = 0; }
   if (p_blue) { delete [] p_blue; p_blue = 0; }


}


bool MullikenPanel::Create(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id,
        const wxPoint& pos, const wxSize& size, long style,
        const wxString& name)
{
  if (!VizPropertyPanel::Create(calculation, parent, id, pos, size, style,
                                name)) {
    wxFAIL_MSG( wxT("MullikenPanel creation failed") );
    return false;
  }

  if (!GetSizer())
    SetSizer(new wxBoxSizer(wxVERTICAL));

  //
  // Create simple gui with a spectrum and a tree control
  //

  // Sizer for spectrum
  wxSizer *spectrumSizer = new wxBoxSizer(wxHORIZONTAL);
  GetSizer()->Add(spectrumSizer,0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

  int max = 100;
  int step = max/gslices;
  vector<wxColour> colors;
  Spectrum s(1, max, gslices);
  float r,g,b;
  for (int x=1; x<max; x+=step) {
     s.getPosNegMidpointValue(x, r, g, b);
     colors.push_back(ewxColor(r, g, b));
  }

  p_spectrum = new WxColorSchemeButton(this, -1, wxDefaultPosition, wxSize(100,20));
  p_spectrum->addColors(colors);
  p_minLabel = new ewxStaticText(this, wxID_ANY, "-10.0");
  spectrumSizer->Add(p_minLabel,0,wxALIGN_CENTER_VERTICAL,1);
  spectrumSizer->Add(p_spectrum, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);
  p_maxLabel = new ewxStaticText(this, wxID_ANY, "10.0");
  spectrumSizer->Add(p_maxLabel,0,wxALIGN_CENTER_VERTICAL,1);


  wxSizer *treeSizer = new wxBoxSizer(wxHORIZONTAL);
  //GetSizer()->Add(treeSizer,0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);
  GetSizer()->Add(treeSizer,1, wxEXPAND|wxGROW|wxALL, 0);

  p_treelist = new wxTreeListCtrl (this, IDTREELIST, wxDefaultPosition, wxDefaultSize, wxTR_TWIST_BUTTONS|wxTR_NO_LINES|wxTR_HIDE_ROOT|wxTR_FULL_ROW_HIGHLIGHT|wxTR_EXTENDED);
  treeSizer->Add(p_treelist, 1,wxEXPAND|wxGROW|wxALL,1);

  // Create the columns
  p_treelist->AddColumn (_T(""), 12, wxALIGN_LEFT);
  p_treelist->SetColumnEditable (0, false);
  p_treelist->AddColumn (_T("#"), 25, wxALIGN_LEFT);
  p_treelist->SetColumnEditable (1, false);
  p_treelist->SetColumnAlignment (1, wxALIGN_LEFT);
  p_treelist->AddColumn (_T("Atom"), 40, wxALIGN_LEFT);
  p_treelist->SetColumnEditable (2, false);
  p_treelist->SetColumnAlignment (1, wxALIGN_LEFT);
  p_treelist->AddColumn (_T(""), 100, wxALIGN_LEFT);
  p_treelist->SetColumnEditable (2, false);
  p_treelist->SetColumnAlignment (2, wxALIGN_LEFT);
  p_treelist->AddColumn (_T("Value"), 100, wxALIGN_CENTER);
  p_treelist->SetColumnEditable (3, false);
  p_treelist->SetColumnAlignment (3, wxALIGN_CENTER);

  // Now the options to choose spectrum and assign charges
  wxSizer *optionsSizer = new wxBoxSizer(wxHORIZONTAL);
  GetSizer()->Add(optionsSizer,0, wxGROW|wxALL, 0);
  optionsSizer->AddSpacer(5);
  p_assign = new ewxButton(this,-1,"Assign Charges");
  optionsSizer->Add(p_assign,0,wxGROW|wxALL,0);

  fillUI();

  GetEventHandler()->Connect(p_assign->GetId(), wxEVT_COMMAND_BUTTON_CLICKED,
           wxCommandEventHandler(MullikenPanel::OnAssign));

  GetSizer()->Fit(this);
  GetSizer()->SetSizeHints(this);

  return true;
}

void MullikenPanel::eventMCB(const Event& event)
{
   string name = event.getName();
   string value = event.getValue();
   if (name == "SelectionChanged") {
      p_internalSelect = true;
      p_treelist->UnselectAll();
      wxTreeItemId root, item;
      wxTreeItemIdValue cookie;

      WxVizToolFW& fw = getFW();
      SGFragment *frag = fw.getSceneGraph().getFragment();

      root = p_treelist->GetRootItem();
      int row = 0;
      int index;
      wxString value;
      item = p_treelist->GetFirstChild(root,cookie);
      int numSel = frag->m_atomHighLight.size();
      // Horribly inefficient nested loop but should be small data sets
      for (; item.IsOk(); item=p_treelist->GetNextChild(root, cookie)) {
         value = p_treelist->GetItemText (item, 1);
         // Atom entries have a number in this column
         if (value != "") {
            sscanf(value.c_str(),"%d",&index);
            index--;  // count from 0
            for (int adx=0; adx<numSel; adx++) {
               if (frag->m_atomHighLight[adx] == index) {
                  // Sadly, the flag that should force addition to the
                  // selection doesn't seem to work so we only get one...
                  p_treelist->SelectItem(item); //,NULL,false);
                  p_treelist->EnsureVisible(item);
                  break;
               }
            }
            row++;
         }
      }

      p_internalSelect = false;
   }
}

void MullikenPanel::fillUI()
{

  p_treelist->DeleteRoot();        // Clear the tree

  wxTreeItemId root = p_treelist->AddRoot (_T("Root"));

  static const char *shells[] = {"s","p","d","f","g","h","i"};

  IPropCalculation *calc = getCalculation();
  ICalculation *icalc = dynamic_cast<ICalculation*>(calc);
  INTERNALEXCEPTION(icalc, "Cannot down cast to ICalucation");


  double min = std::numeric_limits<double>::max();
  double max = std::numeric_limits<double>::min();
  // Get the main property
  PropVector *vec =  (PropVector*) calc->getProperty("MULLIKEN");
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

        // Get the shell charges and basis info if the shel charges exist
        PropVector *svec =  (PropVector*) calc->getProperty("MLKNSHELL");
        TGBSConfig *config = (TGBSConfig*)0;
        if (svec) {
           // Need the basis set too
           config = icalc->gbsConfig();
           // The following are for the rare cases where we have data but for
           // some reason (import) not a valid basis set.
           if (config && config->isEmpty()) config = 0;
        }


        string sym, symLabel;
        int sidx=0;   // shell counter
        wxTreeItemId myparent;
        wxTreeItemId item;
        int currow = 0;
        for (int idx=0; idx<nRows; idx++) {             // loop over atoms
           sym = frag->atomRef(idx)->atomicSymbol();
           symLabel = sym;
            // Treat atom as Bq if it is a fixed point charge
           if (frag->atomRef(idx)->getBehaviorType() == TAtm::POINT) {
              symLabel = sym;
              sym = "Bq";
              symLabel = sym + symLabel;
           }

           // This part does the total charge on an atom
           item = p_treelist->AppendItem (root, wxString::Format (_T("%d"), currow++),-1,-1,new AtomIndex(idx));
           double value = vec->value(idx);
           if (value > max) max = value;
           if (value < min) min = value;
           p_treelist->SetItemText (item, 1, wxString::Format (_T("%d"), idx+1));
           p_treelist->SetItemText (item, 2, symLabel.c_str());
           p_treelist->SetItemText (item, 3, "total atomic");
           p_treelist->SetItemText (item, 4, wxString::Format (_T(PrefLabels::DOUBLEFORMAT), value));


           // This part does shell charges
           // must have the gbsconfig to display the shell charges
           if (config) {
              myparent = item;
              vector<const TGaussianBasisSet*> gbss =
                 config->getGBSList(sym);
              vector<TGaussianBasisSet::AngularMomentum> funcTypes;

              vector <const TGaussianBasisSet*>::const_iterator gbscurs ;
              for (gbscurs=gbss.begin(); gbscurs!= gbss.end(); gbscurs++) {
                 const TGaussianBasisSet* gbs = *gbscurs ;

                 int numContractedSets = gbs->num_contracted_sets(sym.c_str());

                 // For each contracted basis set:
                 for (int ics=0; ics<numContractedSets; ics++) {

                    // funcTypes: rf. ChemTypes.h enum: s_shell,p_shell, etc.
                    funcTypes = gbs->func_types(sym.c_str(),ics);
                    gbs->func_types(sym.c_str(),ics);
                    int numFuncTypes = funcTypes.size();
                    for (int fdx=0; fdx<numFuncTypes; fdx++) {
                       value = svec->value(sidx);
                       // Don't include these in min/max
                       //if (value > max) max = value;
                       //if (value < min) min = value;
                       item = p_treelist->AppendItem (myparent, wxString::Format (_T("%d"), currow++),-1,-1,new AtomIndex(idx));
                       p_treelist->SetItemText (item, 1, _T(""));
                       p_treelist->SetItemText (item, 2, _T(""));
                       p_treelist->SetItemText (item, 3, shells[funcTypes[fdx]]);
                       p_treelist->SetItemText (item, 4, wxString::Format (_T(PrefLabels::DOUBLEFORMAT), value));

                       sidx++;
                    }

                 }
              }
           } else {
              //No config or no Mulliken Shell charges
           }

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

        // This code used to be in the CSpectrum teleuse widget
        // when POSBEGSPECTRUM was used
        if (min > 0) min = 0.0;
        if (-min < max) min = -max;
        if (max < 0) max = 0.0;
        if (max < -min) max = -min;


        setSpectrumRange(min,max);
        computeColors(vec, min, max);

        // This is just a double check on our data sets
        if (svec && sidx!=svec->rows()) {
           cerr << "Number of shell charges doesn't match basis set" << endl;
        }


     }
   }

}

void MullikenPanel::setSpectrumRange(double min,double max)
{

   p_minLabel->SetLabel(wxString::Format (_T("% .3lf"), min));
   p_maxLabel->SetLabel(wxString::Format (_T("% .3lf"), max));
}

void MullikenPanel::computeColors(const PropVector *vec, double min, double max)
{
   clean();

   if (vec) {
      double val;

      int nRows = vec->rows();
      p_red = new float[nRows];
      p_green = new float[nRows];
      p_blue = new float[nRows];


      Spectrum s(min, max, gslices);
      float red,green,blue;
      for (int idx=0; idx<nRows; idx++) {
         val = vec->value(idx);

         s.getPosNegMidpointValue(val, red, green, blue);

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
void MullikenPanel::saveColorTable()
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


void MullikenPanel::restoreColorTable()
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


void MullikenPanel::setColorTable()
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


void MullikenPanel::OnTreeSelChanged( wxTreeEvent& event )
{
   if (p_internalSelect) return;

   vector<int> atoms, bonds;
   wxArrayTreeItemIds selections;
   int cnt = p_treelist->GetSelections(selections);
   for (int idx=0; idx< cnt; idx++) {
      AtomIndex * data = (AtomIndex*)p_treelist->GetItemData(selections.Item(idx));
      // Might be more overhead than its worth - not sure what duplicates do
      // to set selection commands.
      if (find(atoms.begin(), atoms.end(), data->p_index) == atoms.end()) {
         atoms.push_back(data->p_index);
      }
   }
   selectCommand(atoms,bonds);
}


void MullikenPanel::receiveFocus()
{

   saveColorTable();
   setColorTable();
}


void MullikenPanel::loseFocus()
{
   restoreColorTable();
}


void MullikenPanel::OnAssign(wxCommandEvent& event)
{
   SGFragment *frag = getFW().getSceneGraph().getFragment() ;
   vector<TAtm*> *atoms = frag->atoms();
   TAtm *atom;

   IPropCalculation *pcalc = getCalculation();
   PropVector *vec =  (PropVector*) pcalc->getProperty("MULLIKEN");
   if (vec != (PropVector*)0) {
     int nRows = vec->rows();
     for (int idx=0; idx<nRows; idx++) {
       atom = (*atoms)[idx];
       atom->partialCharge(vec->value(idx));
     }

     // Now save it - maybe later we should only allow this if the calc
     // has been reset?
     ICalculation *calc = dynamic_cast<ICalculation*>(pcalc);
     NULLPOINTEREXCEPTION(calc,"Unable to downcast to ICalculation");
     if (calc) {
       ResourceDescriptor::RUNSTATE state = calc->getState();
       if (calc->fragment(frag)) {
          calc->setState(state);

         // Not exactly the right event but forces atom table to refresh
         EventDispatcher::getDispatcher().publish(Event("StepChange"));
       } else {
          wxString msg = "Unable to save partial charges: ";
          msg += calc->messages();
          ewxMessageDialog dlg(this, msg, "ECCE Error",wxOK|wxICON_EXCLAMATION);
          (void)dlg.ShowModal();
          dlg.Destroy();
       }
     }
   }
}


