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
#include "dsm/SegFactory.H"

//#include "tdat/TAtm.H"
#include "tdat/Fragment.H"

#include "util/Ecce.H"
#include "util/Event.H"
  using namespace ecce;
//#include "util/ErrMsg.H"
//#include "util/ETimer.H"
#include "util/SFile.H"
#include "util/STLUtil.H"

#include "wxgui/ewxGrid.H"
#include "wxgui/GridAtomElement.H"
//#include "wxgui/GridElement.H"
#include "wxgui/GridModel.H"
#include "wxgui/ewxFileDialog.H"
//#include "wxgui/ewxWindowUtils.H"
//#include "wxgui/TearableMini.H"
//#include "wxgui/WindowEvent.H"
#include "wxgui/WxGridView.H"
#include "wxgui/ewxMessageDialog.H"

#include "viz/AddSegmentHCmd.H"
#include "viz/AssignResidueAtomsCmd.H"
#include "viz/DeleteResidueCmd.H"
#include "viz/InsertResidueCmd.H"
#include "viz/MakeResidueCmd.H"
#include "viz/RenumberResiduesCmd.H"
#include "viz/SelectCmd.H"
#include "viz/SGFragment.H"
#include "viz/ValidateStrCmd.H"

//#include "wxviz/WxVizToolFW.H"
#include "wxviz/ResidueTable.H"
//#include "wxviz/ResidueTableMenu.H"
#include "wxgui/WxFFDataConfig.H"

// static frag pointer for sort - artifact of re-using GridAtomElement
static Fragment *hackfrag = 0;

//Hardwired column for residue editing.  WE assume that this column is
//always configured in the table
static const int EDITCOL = 0;
//Hardwired column of numerical index.  We assume that this column is 
// always configured in the table
static const int INDEXCOL = 1;

static const wxWindowID ID_DIALOG_RESIDUE_TABLE = wxNewId();

IMPLEMENT_CLASS(ResidueTable, MoleculeDataTable )

BEGIN_EVENT_TABLE( ResidueTable, MoleculeDataTable )

  //TODO could add specific ids here (see MenuIds) 
  EVT_MENU(wxID_ANY, ResidueTable::OnMenuClick)
  EVT_GRID_SELECT_CELL(ResidueTable::OnSelectCell)
  EVT_GRID_LABEL_LEFT_CLICK(ResidueTable::OnGridLabelLeftClick)

END_EVENT_TABLE()


/**
 * Constructor.
 * WARNING: Messaging will not work correctly for this constructor
 *  if multipel subclasses are created because they can't be uniquely named.
 *  Default constructure is required for the IMPLEMENT_DYNAMIC_CLASS
 */
ResidueTable::ResidueTable()
  : MoleculeDataTable()
{}


ResidueTable::ResidueTable(wxWindow* parent,
                           wxWindowID id,
                           const wxPoint& pos,
                           const wxSize& size,
                           long style)
  : MoleculeDataTable()
{
  Create(parent, id, pos, size, style);
}


bool ResidueTable::Create(wxWindow* parent,
                          wxWindowID id,
                          const wxPoint& pos,
                          const wxSize& size,
                          long style)
{
  Listener::setId("ResidueTable");

  if (!MoleculeDataTable::Create("ResidueTable", parent, id, pos, size, style))
  {
    wxFAIL_MSG( wxT("ResidueTable creation failed") );
    return false;
  }

  // We will do the label click processing ourselves
  p_table->enableLabelClickProcessing(false);

  return true;
}


/**
 * Destructor. 
 */
ResidueTable::~ResidueTable()
{}



/**
 * Clears and refills table.
 */
void ResidueTable::refresh()
{
   fillTable();
}


/**
 * Initialize the grid using the current property view.
 */
