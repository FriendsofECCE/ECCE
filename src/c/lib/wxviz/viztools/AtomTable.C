/**
 * @file
 *
 *
 */

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <iostream>
  using std::cout;
  using std::cerr;
  using std::endl;

#include <vector>
  using std::vector;
#include <set>
  using std::set;

//using std::stable_sort;

#include "dsm/AtomView.H"

#include "tdat/TAtm.H"

#include "util/EventDispatcher.H"
#include "util/Event.H"
  using namespace ecce;
#include "util/ErrMsg.H"
#include "util/ETimer.H"
#include "util/StringConverter.H"
#include "util/STLUtil.H"

#include "viz/SGFragment.H"
#include "viz/SetPartialChargeCmd.H"
#include "viz/SetSuffixCmd.H"
#include "viz/SetAtomNameCmd.H"
#include "viz/SetAtomTypeCmd.H"
#include "viz/SetPolarizabilityCmd.H"
#include "viz/SetChargeGroupCmd.H"
#include "viz/SetPolarizationGroupCmd.H"
#include "viz/SetEnvIndexCmd.H"
#include "viz/SetConnectionCmd.H"
#include "viz/SetCoordinatesCmd.H"
#include "viz/SetMassCmd.H"
#include "viz/SetBehaviorCmd.H"
#include "viz/ClearBehaviorCmd.H"
#include "viz/RestoreMassCmd.H"
#include "viz/AtomLabelsCmd.H"
#include "viz/SelectCmd.H"

#include "wxgui/GridAtomElement.H"
#include "wxgui/GridElement.H"
#include "wxgui/GridModel.H"
#include "wxgui/ewxMessageDialog.H"
#include "wxgui/ewxWindowUtils.H"
#include "wxgui/ewxGrid.H"
#include "wxgui/WindowEvent.H"
#include "wxgui/WxGridView.H"

#include "wxviz/WxVizToolFW.H"
#include "wxviz/AtomTable.H"


//Hardwired column of numerical index.  We assume that this column is 
// always configured in the table
static const int INDEXCOL = 1;

const wxWindowID AtomTable::ID_DIALOG_ATOM_TABLE = wxNewId();
const wxWindowID AtomTable::ID_SUMCHARGES = wxNewId();
const wxWindowID AtomTable::ID_H2DEUT = wxNewId();
const wxWindowID AtomTable::ID_RESETMASS = wxNewId();
const wxWindowID AtomTable::ID_BQXATOMS = wxNewId();
const wxWindowID AtomTable::ID_CLRBEHAV = wxNewId();

IMPLEMENT_CLASS( AtomTable, MoleculeDataTable )

BEGIN_EVENT_TABLE( AtomTable, MoleculeDataTable )
  EVT_TOOL( wxID_ANY, AtomTable::OnMenuClick )
  EVT_GRID_SELECT_CELL(AtomTable::OnSelectCell)
END_EVENT_TABLE()


/**
 * Constructor.
 * WARNING: Messaging will not work correctly for this constructor
 *  if multipel subclasses are created because they can't be uniquely named.
 *  Default constructure is required for the IMPLEMENT_DYNAMIC_CLASS
 */
AtomTable::AtomTable() : MoleculeDataTable()
{}


AtomTable::AtomTable(wxWindow* parent,
                     wxWindowID id,
                     const wxPoint& pos,
                     const wxSize& size,
                     long style)
  : MoleculeDataTable()
{
  Create(parent, id, pos, size, style);
}


bool AtomTable::Create(wxWindow* parent,
                       wxWindowID id,
                       const wxPoint& pos,
                       const wxSize& size,
                       long style)
{
  // Get Registry
  ResourceDescriptor rd = ResourceDescriptor::getResourceDescriptor();

  // Get atom view names from registry
  multimap<int, AtomView*> atomViews = rd.getAtomViews();

  // Set default atom view (first one is default)
  p_atomView = atomViews.begin()->second;

  Listener::setId("AtomTable");

  if (!MoleculeDataTable::Create("AtomTable", parent, id, pos, size, style)) {
    wxFAIL_MSG( wxT("AtomTable creation failed") );
    return false;
  }

  return true;
}


