#include <iostream>
  using std::cout;
  using std::cerr;
  using std::endl;

#include "wx/sizer.h"
#include "wx/grid.h" 

#include "util/EventDispatcher.H"
#include "util/PreferenceLabels.H"

#include "tdat/TProperty.H"
#include "tdat/PropVector.H"

#include "dsm/IPropCalculation.H"
#include "dsm/ICalculation.H"

#include "wxgui/ewxConfig.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxWindowUtils.H"
#include "wxgui/WindowEvent.H"
#include "wxgui/ewxGrid.H"

#include "EigenValPanel.H"


// ----------------------------------------------------------------------------


BEGIN_EVENT_TABLE( EigenValPanel, DefaultVecPanel )
   EVT_GRID_RANGE_SELECT(EigenValPanel::OnGridRangeSelect)
END_EVENT_TABLE()


IMPLEMENT_DYNAMIC_CLASS(EigenValPanel, DefaultVecPanel)


EigenValPanel::EigenValPanel()
  : DefaultVecPanel(),
    WxVizTool(),
    p_internalSelect(false)
{
   subscribe();
}


EigenValPanel::EigenValPanel(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id, const wxPoint& pos,
        const wxSize& size, long style, const wxString& name)
  : DefaultVecPanel(),
    WxVizTool(),
    p_internalSelect(false)
{
   Create(calculation, parent, id, pos, size, style, name);
   subscribe();
}


EigenValPanel::~EigenValPanel()
{
   unsubscribe();
}


bool EigenValPanel::Create(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id, const wxPoint& pos,
        const wxSize& size, long style, const wxString& name)
{
   if (!DefaultVecPanel::Create(calculation, parent, id, pos, size, style )) {
      wxFAIL_MSG( wxT("DefaultVecPanel creation failed") );
      return false;
   }

   p_grid->SetRowLabelSize(0);
   p_grid->ForceRefresh();

   return true;
}

void EigenValPanel::eventMCB(const Event& event)
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
 * Fill the table and initialize the UI.
 */
void EigenValPanel::fillTable()
{

   IPropCalculation *expt = getCalculation();

   // Clear table
   p_grid->ClearGrid();
   p_grid->DeleteCols(0,p_grid->GetNumberCols());
   p_grid->DeleteRows(0,p_grid->GetNumberRows());

  set<string> keys = getPropertyNames();
  if (keys.size() > 0) {
     // Should be just one property so that is all we will process
     string key = *keys.begin();
     TProperty *prop = (TProperty*) expt->getProperty(key);
     PropVector *vec = dynamic_cast<PropVector*>(prop);
     if (vec) {

        // Get Fragment
        WxVizToolFW &fw = getFW();
        SGFragment *frag = fw.getSceneGraph().getFragment();
        int natoms = frag->numAtoms();

        p_grid->InsertRows(0,natoms);
        p_grid->InsertCols(0,5);

        wxString cellv;
        for (int idx=0; idx<natoms; idx++) {
           // First col is index
           cellv = wxString::Format (_T("%d"), idx);
           p_grid->SetCellValue(idx, 0, cellv);

           // Second col is atomic symbol
           TAtm *atm = frag->atomRef(idx);
           p_grid->SetCellValue(idx, 1, atm->atomicSymbol());

           // Next three cols are components
           for (int cdx=0; cdx<3; cdx++) {
              cellv = wxString::Format (_T(PrefLabels::DOUBLEFORMAT), vec->value((idx*3)+cdx));
              p_grid->SetCellValue(idx, cdx+2, cellv);
           }

        }
     }

  }

  p_grid->AutoSize();
}


/**
 * Process a range selection.
 * CTRL selects come in one per row making this a bit tricky.
 * Bizarre selection patterns typically seems to result in a unselect of the
 * full range followed by a select of the new range making it easier.
 */
void EigenValPanel::OnGridRangeSelect(wxGridRangeSelectEvent& event)
{
   if (p_internalSelect) return;

   SGFragment *frag = getFW().getSceneGraph().getFragment();
   vector<int> bonds;

   //cout << "Eigen selection" << event.GetTopRow() << "," << event.GetBottomRow() << " " << event.Selecting() << frag->m_atomHighLight.size() << endl;
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