void ResidueTable::fillTable()
{
  // Get residues
  Fragment *frag = getFW().getSceneGraph().getFragment();
  int numResidues = frag->numResidues();

  p_residuemap.clear();
  p_residuemap.resize(numResidues,-1);

  // I think the visible check should already be handled
  //if (isVisible()) {
  if (true) {

    wxSizer *sizer = this->GetSizer();
    p_table->getGrid()->ClearGrid();

    // Get properties to view on grid
    multimap<int, AtomProperty*> properties = 
      p_atomView->getDisplayedProperties();
    
    // Get data model
    GridModel *gridModel = new GridModel();

    // Create grid columns
    gridModel->InsertCols(0, properties.size());

    if (numResidues > 0) {
      // Load rows
      const vector<Residue*>& residues = frag->residues();
      Residue *res = 0;
      int row = 0;
      GridAtomElement *cellValue = 0;
      for (int i = 0; i < numResidues; i++) {

         // Get Residue
         res = residues[i];

         // Only include in grid if it's displayed
         //if (res->displayStyle().getStyle() != DisplayStyle::DNONE) 
         if (res->displayStyle().isDisplayed()) {

            p_residuemap[i] = row;

            // Default is to insert a single row
            gridModel->InsertRows(row);

            int column = 0;
            multimap<int, AtomProperty*>::iterator mapIt =
               properties.begin();

            while (mapIt != properties.end()) {

               AtomProperty *property = mapIt->second;

               try {
                  // ***ONLY*** use the verions of GridAtomElement that takes
                  // a residue in the constructor.  Not all residues have
                  // atoms and the indices mean different things
                  cellValue = new GridAtomElement(res, property);
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


    // Fix up the edit column.  This is necessary because the GridAtomElement
    // structures (based on atoms and their residues) don't know about the
    // residue editing index which is at the fragment level.
    // DO this AFTER setting the model or ka-bang
    int curIndex = frag->editResidueIndex();
    if (curIndex >= 0) {
       GridAtomElement *element = dynamic_cast<GridAtomElement*>(p_table->getElement(curIndex,EDITCOL));
       element->setValue("true");
    }

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

    //restore selections
    setSelections();

  }

}



/**
 * Take selections from fragment and select appropriate rows in table.
 */
void ResidueTable::setSelections()
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

    // What we had was selected atoms.  We need to turn this into 
    // residue numbers.
    vector<int> selRes;
    frag->atomsToResidues(*hl, selRes);

    // build selected row vector
    for (size_t i = 0; i < rows; i++) {
       GridAtomElement *element =
          dynamic_cast<GridAtomElement*>(p_table->getElement(i,0));
       element->setRow(i);
       selectedRows.push_back(element);
    }

     // Sort vector on index
     hackfrag = frag;
     stable_sort(selectedRows.begin(), selectedRows.end(), sortResidueRows);
     hackfrag = 0;


    // Select the rows
    p_table->getGrid()->BeginBatch();
    // Disable gridRangeSelect - necessary to avoid re-selection of the scenegraph
    this->Disconnect(wxEVT_GRID_RANGE_SELECT,
                  wxGridRangeSelectEventHandler(MoleculeDataTable::gridRangeSelect));

    //
    // KLS 09/24/07
    // Added p_residuemap to handle the case where rows are not displayed.
    // Currently the map is always created(fillTable) but I left the 
    // if/else statement in below in case we decide its better to create it
    // temporarily (in else block) and throw it away when we are done.
    // This handles the case: some residues are invisible
    //
    if (rows == frag->numResidues()) {
       for (size_t i = 0; i < selRes.size(); i++) {
          p_table->selectRow(selectedRows[selRes[i]]->getRow(),true);
       }
    } else {
       for (size_t i = 0; i < selRes.size(); i++) {
          int resrow = p_residuemap[selRes[i]];
          if (resrow != -1) {
             p_table->selectRow(selectedRows[resrow]->getRow(),true);
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


/**
 * Figures out the numerical residue index for the row.
 */
int ResidueTable::rowToResIndex(int row)
{
   int ret = 0;
   GridAtomElement *element =
            dynamic_cast<GridAtomElement*>(p_table->getElement(row,INDEXCOL));
   ret = atoi(element->getValue().c_str());

   return ret-1; // table counts from 0
}

/**
 * Figures out the row number that the residue is displayed in taking
 * into account the sort order.
 */
int ResidueTable::resIndexToRow(int resIndex)
{
   int row = -1;
   vector<GridAtomElement*> selectedRows;
   SGFragment *frag = getFW().getSceneGraph().getFragment();

   int rows = p_table->getRowCount();

   if (resIndex >= 0) {

      // build selected row vector
      for (int i = 0; i < rows; i++) {
         GridAtomElement *element =
            dynamic_cast<GridAtomElement*>(p_table->getElement(i,0));
         element->setRow(i);
         selectedRows.push_back(element);
      }

      // Sort vector on index
      hackfrag = frag;
      stable_sort(selectedRows.begin(), selectedRows.end(), sortResidueRows);
      hackfrag = 0;


      // Select the rows
      row = selectedRows[resIndex]->getRow();

   }
   return row;
}



void ResidueTable::updateFragment(GridAtomElement *element)
{
}


void ResidueTable::notifySelections(vector<GridAtomElement*> *selections, bool sendNotification)
{
  if (selections->size() > 0) {
     vector<string> resstr;

    char buf[32];
    for (size_t i = 0; i < selections->size(); i++) {
      GridAtomElement *element = (*selections)[i];
      if (element != 0) {
        // +1 puts the residue index in user indexing (1..n)
        sprintf(buf,"%d", element->getIndex()+1);
        resstr.push_back(buf);
      }
    }

    SelectCmd *cmd = new SelectCmd("Select", &getFW().getSceneGraph(),getId());
    cmd->getParameter("clear")->setBoolean(true);
    cmd->getParameter("residues")->setStringList(resstr);
    getFW().execute(cmd);


  }
}



wxWindow* ResidueTable::GetTearableContent()
{
   wxToolBar *tb = new wxToolBar(this, wxID_ANY,
           wxDefaultPosition, wxDefaultSize,
           wxTB_VERTICAL|wxTB_TEXT|wxTB_NOICONS,
           "Residue Table Menu");

   // Get Registry
   ResourceDescriptor rd = ResourceDescriptor::getResourceDescriptor();
 
   // Get atom view names from registry
   multimap<int, AtomView*> residueViews =
     rd.getResidueViews();
 
   // Load tools cooresponding to each view
   multimap<int, AtomView*>::iterator mapIt; 
   for (mapIt = residueViews.begin(); mapIt != residueViews.end(); mapIt++) {
      tb->AddTool(mapIt->second->getId(),
                  mapIt->second->getMenuItem(),
                  wxNullBitmap, 
                  mapIt->second->getToolTip(),
                  wxITEM_RADIO);
   }
   tb->AddSeparator();
   /*KLS access from main Build menu
   tb->AddTool(MAKERES,"Make into residue",wxNullBitmap, "Make into residue");
   */
   tb->AddTool(RENUMBER,"Renumber residues",wxNullBitmap, "Renumber residues");
   tb->AddTool(DELETERES,"Delete residue(s)",wxNullBitmap, "Delete residues");
   tb->AddTool(INSERTRES,"Insert residue",wxNullBitmap, "Insert residue");
   tb->AddTool(APPENDRES,"Append residue",wxNullBitmap, "Append residue");
   tb->AddSeparator();
   tb->AddTool(VALIDATE,"Validate structure",wxNullBitmap, "Validate structure");
   tb->AddTool(ADDH,"Add hydrogens",wxNullBitmap, "Add hydrogens");
   tb->AddTool(ASSIGNRESATOMS,"Assign residue atoms",wxNullBitmap, "Assign residue atoms");
   tb->AddSeparator();
   tb->AddTool(CONFIGURE,"Configure force field data...",wxNullBitmap, "Brings up dialog to specify paths where forcefield data files are located.");
   tb->AddTool(WRITESEG,"Write Segment file...",wxNullBitmap, "Writes a segment file for a single residue.");
   tb->AddTool(WRITEFRAG,"Write Fragment file...",wxNullBitmap, "Writes a fragment file for a single residue.");
   tb->Realize();

   if (p_viewId != -1) {
      tb->ToggleTool(p_viewId, true);
   }

   return tb;
}


void ResidueTable::OnMenuClick( wxCommandEvent& event )
{
   ResourceDescriptor rd = ResourceDescriptor::getResourceDescriptor();
   int low, high;
   rd.getResidueViewIdRange(low, high);
   int id = event.GetId();

   if ((id >= low) && (id <= high)) {
       p_viewId = id;
       p_atomView = rd.getResidueView(p_viewId);
       fillTable();
   }
   else {
     // KLS 6/07
     // NOTE: the table GetSelectedRows appears NOT to return the right
     // thing if extended selection is used.  Use the models getSelection
     // method instead (hand maintained list).
     Command *cmd = 0;
     if (id == MAKERES) {
         cmd = new MakeResidueCmd("Make into residue",&getFW().getSceneGraph());
     } else if (id == RENUMBER) {
         cmd = new RenumberResiduesCmd("Renumber residues",&getFW().getSceneGraph());
     } else if (id == DELETERES) {
         set<int> rows = p_table->getSelectedRows();
         if (rows.size() == 0) {
            getFW().showMessage("Please select residues to delete.",true);
         } else {
            vector<int> todel;
            for (set<int>::iterator setIt = rows.begin();
                  setIt != rows.end();
                  setIt++) {
               todel.push_back(rowToResIndex(*setIt));
            }
            // If this is not done, sporadic crash if atom selection
            // has atoms deleted by the DeleteResidueCmd upon redraw
            cmd = new SelectCmd("Select", &getFW().getSceneGraph(),getId());
            cmd->getParameter("clear")->setBoolean(true);
            getFW().execute(cmd);

            cmd = new DeleteResidueCmd("Delete residue(s)",
                  &getFW().getSceneGraph());
            cmd->getParameter("residues")->setIntegerList(todel);
         }
     } else if (id == INSERTRES) {
         wxArrayInt rows = p_table->getGrid()->GetSelectedRows();
         if (rows.GetCount() == 0 || rows.GetCount() > 1) {
            getFW().showMessage("Please select a single residue to insert before.",true);
         } else {
            int row = rowToResIndex(rows.Item(0));
            cmd = new InsertResidueCmd("Insert residue",
                  &getFW().getSceneGraph());
            cmd->getParameter("index")->setInteger(row);
         }
     } else if (id == APPENDRES) {
         wxArrayInt rows = p_table->getGrid()->GetSelectedRows();
         if (rows.GetCount() == 0 || rows.GetCount() > 1) {
            getFW().showMessage("Please select a single residue to append to.",true);
         } else {
            int row = rowToResIndex(rows.Item(0));
            cmd = new InsertResidueCmd("Append residue",
                  &getFW().getSceneGraph());
            cmd->getParameter("index")->setInteger(row+1);
         }
     } else if (id == VALIDATE) {
        cmd = new ValidateStrCmd("Validate Structure",&getFW().getSceneGraph());
     } else if (id == ADDH) {
        cmd = new AddSegmentHCmd("Add Hydrogens", &getFW().getSceneGraph());
     } else if (id == ASSIGNRESATOMS) {
         set<int> rows = p_table->getSelectedRows();
         int row = 0;  // default if only one row but none selected
         if (rows.size() > 1) {
            getFW().showMessage("Please select a single residue.",true);
         } else if (rows.size() == 1) {
            row = *rows.begin();
         }
         cmd = new AssignResidueAtomsCmd("Assign residue atoms", &getFW().getSceneGraph());
         cmd->getParameter("residue")->setInteger(row);
     } else if (id == CONFIGURE) {
        WxFFDataConfig dialog(this);
        dialog.ShowModal();
     } else if (id == WRITESEG || 
           id == WRITEFRAG) {
        set<int> rows = p_table->getSelectedRows();
        int numRows = p_table->getRowCount();

        int therow = 0;
        if (numRows > 1 && rows.size() == 0) {
           getFW().showMessage("Please select a single residues to write.",true);
        } else {
           if (numRows > 1) therow = rowToResIndex(*rows.begin());
           Fragment *frag = getFW().getSceneGraph().getFragment() ;

           const vector<Residue*>& residues = frag->residues();
           Residue* res = residues[therow];
           string filename = res->name().c_str();

           // remove any white space from begining and end of residue name
           STLUtil::stripLeadingAndTrailingWhiteSpace(filename);

           if (id == WRITESEG) {
              filename += ".sgm";
              if (p_segdir != "") {
                 SFile dirfile(p_segdir.c_str());
                 if (!dirfile.exists()) p_segdir = Ecce::realUserHome();
              } else {
                 p_segdir = Ecce::realUserHome();
              }
              ewxFileDialog dlg(this,"ECCE Write Segment File", 
                    p_segdir, filename.c_str(),
                    "*.*", wxFD_SAVE|wxFD_OVERWRITE_PROMPT);
              dlg.SetFilename(filename.c_str());
              if (dlg.ShowModal() == wxID_OK) {
                 string path = dlg.GetPath().c_str();
                 SFile sfile(path);
                 p_segdir = sfile.pathroot();

                 SegFactory segfactory;
                 EcceURL url(path); 
                 const FileEDSI furl(url);
                 if (!segfactory.writeSegment(res,furl,frag)) {
                    /*
                    getFW().showMessage(
                          "Could not create segment file. Check that all "
                          "atoms have been assigned atom types.", true);
                    */
                    ewxMessageDialog errdlg(this, 
                       "Could not create segment file. "
                       "Use the Validate option to validate the system "
                       "and make sure that all atoms have been assigned "
                       "atom types.",
                       "Write Segment Error",wxOK);
                    errdlg.ShowModal();
                 }

              }
              dlg.Destroy();

           } else { // WRITEFRAG
              filename += ".frg";
              if (p_fragdir != "") {
                 SFile dirfile(p_fragdir.c_str());
                 if (!dirfile.exists()) p_fragdir = Ecce::realUserHome();
              } else {
                 p_fragdir = Ecce::realUserHome();
              }
              ewxFileDialog dlg(this,"ECCE Write Fragment File", 
                    p_fragdir, filename.c_str(),
                    "*.*", wxFD_SAVE|wxFD_OVERWRITE_PROMPT);

              dlg.SetFilename(filename.c_str());
              if (dlg.ShowModal() == wxID_OK) {
                 string path = dlg.GetPath().c_str();
                 SFile sfile(path);
                 p_fragdir = sfile.pathroot();
                 SegFactory segfactory;

                 EcceURL url(path); 
                 const FileEDSI furl(url);
                 if (!segfactory.writeFragment(res, furl, frag)) {
                    ewxMessageDialog errdlg(this, 
                       "Could not create fragment file. "
                       "Use the Validate option to validate the system "
                       "and make sure that all atoms have been assigned "
                       "atom types.",
                       "Write Fragment Error",wxOK);
                    errdlg.ShowModal();
                 }
                    /*
                    getFW().showMessage( "Could not create fragment file. "
                       "Check that all atoms " "have been assigned atom types.",
                          true);
                    */

              }
           }
        }
     }

     if (cmd) {
        wxBusyCursor busy;
        getFW().execute(cmd);
     }
   }


   event.Skip();
}



bool ResidueTable::sortResidueRows(const GridAtomElement *e1, const GridAtomElement *e2)
{
  bool ret = false;

  if (const_cast<GridAtomElement*>(e1)->getIndex() < const_cast<GridAtomElement*>(e2)->getIndex()) {
    ret = true;
  }

  return ret;
}


/**
 * Implement column sorting ourselves so we can turn of the event handling that
 * gets triggered by restoring selection after the sort.
 * Don't turn off the tables handling or selection data members will be out 
 * of date.
 */
void ResidueTable::OnGridLabelLeftClick(wxGridEvent& event)
{
   wxBusyCursor wait;
   int col = event.GetCol();
   p_table->setAscSort(!p_table->getAscSort());
   SetEvtHandlerEnabled(false);
   p_table->sortByCol(col, p_table->getAscSort());
   SetEvtHandlerEnabled(true);

}

/**
 * If use clicks in an edit cell, make it the current residue edting
 * index.
 */
void ResidueTable::OnSelectCell(wxGridEvent& event)
{

   if (event.GetCol() == EDITCOL) {
     setResEditIndex(event.GetRow(), false);
   } else {
      string label = p_table->getGrid()->GetColLabelValue(event.GetCol()).c_str();
      if (label.find("Chain",0) != string::npos) {
         vector<string> chains;
         chains.push_back(p_table->getGrid()->GetCellValue(event.GetRow(),event.GetCol()).c_str());
         SelectCmd *cmd = new SelectCmd("Select chain", &getFW().getSceneGraph()/*,getId()*/); 
         cmd->getParameter("clear")->setBoolean(true);
         cmd->getParameter("chains")->setStringList(chains);
         getFW().execute(cmd);
      }
      if (label.find("Name",0) == 0) {
         vector<string> names;
         names.push_back(p_table->getGrid()->GetCellValue(event.GetRow(),event.GetCol()).c_str());
         SelectCmd *cmd = new SelectCmd("Select residue name", &getFW().getSceneGraph()/*,getId()*/);
         cmd->getParameter("clear")->setBoolean(true);
         cmd->getParameter("residues")->setStringList(names);
         getFW().execute(cmd);
      }
      if (label.find("Status",0) == 0) {
         string name;
         name = p_table->getGrid()->GetCellValue(event.GetRow(),event.GetCol()).c_str();
         SelectCmd *cmd = new SelectCmd("Select residue status", &getFW().getSceneGraph()/*,getId()*/);
         cmd->getParameter("clear")->setBoolean(true);
         cmd->getParameter("resstatus")->setString(name);
         getFW().execute(cmd);
      }
   }
   Refresh();  // sometimes seems necessary to make checkmark disappear
   event.Skip();
}



/**
 * First turn off editing of existing resIndex (if set).  Then
 * turn on editing of the new resIndex.
 * @param onOnly for external events, true means this cannot be a stop editing
 *          event if the index is the same as current
 */
void ResidueTable::setResEditIndex(int row, bool onOnly)
{
   GridAtomElement *element = 0;
   Fragment *frag = getFW().getSceneGraph().getFragment();

   int prevIndex = frag->editResidueIndex();
   element = dynamic_cast<GridAtomElement*>(p_table->getElement(row,EDITCOL));
   int resIndex = element->getIndex();


   // First act on the new edit index 
   // Note true/false get turned into 1/0 in the lower layers
   bool isSet = element->getValue() == "1";
   if (isSet) {
      if (!onOnly) {
         element->setValue("false");
         frag->editResidueMode(false,-1);
      }
   } else {
      element->setValue("true");
      frag->editResidueMode(true,element->getIndex());
   }

   // Now uncheck any residue previously being editing because this
   // is radio behavior
   // Have to translate prevIndex into the correct table row
   if (prevIndex >= 0 ) {
      if (prevIndex != resIndex || (prevIndex == resIndex && !onOnly)) {
         int resindex = resIndexToRow(prevIndex);
         element = dynamic_cast<GridAtomElement*>(p_table->getElement(resindex,EDITCOL));
         element->setValue("false");
      }
   }
}

void ResidueTable::eventMCB(const Event& event)
{
   /** Only process events I didn't send */
   if (event.getSenderId() == getId()) return;

   if (!isVisible()) return;

   // update UI
   if (event.getName() == "UpdateUI") {
      fillTable();
   }

   // Residue Changed
   if (event.getName() == "ResidueChange") {
      fillTable();
   }

   if (event.getName() == "VisibilityChange") {
      fillTable();
   }

   if (event.getName() == "ResEditIndexChange") {
      int resIndex = -1;
      sscanf(event.getValue().c_str(),"%d",&resIndex);
      int row = resIndexToRow(resIndex);
      setResEditIndex(row, true);
      p_table->getGrid()->MakeCellVisible(row,0);
      // Required to get the table to show the change
      Refresh();
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

void ResidueTable::restoreSettings(wxConfig * config)
{
   ResourceDescriptor rd = ResourceDescriptor::getResourceDescriptor();
   int view = -1;
   if (config->Read("/ResidueTable/View", &view) && view > 0) {
     p_viewId = view;
     p_atomView = rd.getResidueView(p_viewId);
     //fillTable();
   } else {
     multimap<int, AtomView*> residueViews = rd.getResidueViews();
     p_atomView = residueViews.begin()->second;
   }

  int sortCol = 0;
  if (config->Read("/ResidueTable/SortCol", &sortCol)) {
    p_table->setSortCol(sortCol);
  }

  bool choice = true;
  if (config->Read("/ResidueTable/IsAscSort", choice)) {
    p_table->setAscSort(choice);
  }

  p_segdir = config->Read("/ResidueTable/SegDir", "");
  p_fragdir = config->Read("/ResidueTable/FragDir", "");
}

void ResidueTable::saveSettings(wxConfig * config)
{
   config->Write("/ResidueTable/View", p_viewId);
   config->Write("/ResidueTable/SortCol", p_table->getSortCol());
   config->Write("/ResidueTable/IsAscSort", p_table->getAscSort());
   config->Write("/ResidueTable/SegDir", p_segdir);
   config->Write("/ResidueTable/FragDir", p_fragdir);
}