/**
 * Destructor. 
 */
AtomTable::~AtomTable()
{}


wxWindow* AtomTable::GetTearableContent()
{
  wxToolBar *tb = new wxToolBar(this, wxID_ANY,
          wxDefaultPosition, wxDefaultSize,
          wxTB_VERTICAL|wxTB_TEXT|wxTB_NOICONS,
          "Atom Table Menu");

  // Get Registry
  ResourceDescriptor rd = ResourceDescriptor::getResourceDescriptor();
 
  // Get atom view names from registry
  multimap<int, AtomView*> atomViews = rd.getAtomViews();
 
  // Load tools cooresponding to each view
  multimap<int, AtomView*>::iterator mapIt; 
  for (mapIt = atomViews.begin(); mapIt != atomViews.end(); mapIt++) {
    tb->AddTool(mapIt->second->getId(),
                mapIt->second->getMenuItem(),
                wxNullBitmap, 
                mapIt->second->getToolTip(),
                wxITEM_RADIO);
  }

  tb->AddSeparator();

  tb->AddTool(ID_SUMCHARGES, "Sum partial charges", wxNullBitmap,
              "Sum all partial charge values");
  tb->AddTool(ID_H2DEUT, "Change H to Deuterium", wxNullBitmap,
              "Change all Hydrogens to Deuteriums");
  tb->AddTool(ID_RESETMASS, "Reset masses to defaults", wxNullBitmap,
              "Reset all mass values to their defaults");

  tb->AddSeparator();

  tb->AddTool(ID_BQXATOMS, "Make selected atoms BqX", wxNullBitmap,
              "Change selected atoms to have BqX behavior");
  tb->AddTool(ID_CLRBEHAV, "Clear all atom behavior", wxNullBitmap,
              "Clear atom behavior for all atoms");

  tb->Realize();

  if (p_viewId != -1) {
    tb->ToggleTool(p_viewId, true);
  }

  return tb;
}


/**
 * Initialize the grid using the current property view.
 */
void AtomTable::fillTable()
{
  // Get fragment/atoms
  SGFragment *frag = getFW().getSceneGraph().getFragment();
  int numAtoms = frag->numAtoms();

  p_atommap.clear(); 
  p_atommap.resize(numAtoms,-1);


  // I think the visible check should already be handled
  //if (isVisible()) {
  if (true) {
    p_table->getGrid()->BeginBatch();

    wxSizer *sizer = this->GetSizer();
    p_table->getGrid()->ClearGrid();

    // Get properties to view on grid
    multimap<int, AtomProperty*> properties = 
      p_atomView->getDisplayedProperties();
    
    // Get data model
    GridModel *gridModel = new GridModel();

    // Create grid columns
    gridModel->InsertCols(0, properties.size());

    if (numAtoms == 0) {
      //gridModel->DeleteCols(0, properties.size());

      // Display a "NO SYSTEM" message

    }
    else {
      // Load rows
      int row = 0;
      for (int i = 0; i < numAtoms; i++) {
        
        // Get Atom
        TAtm *atom = frag->atomRef(i);

        // Only include in grid, if it's not hidden and
        // its not a nub
        if ((atom->displayStyle().isDisplayed()) && (!atom->isNub())) {
           p_atommap[i] = row;

          // Default is to insert a single row
          gridModel->InsertRows(row);
          

          int column = 0;
          multimap<int, AtomProperty*>::iterator mapIt =
            properties.begin();
          
          while (mapIt != properties.end()) {
            
            AtomProperty *property = mapIt->second;

            try {
              GridAtomElement *cellValue = new GridAtomElement(atom, property);
              gridModel->SetValueAsGridElement(row, column, cellValue);
            }
            catch (InvalidException ex) {
              cerr << ex.what() << endl;
            }
            
            if (row == 0) {
              gridModel->SetColLabelValue(column, property->getLabel());
            }
            gridModel->SetRowLabelValue(row,"");
            
            mapIt++;
            column++;
          }
          row++;
        }
      }
      sizer->Layout();
    }

    // Go ahead and assign data model to grid
    // WxGridView takes ownership of pointer
    p_table->setModel(gridModel);

    // Restore sort settings
    //restoreSortSettings();

    // Sort grid
    int sortCol = p_table->getSortCol();
    bool ascSort = p_table->getAscSort();
    if (sortCol >= 0) {
      p_table->sortByCol(sortCol, ascSort);
    }
    else {
      p_table->sortByCol(1, true);
    }

    p_table->getGrid()->EndBatch();

    //restore selections - has its own begin/end batch
    setSelections();

  }

  //TODO make this into a generic method on ToolboxControl
  // Is this dead code??
  char buf[128];
  sprintf(buf, "%s (%zu)",GetName().c_str(), frag->numNonNubs());
  WindowEvent event("ToolboxControlLabel",buf,this);
  EventDispatcher::getDispatcher().publish(event);
}

/**
 * Refreshes just the coordinates of the table 
 * useful for steping through geometries/trajectories
 */
void AtomTable::fastFillTable()
{
   // Get fragment/atoms
   SGFragment *frag = getFW().getSceneGraph().getFragment();
   int numAtoms = frag->numAtoms();

   // This can occur when new contexts are created; quietly ignore
   // otherwise you will mysteriously crash.  In particular, I was
   // getting crashes when creating a new context if there were any
   // atoms/bonds selected - bad for: copy/new/paste.
   if (numAtoms == 0) return;

   p_atommap.clear(); 
   p_atommap.resize(numAtoms,-1);


   if (true /*isVisible*/) {
      p_table->getGrid()->BeginBatch();

      wxSizer *sizer = this->GetSizer();
      //    p_table->getGrid()->ClearGrid();

      // Get properties to view on grid
      multimap<int, AtomProperty*> properties = 
         p_atomView->getDisplayedProperties();

      // Get data model
      GridModel *gridModel = p_table->getModel();

      // Create grid columns
      //    gridModel->InsertCols(0, properties.size());

      // Load rows
      int row = 0;
      for (int i = 0; i < numAtoms; i++) {

         // Get Atom
         TAtm *atom = frag->atomRef(i);

         // Only include in grid, if it's not hidden and
         // its not a nub
         if ((atom->displayStyle().isDisplayed()) && (!atom->isNub())) {
            p_atommap[i] = row;


            int column = 0;
            multimap<int, AtomProperty*>::iterator mapIt =
               properties.begin();

            while (mapIt != properties.end()) {

               AtomProperty *property = mapIt->second;

               try {
                  GridAtomElement *cellValue = new GridAtomElement(atom, property);
                  gridModel->SetValueAsGridElement(row, column, cellValue);
               }
               catch (InvalidException ex) {
                  cerr << ex.what() << endl;
               }

               if (row == 0) {
                  gridModel->SetColLabelValue(column, property->getLabel());
               }
               gridModel->SetRowLabelValue(row,"");

               mapIt++;
               column++;
            }
            row++;
         }
      }
      sizer->Layout();


      // Might want to toss all this sorting stuff
      // Sort grid
      int sortCol = p_table->getSortCol();
      bool ascSort = p_table->getAscSort();
      if (sortCol >= 0) {
         p_table->sortByCol(sortCol, ascSort);
      }
      else {
         p_table->sortByCol(1, true);
      }

      p_table->getGrid()->EndBatch();

      //restore selections - has its own begin/end batch
      setSelections();

   }

}


/**
 * Takes selection from fragment and sets selection in table.
 */
void AtomTable::setSelections()
{

//ETimer timer;
//timer.start();

  vector<GridAtomElement*> selectedRows;
  SGFragment *frag = getFW().getSceneGraph().getFragment();
  vector<int> *hl = frag->getAtomHighlight();

  size_t rows = p_table->getRowCount();
  size_t cols = p_table->getColCount();

  p_table->clearSelectedRows();

  if (((rows > 0) && (cols > 0)) && hl->size() > 0) {

    // build selected row vector
    for (size_t i = 0; i < rows; i++) {
      GridAtomElement *element =
        dynamic_cast<GridAtomElement*>(p_table->getElement(i,0));
      element->setRow(i);
      selectedRows.push_back(element);
    }

    // Sort vector on index 
    stable_sort(selectedRows.begin(), selectedRows.end(), sortRows);

    // Select the rows
    p_table->getGrid()->BeginBatch();
    // Disable gridRangeSelect - necessary to avoid re-selection of the scenegraph
    this->Disconnect(wxEVT_GRID_RANGE_SELECT,
                  wxGridRangeSelectEventHandler(MoleculeDataTable::gridRangeSelect));


    //
    // KLS 09/24/07
    // Added p_atommap to handle the case where rows are not displayed.
    // Currently the map is always created(fillTable) but I left the 
    // if/else statement in below in case we decide its better to create it
    // temporarily (in else block) and throw it away when we are done.
    // This handles the cases: nubs are not in table; some atoms are invisible
    //
    int numHighlighted = hl->size();
    if (rows == frag->numAtoms()) {
       for (int i = 0; i < numHighlighted; i++) {
          int row = selectedRows[(*hl)[i]]->getRow();
          p_table->selectRow(row, true);
       }
    } else {
       for (int i = 0; i < numHighlighted; i++) {
          int maprow = p_atommap[(*hl)[i]];
          if (maprow != -1) {
             p_table->selectRow(selectedRows[maprow]->getRow(), true);
          }
       }
    }
    // Go ahead and enable - done selecting the rows
    this->Connect(wxEVT_GRID_RANGE_SELECT,
                  wxGridRangeSelectEventHandler(MoleculeDataTable::gridRangeSelect));
    p_table->getGrid()->EndBatch();

  }

//timer.stop();
//cout << "done getting selections :: elapsed time " << timer.elapsedTime() << endl;
}




void AtomTable::updateFragment(GridAtomElement *element)
{
  SGFragment *sgfrag = getFW().getSceneGraph().getFragment();
  bool refreshLabels = false;
  int atomIndex = element->getIndex();
  TAtm *atom = sgfrag->atomRef(atomIndex);
  AtomProperty *ap = dynamic_cast<AtomProperty*>(element->getProperty()); 
  AtomProperty::Prop prop = AtomProperty::AP_UNDEFINED;
  if (ap != 0) {
    prop = ap->getProp();
  }
  string val = element->getValue().c_str();

  switch (prop) {

    case AtomProperty::AP_CHARGE:
      {
        // Refresh labels after update
        refreshLabels = true;

        // Create Command object
        SetPartialChargeCmd *cmd = 
          new SetPartialChargeCmd("New Partial Charge", &getFW().getSceneGraph(), getId());

        // Set parameters
        double dval;
        StringConverter::toDouble(val, dval);
        cmd->getParameter("atomnum")->setInteger(atomIndex);
        cmd->getParameter("partialCharge")->setDouble(dval);

        // Execute command
        getFW().execute(cmd);
      }
      break;

    case AtomProperty::AP_SUFFIX:
      {
        // Refresh labels after update
        refreshLabels = true;

        // Create Command object
        SetSuffixCmd *cmd = 
          new SetSuffixCmd("Set Suffix", &getFW().getSceneGraph(), getId());

        // Set parameters
        cmd->getParameter("atomnum")->setInteger(atomIndex);
        cmd->getParameter("suffix")->setString(val);

        // Execute command
        getFW().execute(cmd);
      }
      break;

    case AtomProperty::AP_ATOM_NAME:
      {
        // Refresh labels after update
        refreshLabels = true;

        // Create Command object
        SetAtomNameCmd *cmd = 
          new SetAtomNameCmd("Set Atom Name", &getFW().getSceneGraph(), getId());

        // Set parameters
        cmd->getParameter("atomnum")->setInteger(atomIndex);
        cmd->getParameter("atomName")->setString(val);

        // Execute command
        getFW().execute(cmd);
      }
      break;

    case AtomProperty::AP_TYPE:
      {
        // Refresh labels after update
        refreshLabels = true;

        // Create Command object
        SetAtomTypeCmd *cmd = 
          new SetAtomTypeCmd("Set Atom Type", &getFW().getSceneGraph(), getId());

        // Set parameters
        cmd->getParameter("atomnum")->setInteger(atomIndex);
        cmd->getParameter("atomType")->setString(val);

        // Execute command
        getFW().execute(cmd);
      }
      break;

    case AtomProperty::AP_POL:
      {
        // Create Command object
        SetPolarizabilityCmd *cmd = 
          new SetPolarizabilityCmd("Set Polarizability", &getFW().getSceneGraph(), getId());

        // Set parameters
        double dval;
        StringConverter::toDouble(val, dval);
        cmd->getParameter("atomnum")->setInteger(atomIndex);
        cmd->getParameter("polarization")->setDouble(dval);

        // Execute command
        getFW().execute(cmd);
      }
      break;

    case AtomProperty::AP_CG:
      {
        // Create Command object
        SetChargeGroupCmd *cmd = 
          new SetChargeGroupCmd("Set Charge Group", &getFW().getSceneGraph(), getId());

        // Set parameters
        int ival;
        StringConverter::toInt(val, ival);
        cmd->getParameter("atomnum")->setInteger(atomIndex);
        cmd->getParameter("chargeGroup")->setInteger(ival);

        // Execute command
        getFW().execute(cmd);
      }
      break;

    case AtomProperty::AP_PG:
      {
        // Create Command object
        SetPolarizationGroupCmd *cmd = 
          new SetPolarizationGroupCmd("Set Polarization Group", &getFW().getSceneGraph(), getId());

        // Set parameters
        int ival;
        StringConverter::toInt(val, ival);
        cmd->getParameter("atomnum")->setInteger(atomIndex);
        cmd->getParameter("polarizationGroup")->setInteger(ival);

        // Execute command
        getFW().execute(cmd);
      }
      break;

    case AtomProperty::AP_ENV:
      {
        // Create Command object
        SetEnvIndexCmd *cmd = 
          new SetEnvIndexCmd("Set Env Index", &getFW().getSceneGraph(), getId());

        // Set parameters
        int ival;
        StringConverter::toInt(val, ival);
        cmd->getParameter("atomnum")->setInteger(atomIndex);
        cmd->getParameter("envIndex")->setInteger(ival);

        // Execute command
        getFW().execute(cmd);
      }
      break;

    case AtomProperty::AP_LINK_NUM:
      {
        // Create Command object
        SetConnectionCmd *cmd = 
          new SetConnectionCmd("Set Connection", &getFW().getSceneGraph(), getId());

        // Set parameters
        int ival = 0;
        StringConverter::toInt(val, ival);
        cmd->getParameter("atomnum")->setInteger(atomIndex);
        //cmd->getParameter("connectionIndex")->setInteger(ival);
        cmd->getParameter("connectionIndex")->setInteger(1);

        // Execute command
        getFW().execute(cmd);
      }
      break;

    case AtomProperty::AP_XCOORD:
      {
        // Create Command object
        SetCoordinatesCmd *cmd = 
          new SetCoordinatesCmd("Set Coordinates", &getFW().getSceneGraph(), getId());

        // Set parameters
        const double *coords = atom->coordinates();
        double dval;
        StringConverter::toDouble(val, dval);
        cmd->getParameter("x")->setDouble(dval);
        cmd->getParameter("y")->setDouble(coords[1]);
        cmd->getParameter("z")->setDouble(coords[2]);
        cmd->getParameter("atomnum")->setInteger(atomIndex);

        // Execute command
        getFW().execute(cmd);
      }
      break;

    case AtomProperty::AP_YCOORD:
      {
        // Create Command object
        SetCoordinatesCmd *cmd = 
          new SetCoordinatesCmd("Set Coordinates", &getFW().getSceneGraph(), getId());

        // Set parameters
        const double *coords = atom->coordinates();
        double dval;
        StringConverter::toDouble(val, dval);
        cmd->getParameter("x")->setDouble(coords[0]);
        cmd->getParameter("y")->setDouble(dval);
        cmd->getParameter("z")->setDouble(coords[2]);
        cmd->getParameter("atomnum")->setInteger(atomIndex);

        // Execute command
        getFW().execute(cmd);
      }
      break;

    case AtomProperty::AP_ZCOORD:
      {
        // Create Command object
        SetCoordinatesCmd *cmd = 
          new SetCoordinatesCmd("Set Coordinates", &getFW().getSceneGraph(), getId());

        // Set parameters
        const double *coords = atom->coordinates();
        double dval;
        StringConverter::toDouble(val, dval);
        cmd->getParameter("x")->setDouble(coords[0]);
        cmd->getParameter("y")->setDouble(coords[1]);
        cmd->getParameter("z")->setDouble(dval);
        cmd->getParameter("atomnum")->setInteger(atomIndex);

        // Execute command
        getFW().execute(cmd);
      }
      break;

    case AtomProperty::AP_MASS:
      {
        // Create Command object
        SetMassCmd *cmd = 
          new SetMassCmd("Set Mass", &getFW().getSceneGraph(), getId());

        // Set parameters
        string symbol = atom->atomicSymbol();
        double dval;
        StringConverter::toDouble(val, dval);
        cmd->getParameter("value")->setDouble(dval);
        cmd->getParameter("atomicSymbol")->setString(symbol.c_str());
        cmd->getParameter("atomicMass")->setInteger(-1);

        // Execute command
        getFW().execute(cmd);
      }
      break;

    case AtomProperty::AP_BEHAVIOR:
      {
        // Create Command object
        SetBehaviorCmd *cmd = 
          new SetBehaviorCmd("Set Behavior", &getFW().getSceneGraph(), getId());

        // Set parameters
        cmd->getParameter("atomnum")->setInteger(atomIndex);
        cmd->getParameter("behavior")->setInteger(TAtm::stringToBehaviorType(val));

        // Execute command
        getFW().execute(cmd);
      }
      break;

    default:
      
      EE_RT_ASSERT(FALSE, EE_WARNING, "Invalid column for update");
      break;
  }

  // Refresh atom labels if necessary
  if (refreshLabels && areLabelsOn()) {
    AtomLabelsCmd *cmd = 
      new AtomLabelsCmd("Atom Labels", &getFW().getSceneGraph(), this);
    getFW().execute(cmd);
  }
}


void AtomTable::notifySelections(vector<GridAtomElement*> *selections, bool sendNotification)
{
  // We have had some indecision about whether to use the command for all
  // selections.  A drawback may be selection speed creating and reparsing 
  // strings but I think that may have been due to a different bug.
  // At this point, the command seems like the preferred manner to me
  if (selections->size() > 0) {
#define selectcmd
#ifdef selectcmd
    vector<string> atomsstr;
#else
    vector<int> atoms;
#endif
    vector<int> bonds;
    char buf[12];
    for (size_t i = 0; i < selections->size(); i++) {
      GridAtomElement *element = (*selections)[i];
      if (element != 0) {
        // Add one to get user index number
        int atomIndex = element->getIndex() + 1;
#ifdef selectcmd
        // +1 because the command uses user comprehensible indices starting
        // from 1 instead of 0.
        // Not sure what is in element???
        sprintf(buf,"%d",atomIndex);
        atomsstr.push_back(buf);
#else
        atoms.push_back(atomIndex);
#endif
      }
    }

#ifdef selectcmd
    SelectCmd *cmd = new SelectCmd("Select", &getFW().getSceneGraph(), getId());
    cmd->getParameter("clear")->setBoolean(true);
    cmd->getParameter("atoms")->setStringList(atomsstr);
    getFW().execute(cmd);
#else
    //setSelection(atoms, bonds);
    // @todo if using setSelection, must set the fragment m_atomHighlight and
    // the bond one too.  
#endif
  }
}

/**
 * If use clicks in an edit cell, make it the current residue edting
 * index.
 */
void AtomTable::OnSelectCell(wxGridEvent& event)
{
   string label = p_table->getGrid()->GetColLabelValue(event.GetCol()).c_str();
   STLUtil::stripLeadingWhiteSpace(label);
   if (label == "") {
      // This is the atom column
      string atom;
      atom = p_table->getGrid()->GetCellValue(event.GetRow(),event.GetCol()).c_str();
      SelectCmd *cmd = new SelectCmd("Select element", &getFW().getSceneGraph(),getId());
      cmd->getParameter("clear")->setBoolean(true);
      cmd->getParameter("element")->setString(atom);
      getFW().execute(cmd);
   }
   if (label.find("Name",0) == 0) {
      vector<string> names;
      names.push_back(p_table->getGrid()->GetCellValue(event.GetRow(),event.GetCol()).c_str());
      SelectCmd *cmd = new SelectCmd("Select residue name", &getFW().getSceneGraph(),getId());
      cmd->getParameter("clear")->setBoolean(true);
      cmd->getParameter("residues")->setStringList(names);
      getFW().execute(cmd);
   }
   if (label.find("Atom Name",0) == 0) {
      string name;
      name = p_table->getGrid()->GetCellValue(event.GetRow(),event.GetCol()).c_str();
      SelectCmd *cmd = new SelectCmd("Select atom name", &getFW().getSceneGraph(),getId());
      cmd->getParameter("clear")->setBoolean(true);
      cmd->getParameter("atom_name")->setString(name);
      getFW().execute(cmd);
   }
   if (label.find("Chain",0) != string::npos) {
      vector<string> chains;
      chains.push_back(p_table->getGrid()->GetCellValue(event.GetRow(),event.GetCol()).c_str());
      SelectCmd *cmd = new SelectCmd("Select chain", &getFW().getSceneGraph(),getId());
      cmd->getParameter("clear")->setBoolean(true);
      cmd->getParameter("chains")->setStringList(chains);
      getFW().execute(cmd);
   }
   event.Skip();
}


void AtomTable::OnMenuClick( wxCommandEvent& event )
{
  /*
   // Request that the menu go away
   // Do this first because otherwise it stays there when we post the 
   // partial charge message dialog
   wxObject *obj = event.GetEventObject();
   wxWindow *win = dynamic_cast<wxWindow*>(obj);
   WindowEvent winevent("RequestPopdown","",
         ewxWindowUtils::getTopWindow(win));
   EventDispatcher::getDispatcher().publish(winevent);
   */



   ResourceDescriptor rd = ResourceDescriptor::getResourceDescriptor();
   int low, high;
   rd.getAtomViewIdRange(low, high);
   int id = event.GetId();

   if ((id >= low) && (id <= high)) {
       p_viewId = id;
       p_atomView = rd.getAtomView(p_viewId);
       fillTable();
   }
   else if (id == ID_SUMCHARGES) {
       sumCharges();
   }
   else if (id == ID_H2DEUT) {
       Command *cmd = new SetMassCmd("Change H to Deuterium",&getFW().getSceneGraph());
       cmd->getParameter("atomicSymbol")->setString("H");
       cmd->getParameter("atomicMass")->setInteger(2);
       getFW().execute(cmd);
   }
   else if (id == ID_RESETMASS) {
       Command *cmd = new RestoreMassCmd("Restore masses to defaults",&getFW().getSceneGraph());
       getFW().execute(cmd);
   }
   else if (id == ID_BQXATOMS) {
       Command *cmd = new SetBehaviorCmd("Make selected atoms BqX",&getFW().getSceneGraph());
       cmd->getParameter("behavior")->setInteger(TAtm::BQX);
       getFW().execute(cmd);
   }
   else if (id == ID_CLRBEHAV) {
       Command *cmd = new ClearBehaviorCmd("Clear all atom behavior",&getFW().getSceneGraph());
       getFW().execute(cmd);
   }

   event.Skip();
}



void AtomTable::eventMCB(const Event& event)
{
   // Only process events that are not sent by me
   if (event.getSenderId() == getId()) return;

   if (!isVisible()) return;

   // Refresh
   if (event.getName() == "UpdateUI") {
     
      fillTable();
   }

   // Geometry Changed
   if (event.getName() == "GeomChange" || event.getName() == "ResidueChange") {
      fillTable();
   }

   // Like GeomChange but no editing really occured.
   if (event.getName() == "StepChange") {
      fastFillTable();
   }

   if (event.getName() == "VisibilityChange") {
      fillTable();
   }

   // Selections Changed
   if (event.getName() == "SelectionChanged") {
      setSelections();

      wxArrayInt rows = p_table->getGrid()->GetSelectedRows();
      if (rows.GetCount() > 0) {
         int row = rows.Item(0);
         p_table->getGrid()->MakeCellVisible(row,0);
      }

   }
}

void AtomTable::sumCharges()
{
   SGFragment *frag = getFW().getSceneGraph().getFragment();

   double sum = 0.0;     // The final result we'll display
   string msg;           // The dialog message - depends on selection

   set<int> selectedRows = p_table->getSelectedRows();

   if (selectedRows.size() == 0) {
      // Sum over all atoms visible atoms
      size_t numAtoms = frag->numAtoms();
      size_t nRows = p_table->getRowCount();
      if (nRows < numAtoms) {
         // Only sum over atoms currently displayed.  We must
         // go back to the source data because the Grid might not have
         // the data in its view
         for (size_t idx=0; idx<numAtoms; idx++) {
            TAtm *atom = frag->atomRef(idx);

            // Only include in grid, if it's not hidden and its not a nub
            if ((atom->displayStyle().isDisplayed()) && (!atom->isNub())) {
               sum += atom->partialCharge();
            }
         }
      } else {
         sum = frag->getTotalCharge();
      }

      msg = "The sum of the partial charges is ";

      // KLS why do this?
      // For charge field on old main UI
      // What to do???
      //sgfrag->charge(sum);
     // cout << "should we be setting the charge???" << endl;

   } else {
      // Sum only selected atoms
      // Can't assume data is in table but must take sort order into account
      for (set<int>::iterator setIt = selectedRows.begin();
                  setIt != selectedRows.end();
                  setIt++) {
         int row = *setIt;
         int atomIndex = rowToAtomIndex(row);
         TAtm *atom = frag->atomRef(atomIndex);
         sum += atom->partialCharge();
      }
      msg = "The sum of the partial charges for the selected atoms is ";
   }
   msg += StringConverter::toString(sum);

   ewxMessageDialog dlg(this, msg.c_str(), "Sum of charges", wxOK);
   dlg.ShowModal();

}



/**
 * Figures out the numerical residue index for the row.
 */
int AtomTable::rowToAtomIndex(int row)
{
   int ret = 0;
   GridAtomElement *element =
            dynamic_cast<GridAtomElement*>(p_table->getElement(row,INDEXCOL));
   ret = atoi(element->getValue().c_str());

   return ret-1; // table counts from 0
}

void AtomTable::restoreSettings(wxConfig * config)
{
   int view = -1;
   if (config->Read("/AtomTable/View", &view)) {
      if (view > 0) {
         p_viewId = view;
         ResourceDescriptor rd = ResourceDescriptor::getResourceDescriptor();
         if (rd.getAtomView(p_viewId)) {
            p_atomView = rd.getAtomView(p_viewId);
            //fillTable();
         }
      }
   }

  int sortCol = 0;
  if (config->Read("/AtomTable/SortCol", &sortCol)) {
    p_table->setSortCol(sortCol);
  }

  bool choice = true;
  if (config->Read("/AtomTable/IsAscSort", choice)) {
    p_table->setAscSort(choice);
  }
}

void AtomTable::saveSettings(wxConfig * config)
{
   config->Write("/AtomTable/View", p_viewId);
   config->Write("/AtomTable/SortCol", p_table->getSortCol());
   config->Write("/AtomTable/IsAscSort", p_table->getAscSort());
}
