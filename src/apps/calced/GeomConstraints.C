#include "util/BrowserHelp.H"
#include "util/JMSMessage.H"
#include "util/JMSPublisher.H"
#include "util/StringConverter.H"
#include "util/StringTokenizer.H"

#include "tdat/Fragment.H"
#include "tdat/GeomConstraint.H"
#include "tdat/GeomConstraintModel.H"
#include "tdat/GeomConstraintRules.H"
#include "tdat/GeomRestraint.H"

#include "dsm/EDSIFactory.H"
#include "dsm/Resource.H"
#include "dsm/ResourceType.H"

#include "wxgui/ewxGrid.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/GridModel.H"
#include "wxgui/WxFeedback.H"
#include "wxgui/WxGridView.H"
#include "wxgui/WxJMSMessageDispatch.H"

#include "CalcEd.H"
#include "GeomConstraints.H"



/** Used to subscribe to messaging, publish, identify */
static const string WHOAMI = GEOMCONSTRAINTS;


IMPLEMENT_CLASS(GeomConstraints, wxDialog)


/*!
 * GeomConstraints constructors
 */
GeomConstraints::GeomConstraints(CalcEd *calced,
                                 wxWindow* parent,
                                 wxWindowID id,
                                 const wxString& caption,
                                 const wxPoint& pos,
                                 const wxSize& size,
                                 long style ):
  GeomConstraintsGUI(parent, id, caption, pos, size, style)
{
  initMessaging();

  connectEventHandlers();

  p_calcEd = calced;

  p_torsionConstraints = 0;
  p_angleConstraints = 0;
  p_bondConstraints = 0;
  p_torsionRestraints = 0;
  p_angleRestraints = 0;
  p_bondRestraints = 0;
  p_fixedAtomConstraints = 0;
  p_selectedTable = 0;

  p_torsionConstraintSizer->SetMinSize(200, 40);
  p_angleConstraintSizer->SetMinSize(200, 40);
  p_bondConstraintSizer->SetMinSize(200, 40);
  p_torsionConstraintSizer->SetMinSize(200, 40);
  p_angleConstraintSizer->SetMinSize(200, 40);
  p_bondConstraintSizer->SetMinSize(200, 40);
  p_fixedAtomSizer->SetMinSize(200, 40);

  p_visibleRows = 5;

  createTables();

  initDisplay();

  GetSizer()->Layout();
  GetSizer()->SetSizeHints(this);
}


GeomConstraints::~GeomConstraints( )
{
  if (p_msgDispatcher != 0)
    delete p_msgDispatcher;
}


void GeomConstraints::createTables() 
{
  createBondTables();
  createAngleTables();
  createTorsionTables();
  createFixedAtomTable();
}


void GeomConstraints::clearTables()
{
  // Clear model and redisplay tables
  GeomConstraintModel *model = getModel();
  if (model && model->size() > 0) {
    model->clear();
    notify();
  }
  initDisplay();
}


void GeomConstraints::hideTable(WxGridView *table, wxSizer *container)
{
  if (table->IsShown()) {

    GridModel *gridModel = table->getModel();  

    // Clear Table
    gridModel->DeleteRows(0, gridModel->GetNumberRows());

    // Clear container
    table->Show(false);
    container->Detach(table);
    container->Clear();

    ewxStaticText *label = new ewxStaticText(this, wxID_STATIC, " ");
    container->Add(label, 0, wxALIGN_CENTER_VERTICAL);

  }

  GetSizer()->Layout();
  GetSizer()->SetSizeHints(this);
}


void GeomConstraints::initConstraintTable(WxGridView *table)
{
  GridModel *model = table->getModel();
  ewxGrid *grid = dynamic_cast<ewxGrid*>(table->getGrid());
  int cols = model->GetNumberCols(); 
  int rows = model->GetNumberRows();

  if ((cols > 0) && (rows > 0)) {
    for (int i = 0; i < rows; i++) {
      if ((i % 2) == 0) {
        wxGridCellAttr *rowAttr = new wxGridCellAttr();
        rowAttr->SetBackgroundColour(grid->getHightLightRowColor());
        grid->SetRowAttr(i, rowAttr);
      }
      else {
        wxGridCellAttr *rowAttr = new wxGridCellAttr();
        grid->SetRowAttr(i, rowAttr);
      }
      if (i == 0) {
        for (int j = 0; j < cols; j++) {
          GridElement *colElement = model->GetValueAsGridElement(i, j);
          if (colElement != 0) {
            wxGridCellAttr *colAttr = new wxGridCellAttr();
            switch (colElement->getPropertyType()) {
              case GridProperty::PT_CDATA:
                colAttr->SetReadOnly();
                colAttr->SetAlignment(wxALIGN_LEFT, wxALIGN_CENTER);
                break;
              case GridProperty::PT_FLOAT:
                colAttr->SetRenderer(new wxGridCellStringRenderer);
                colAttr->SetEditor(new wxGridCellFloatEditor);
                colAttr->SetAlignment(wxALIGN_RIGHT, wxALIGN_CENTER);
                break;              
              default:
                colAttr->SetAlignment(wxALIGN_LEFT, wxALIGN_CENTER);
                colAttr->SetReadOnly();
                break;
            }
            grid->SetColAttr(j, colAttr);
          }
        }
      }
    }
  }

  table->enableGridLines(true);
  table->getGrid()->ClearSelection();
  table->enableLabelClickProcessing(false);

  //setTableSize(table);
}


void GeomConstraints::initFixedAtomTable()
{
  GridModel *model = p_fixedAtomConstraints->getModel();
  ewxGrid *grid = dynamic_cast<ewxGrid*>(p_fixedAtomConstraints->getGrid());
  int cols = model->GetNumberCols(); 
  int rows = model->GetNumberRows();

  if ((cols > 0) && (rows > 0)) {
    for (int i = 0; i < rows; i++) {
      if ((i % 2) == 0) {
        wxGridCellAttr *rowAttr = new wxGridCellAttr();
        rowAttr->SetBackgroundColour(grid->getHightLightRowColor());
        grid->SetRowAttr(i, rowAttr);
      }
      else {
        wxGridCellAttr *rowAttr = new wxGridCellAttr();
        grid->SetRowAttr(i, rowAttr);
      }
      if (i == 0) {
        for (int j = 0; j < cols; j++) {
          GridElement *colElement = model->GetValueAsGridElement(i, j);
          if (colElement != 0) {
            wxGridCellAttr *colAttr = new wxGridCellAttr();
            colAttr->SetReadOnly();
            grid->SetColAttr(j, colAttr);
          }
        }
      }
    }
  }

  p_fixedAtomConstraints->enableGridLines(true);
  p_fixedAtomConstraints->getGrid()->ClearSelection();
  p_fixedAtomConstraints->enableLabelClickProcessing(false);

  //setTableSize(p_fixedAtomConstraints);

}


void GeomConstraints::setTableSize(WxGridView *table) 
{
  if (table->getRowCount() > 0) {
    int rows = (p_visibleRows <= table->getRowCount()) ? 
                p_visibleRows :
                table->getRowCount();
    int height = 0;
    int labelHeight = table->getColLabelHeight();
    int rowHeight = table->getGrid()->GetGridWindow()->GetCharHeight();

    height = labelHeight + (rowHeight * rows);

    // Set the window's minimum size
    // so sizer will be correct
    wxSize winSize = table->GetSize();
    winSize.SetHeight(height + 50);
    table->SetMinSize(winSize);
    table->SetSize(winSize);
    table->SetMaxSize(winSize);
  }

}


void GeomConstraints::createBondTables()
{
  // Bond Constraints
  p_bondConstraints = new WxGridView(this, wxID_ANY, GEOMCONSTRAINTS);
  p_bondConstraintSizer->Add(p_bondConstraints, 0, wxEXPAND, 0);
  GridModel *gridModel = new GridModel();
  gridModel->InsertCols(0, 3);
  p_bondConstraints->setModel(gridModel);
  p_bondConstraints->setColLabel(0, "Atom1");
  p_bondConstraints->setColLabel(1, "Atom2");
  p_bondConstraints->setColLabel(2, "Length");
  initConstraintTable(p_bondConstraints);

  // Bond Restraints
  p_bondRestraints = new WxGridView(this, wxID_ANY, GEOMCONSTRAINTS);
  p_bondRestraintSizer->Add(p_bondRestraints, 0, wxEXPAND, 0);
  gridModel = new GridModel();
  gridModel->InsertCols(0, 4);
  p_bondRestraints->setModel(gridModel);
  p_bondRestraints->setColLabel(0, "Atom1");
  p_bondRestraints->setColLabel(1, "Atom2");
  p_bondRestraints->setColLabel(2, "Length");
  p_bondRestraints->setColLabel(3, "Spring Constant");
  initConstraintTable(p_bondRestraints);
}


void GeomConstraints::createAngleTables()
{
  // Angle Constraints
  p_angleConstraints = new WxGridView(this, wxID_ANY, GEOMCONSTRAINTS);
  p_angleConstraintSizer->Add(p_angleConstraints, 0, wxEXPAND, 0);
  GridModel *gridModel = new GridModel();
  gridModel->InsertCols(0, 4);
  p_angleConstraints->setModel(gridModel);
  p_angleConstraints->setColLabel(0, "Atom1");
  p_angleConstraints->setColLabel(1, "Atom2");
  p_angleConstraints->setColLabel(2, "Atom3");
  p_angleConstraints->setColLabel(3, "Angle");
  initConstraintTable(p_angleConstraints);

  // Angle Restraints
  p_angleRestraints = new WxGridView(this, wxID_ANY, GEOMCONSTRAINTS);
  p_angleRestraintSizer->Add(p_angleRestraints, 0, wxEXPAND, 0);
  gridModel = new GridModel();
  gridModel->InsertCols(0, 5);
  p_angleRestraints->setModel(gridModel);
  p_angleRestraints->setColLabel(0, "Atom1");
  p_angleRestraints->setColLabel(1, "Atom2");
  p_angleRestraints->setColLabel(2, "Atom3");
  p_angleRestraints->setColLabel(3, "Angle");
  p_angleRestraints->setColLabel(4, "Spring Constant");
  initConstraintTable(p_angleRestraints);
}


void GeomConstraints::createTorsionTables()
{
  // Torsion Constraints
  p_torsionConstraints = new WxGridView(this, wxID_ANY, GEOMCONSTRAINTS);
  p_torsionConstraintSizer->Add(p_torsionConstraints, 0, wxEXPAND, 0);
  GridModel *gridModel = new GridModel();
  gridModel->InsertCols(0, 5);
  p_torsionConstraints->setModel(gridModel);
  p_torsionConstraints->setColLabel(0, "Atom1");
  p_torsionConstraints->setColLabel(1, "Atom2");
  p_torsionConstraints->setColLabel(2, "Atom3");
  p_torsionConstraints->setColLabel(3, "Atom4");
  p_torsionConstraints->setColLabel(4, "Torsion");
  initConstraintTable(p_torsionConstraints);

  // Torsion Restraints
  p_torsionRestraints = new WxGridView(this, wxID_ANY, GEOMCONSTRAINTS);
  p_torsionRestraintSizer->Add(p_torsionRestraints, 0, wxEXPAND, 0);
  gridModel = new GridModel();
  gridModel->InsertCols(0, 6);
  p_torsionRestraints->setModel(gridModel);
  p_torsionRestraints->setColLabel(0, "Atom1");
  p_torsionRestraints->setColLabel(1, "Atom2");
  p_torsionRestraints->setColLabel(2, "Atom3");
  p_torsionRestraints->setColLabel(3, "Atom4");
  p_torsionRestraints->setColLabel(4, "Torsion");
  p_torsionRestraints->setColLabel(5, "Spring Constant");
  initConstraintTable(p_torsionRestraints);
}


void GeomConstraints::createFixedAtomTable()
{
  // Fixed Atom Constraints
  p_fixedAtomConstraints = new WxGridView(this, wxID_ANY, GEOMCONSTRAINTS);
  p_fixedAtomSizer->Add(p_fixedAtomConstraints, 0, wxEXPAND, 0);
  GridModel *gridModel = new GridModel();
  gridModel->InsertCols(0, 4);
  p_fixedAtomConstraints->setModel(gridModel);
  p_fixedAtomConstraints->setColLabel(0, "Atom");
  p_fixedAtomConstraints->setColLabel(1, "X");
  p_fixedAtomConstraints->setColLabel(2, "Y");
  p_fixedAtomConstraints->setColLabel(3, "Z");
  initFixedAtomTable();
}


/*
 * Redisplay all tables
 */
void GeomConstraints::initDisplay(bool isEnabled)
{
  // Get model
  GeomConstraintModel *model = getModel();

  // Display tables
  if (model != 0) {
    model->cleanUnmatchedAtoms();
    displayTorsionConstraints(model);
    displayAngleConstraints(model);
    displayBondConstraints(model);
    displayTorsionRestraints(model);
    displayAngleRestraints(model);
    displayBondRestraints(model);
    displayFixedAtoms(model);
  }
  else {
    hideTable(p_torsionConstraints, p_torsionConstraintSizer);
    hideTable(p_angleConstraints, p_angleConstraintSizer);
    hideTable(p_bondConstraints, p_bondConstraintSizer);
    hideTable(p_torsionRestraints, p_torsionRestraintSizer);
    hideTable(p_angleRestraints, p_angleRestraintSizer);
    hideTable(p_bondRestraints, p_bondRestraintSizer);
    hideTable(p_fixedAtomConstraints, p_fixedAtomSizer);
  }
    

  // Enable/Disable buttons
  if (isEnabled) {
    FindWindow(ID_GEOM_CONSTRAIN_BUTTON)->Enable();
    FindWindow(ID_GEOM_RESTRAIN_BUTTON)->Enable();
    FindWindow(ID_GEOM_FIX_ATOM_BUTTON)->Enable();
    FindWindow(ID_GEOM_REMOVE_BUTTON)->Enable();
    FindWindow(ID_GEOM_CLEAR_ALL_BUTTON)->Enable();

    p_torsionConstraints->Enable();
    p_angleConstraints->Enable();
    p_bondConstraints->Enable();
    p_torsionRestraints->Enable();
    p_angleRestraints->Enable();
    p_bondRestraints->Enable();
    p_fixedAtomConstraints->Enable();
  }
  else {
    FindWindow(ID_GEOM_CONSTRAIN_BUTTON)->Disable();
    FindWindow(ID_GEOM_RESTRAIN_BUTTON)->Disable();
    FindWindow(ID_GEOM_FIX_ATOM_BUTTON)->Disable();
    FindWindow(ID_GEOM_REMOVE_BUTTON)->Disable();
    FindWindow(ID_GEOM_CLEAR_ALL_BUTTON)->Disable();

    p_torsionConstraints->Disable();
    p_angleConstraints->Disable();
    p_bondConstraints->Disable();
    p_torsionRestraints->Disable();
    p_angleRestraints->Disable();
    p_bondRestraints->Disable();
    p_fixedAtomConstraints->Disable();
  }

  // Fix layout
  GetSizer()->Layout();
  GetSizer()->SetSizeHints(this);
}


void GeomConstraints::notify() 
{
  p_calcEd->enableSave();
}


GeomConstraintModel *GeomConstraints::getModel()
{
  GeomConstraintModel *ret = 0;
  Fragment *frag = p_calcEd->getFrag();

  if (frag) {
     ret = frag->getConstraints();
  }

  return ret;
}


void GeomConstraints::filterConstraints(vector<GeomConstraint*>& constraints) 
{
  vector<GeomConstraint*>::iterator it;
  
  it = constraints.begin();
  while (it != constraints.end()) {
    if (dynamic_cast<GeomRestraint*>(*it)) {
      constraints.erase(it);
    }
    else {
      it++;
    }
  }
}


void GeomConstraints::filterRestraints(vector<GeomConstraint*>& constraints) 
{
  vector<GeomConstraint*>::iterator it;
  
  it = constraints.begin();
  while (it != constraints.end()) {
    if (!dynamic_cast<GeomRestraint*>(*it)) {
      constraints.erase(it);
    }
    else {
      it++;
    }
  }
}


void GeomConstraints::displayTorsionConstraints(GeomConstraintModel *model)
{
  vector<GeomConstraint*> *constraints = model->getTorsionConstraints();
  filterConstraints(*constraints);
  displayConstraints(p_torsionConstraintSizer, 
                     p_torsionConstraints, 
                     *constraints, 
                     false,
                     model->getRules().supportsConstrainTorsions());
  delete constraints;
}


void GeomConstraints::displayAngleConstraints(GeomConstraintModel *model)
{
  vector<GeomConstraint*> *constraints = model->getAngleConstraints();
  filterConstraints(*constraints);
  displayConstraints(p_angleConstraintSizer, 
                     p_angleConstraints, 
                    *constraints, 
                    false,
                    model->getRules().supportsConstrainAngles());
  delete constraints;
}


void GeomConstraints::displayBondConstraints(GeomConstraintModel *model)
{
  vector<GeomConstraint*> *constraints = model->getBondConstraints();
  filterConstraints(*constraints);
  displayConstraints(p_bondConstraintSizer, 
                     p_bondConstraints, 
                     *constraints, 
                     false,
                     model->getRules().supportsConstrainBonds());
  delete constraints;
}


void GeomConstraints::displayTorsionRestraints(GeomConstraintModel *model)
{
  vector<GeomConstraint*> *constraints = model->getTorsionConstraints();
  filterRestraints(*constraints);
  displayConstraints(p_torsionRestraintSizer, 
                     p_torsionRestraints, 
                     *constraints, 
                     true,
                     model->getRules().supportsRestrainTorsions());
  delete constraints;
}


void GeomConstraints::displayAngleRestraints(GeomConstraintModel *model)
{
  vector<GeomConstraint*> *constraints = model->getAngleConstraints();
  filterRestraints(*constraints);
  displayConstraints(p_angleRestraintSizer, 
                     p_angleRestraints, 
                     *constraints, 
                     true,
                     model->getRules().supportsRestrainAngles());
  delete constraints;
}


void GeomConstraints::displayBondRestraints(GeomConstraintModel *model)
{
  vector<GeomConstraint*> *constraints = model->getBondConstraints();
  filterRestraints(*constraints);
  displayConstraints(p_bondRestraintSizer, 
                     p_bondRestraints, 
                     *constraints, 
                     true,
                     model->getRules().supportsRestrainBonds());
  delete constraints;
}


void GeomConstraints::displayFixedAtoms(GeomConstraintModel *model)
{
  char buf[32];
  GridModel *gridModel = p_fixedAtomConstraints->getModel();  
  bool isSupported = model->getRules().supportsAtomConstraints();
  vector<GeomConstraint*> *constraints = model->getAtomConstraints();

  // Clear Table
  gridModel->DeleteRows(0, gridModel->GetNumberRows());

  // Clear container
  p_fixedAtomConstraints->Show(false);
  p_fixedAtomSizer->Detach(p_fixedAtomConstraints);
  p_fixedAtomSizer->Clear();

  if (isSupported) {

    if (constraints->size() > 0) {

      for (int idx=0; idx<constraints->size(); idx++) {

         gridModel->InsertRows(idx);

         GeomConstraint *constraint = (*constraints)[idx];
         vector<TAtm*> *atoms = constraint->getAtoms();
         const double *coords = (*atoms)[0]->coordinates();

         // Atom Col.  Make sure to start counting at 1, not 0
         sprintf(buf, "%s%d", (*atoms)[0]->atomicSymbol().c_str(),
                              (*atoms)[0]->index()+1);
         GridElement *atomCell = new GridElement(buf, new GridProperty(GridProperty::PT_CDATA));
         gridModel->SetValueAsGridElement(idx, 0, atomCell);

         // X Col
         sprintf(buf, "%lf", coords[0]);
         GridElement *xCell = new GridElement(buf, new GridProperty(GridProperty::PT_NUMERIC));
         gridModel->SetValueAsGridElement(idx, 1, xCell);

         // Y Col
         sprintf(buf, "%lf", coords[1]);
         GridElement *yCell = new GridElement(buf, new GridProperty(GridProperty::PT_NUMERIC));
         gridModel->SetValueAsGridElement(idx, 2, yCell);

         // Z Col
         sprintf(buf, "%lf", coords[2]);
         GridElement *zCell = new GridElement(buf, new GridProperty(GridProperty::PT_NUMERIC));
         gridModel->SetValueAsGridElement(idx, 3, zCell);

         delete atoms;

      }
      initFixedAtomTable();
      p_fixedAtomSizer->Add(p_fixedAtomConstraints);
      //p_fixedAtomSizer->Add(p_fixedAtomConstraints, 0, wxEXPAND, 0);
      p_fixedAtomConstraints->Show(true);


    }
    else {
      ewxStaticText *label = new ewxStaticText(this, wxID_STATIC, " ");
      p_fixedAtomSizer->Add(label, 0, wxALIGN_CENTER_VERTICAL);
    }
  }
  else {
    ewxStaticText *label = new ewxStaticText(this, wxID_STATIC, "Not Supported");
    p_fixedAtomSizer->Add(label, 0, wxALIGN_CENTER_HORIZONTAL);

  }

  GetSizer()->Layout();
  GetSizer()->SetSizeHints(this);
}


void GeomConstraints::displayConstraints(wxSizer *container,
                                         WxGridView *table,
                                         vector<GeomConstraint*>& constraints,
                                         bool isRestraintTable,
                                         bool isSupported)
{
  char buf[32];
  GridModel *gridModel = table->getModel();  

  // Clear table
  gridModel->DeleteRows(0, gridModel->GetNumberRows());

  // Clear container
  table->Show(false);
  container->Detach(table);
  container->Clear(true);

  if (isSupported) {

    if (constraints.size() > 0) {

      for (int idx=0; idx<constraints.size(); idx++) {

         gridModel->InsertRows(idx);

         GeomConstraint *constraint = constraints[idx];
         vector<TAtm*> *atoms = constraint->getNameOrderedAtoms();

         // Construct a name.  Make sure to start counting at 1, not 0
         for (int adx=0; adx<atoms->size(); adx++) {
           sprintf(buf, "%s%d", (*atoms)[adx]->atomicSymbol().c_str(),
                                (*atoms)[adx]->index()+1);
          GridElement *atomCell = new GridElement(buf, new GridProperty(GridProperty::PT_CDATA));
          gridModel->SetValueAsGridElement(idx, adx, atomCell);
            
         }
         sprintf(buf, "%lf", constraint->getConstraintValue());
         GridElement *valCell = new GridElement(buf, new GridProperty(GridProperty::PT_FLOAT));
         gridModel->SetValueAsGridElement(idx, atoms->size(), valCell);

         if (isRestraintTable) {
           GeomRestraint *restraint = dynamic_cast<GeomRestraint*>(constraint);
           if (restraint) {
             sprintf(buf, "%lf", restraint->getRestraintValue());
             GridElement *scCell = new GridElement(buf, new GridProperty(GridProperty::PT_FLOAT));
             gridModel->SetValueAsGridElement(idx, atoms->size()+1, scCell);
           }
         }
         delete atoms;
      }
      initConstraintTable(table);
      container->Add(table);
      //container->Add(table, 0, wxEXPAND, 0);
      table->Show(true);
    }
    else {
      ewxStaticText *label = new ewxStaticText(this, wxID_STATIC, " ");
      container->Add(label, 0, wxALIGN_CENTER_VERTICAL);
    }
  }
  else {
    ewxStaticText *label = new ewxStaticText(this, wxID_STATIC, "Not Supported");
    container->Add(label, 0, wxALIGN_CENTER_HORIZONTAL);

  }

  GetSizer()->Layout();
  GetSizer()->SetSizeHints(this);
}


void GeomConstraints::requestBuilder(const string& actionTopic) 
{
  string url = p_calcEd->getContext();

  JMSPublisher publisher(WHOAMI);
  JMSMessage *msg = publisher.newMessage();
  msg->addProperty("action","start");
  publisher.publish("ecce_activity",*msg);
  delete msg;

  // Send the start app request to gateway:
  Target gateway(GATEWAY, "");
  msg = publisher.newMessage(gateway);
  // Ensure that this message will carry original sender info for
  // sending point to point responses after startup
  msg->markAsPointToPoint();


  // which builder do we invoke?  find out now!
  Resource *resource = EDSIFactory::getResource(url);
  if (resource) {
    ResourceType *resourceType = resource->getDescriptor();
    if (resourceType) {
      vector<ResourceTool*> tools = resourceType->getTools();
      for (int i = 0; i < tools.size(); i++) {
        string name = tools[i]->getName();
        if (name == BUILDER) {
          publisher.invoke(*msg, name, 0, url, actionTopic);
          break;
        }
      }
    }
  }

  delete msg;
}



/**
 * wxEVT_CLOSE_WINDOW event handler
 */
void GeomConstraints::OnCloseWindow( wxCloseEvent& event )
{
  this->Show(false);
}


/**
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_GEOM_CONSTRAIN_BUTTON
 */
void GeomConstraints::OnConstrainButtonClick( wxCommandEvent& event )
{
  p_selectionType = CONSTRAINT;
  requestBuilder("getselection");
  event.Skip();
}


/**
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_GEOM_RESTRAIN_BUTTON
 */
void GeomConstraints::OnRestrainButtonClick( wxCommandEvent& event )
{
  p_selectionType = RESTRAINT;
  requestBuilder("getselection");
  event.Skip();
}


/**
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_GEOM_FIX_ATOMS_BUTTON
 */
void GeomConstraints::OnFixAtomsButtonClick( wxCommandEvent& event )
{
  p_selectionType = FIXED;
  requestBuilder("getselection");
  event.Skip();
}


/**
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_GEOM_REMOVE_BUTTON
 */
void GeomConstraints::OnRemoveButtonClick( wxCommandEvent& event )
{

  if (p_selectedTable != 0) {
    GeomConstraintModel *model = getModel();
    vector<GeomConstraint*> *constraints = getSelectedTableConstraints();
    if (constraints != 0) {
      set<int> selRows = p_selectedTable->getSelectedRows();
      set<int>::iterator setIt = selRows.begin();
      while (setIt != selRows.end()) {
        model->remove((*constraints)[(*setIt)]);
        setIt++;
      }
      delete constraints;
      p_selectedTable = 0;
      initDisplay();
      notify();
    }
  }
  event.Skip();
}


vector<GeomConstraint*> *GeomConstraints::getSelectedTableConstraints()
{
  vector<GeomConstraint*> *ret = 0;
  GeomConstraintModel *model = getModel();

  if (p_torsionConstraints->GetId() == p_selectedTable->GetId()) {
    ret = model->getTorsionConstraints();
    filterConstraints(*ret);
  }
  if (p_angleConstraints->GetId() == p_selectedTable->GetId()) {
    ret = model->getAngleConstraints();
    filterConstraints(*ret);
  }
  if (p_bondConstraints->GetId() == p_selectedTable->GetId()) {
    ret = model->getBondConstraints();
    filterConstraints(*ret);
  }
  if (p_torsionRestraints->GetId() == p_selectedTable->GetId()) {
    ret = model->getTorsionConstraints();
    filterRestraints(*ret);
  }
  if (p_angleRestraints->GetId() == p_selectedTable->GetId()) {
    ret = model->getAngleConstraints();
    filterRestraints(*ret);
  }
  if (p_bondRestraints->GetId() == p_selectedTable->GetId()) {
    ret = model->getBondConstraints();
    filterRestraints(*ret);
  }
  if (p_fixedAtomConstraints->GetId() == p_selectedTable->GetId()) {
    ret = model->getAtomConstraints();
  }

  return ret;
}


/**
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_GEOM_CLEAR_ALL_BUTTON
 */
void GeomConstraints::OnClearAllButtonClick( wxCommandEvent& event )
{
  clearTables();
  p_selectedTable = 0;
  event.Skip();
}


/**
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_GEOM_CLOSE_BUTTON
 */
void GeomConstraints::OnCloseButtonClick( wxCommandEvent& event )
{
  wxCloseEvent evt;
  OnCloseWindow(evt);
}


/**
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_GEOM_CLOSE_BUTTON
 */
void GeomConstraints::OnHelpButtonClick( wxCommandEvent& event )
{
  BrowserHelp help;
  help.showPage(help.URL("HomeFallback"));
}


void GeomConstraints::initMessaging()
{
  p_msgDispatcher = new WxJMSMessageDispatch(WHOAMI, false);

  p_msgDispatcher->subscribe("ecce_getselection",
       this, (wxJmsCBFunc)&GeomConstraints::selectionMCB);

  p_msgDispatcher->startSubscriber();
  p_msgDispatcher->registerTopShell(this);
  p_msgDispatcher->notifyReady();
}


void GeomConstraints::selectionMCB(JMSMessage& msg)
{
  // Get highlighted atoms 
  int natoms = msg.getIntProperty("numatoms");
  string selection = msg.getProperty("selection");
  vector<int> highlight;
  if (natoms > 0) {
    StringTokenizer tkn(selection);
    for (int i=0; i<natoms; i++) {
      string iatom = tkn.next();
      highlight.push_back(atoi(iatom.c_str()));
    }
  }


  int numAtoms = highlight.size();
  GeomConstraintModel *model = getModel();
  if ((model) && (numAtoms > 0)) {

    try {

      switch (p_selectionType) {
        case CONSTRAINT:
          p_calcEd->getFeedback()->setMessage("CONSTRIANT geometry constraint operation", WxFeedback::INFO);
          if (model->addConstraint(highlight)) {
            p_calcEd->getFeedback()->setMessage("Duplicates were discarded.", WxFeedback::INFO);
          }
          if (numAtoms == 2) {
            displayBondConstraints(model);
          }
          else if (numAtoms == 3) {
            displayAngleConstraints(model);
          }
          else if (numAtoms == 4) {
            displayTorsionConstraints(model);
          }
          else {
            // shouldn't get here
            p_calcEd->getFeedback()->setMessage("Invalid number of atoms selected", WxFeedback::ERROR);
          }
          break;
        case RESTRAINT:
          p_calcEd->getFeedback()->setMessage("RESTRAINT geometry constraint operation", WxFeedback::INFO);
          if (model->addRestraint(highlight)) {
            p_calcEd->getFeedback()->setMessage("Duplicates were discarded.", WxFeedback::INFO);
          }
          if (numAtoms == 2) {
            displayBondRestraints(model);
          }
          else if (numAtoms == 3) {
            displayAngleRestraints(model);
          }
          else if (numAtoms == 4) {
            displayTorsionRestraints(model);
          }
          else {
            // shouldn't get here
            p_calcEd->getFeedback()->setMessage("Invalid number of atoms selected", WxFeedback::ERROR);
          }
          break;
        case FIXED:
          p_calcEd->getFeedback()->setMessage("FIXED geometry constraint operation", WxFeedback::INFO);
          if (model->fixAtoms(highlight)) {
            p_calcEd->getFeedback()->setMessage("Duplicates were discarded.", WxFeedback::INFO);
          }
          displayFixedAtoms(model);
          break;
        default:
          p_calcEd->getFeedback()->setMessage("Unknown geometry constraint operation", WxFeedback::ERROR);
          break;
      }

      notify();

    }
    catch (InvalidException& ex) {
      p_calcEd->getFeedback()->setMessage(ex.what(), WxFeedback::ERROR);
    }

  }

}

void GeomConstraints::connectEventHandlers()
{
    // Left click on a table cell.
    this->Connect(wxEVT_GRID_CELL_LEFT_CLICK,
                  wxGridEventHandler(GeomConstraints::gridCellLeftClick));

    // Value changed
    this->Connect(wxEVT_GRID_CELL_CHANGE,
                  wxGridEventHandler(GeomConstraints::gridCellChange));
}


void GeomConstraints::gridCellLeftClick(wxGridEvent& event)
{
  ewxGrid *table = (ewxGrid *)(event.GetEventObject());
  p_selectedTable = (WxGridView*)table->GetParent();

  if (p_torsionConstraints->GetId() != p_selectedTable->GetId()) {
    p_torsionConstraints->getGrid()->ClearSelection();
  }
  if (p_angleConstraints->GetId() != p_selectedTable->GetId()) {
    p_angleConstraints->getGrid()->ClearSelection();
  }
  if (p_bondConstraints->GetId() != p_selectedTable->GetId()) {
    p_bondConstraints->getGrid()->ClearSelection();
  }
  if (p_torsionRestraints->GetId() != p_selectedTable->GetId()) {
    p_torsionRestraints->getGrid()->ClearSelection();
  }
  if (p_angleRestraints->GetId() != p_selectedTable->GetId()) {
    p_angleRestraints->getGrid()->ClearSelection();
  }
  if (p_bondRestraints->GetId() != p_selectedTable->GetId()) {
    p_bondRestraints->getGrid()->ClearSelection();
  }
  if (p_fixedAtomConstraints->GetId() != p_selectedTable->GetId()) {
    p_fixedAtomConstraints->getGrid()->ClearSelection();
  }

  event.Skip();
}


void GeomConstraints::gridCellChange(wxGridEvent& event)
{
  WxGridView *table = (WxGridView*)(((wxWindow*)(event.GetEventObject()))->GetParent());

  if (p_selectedTable != 0) {
    if (table->GetId() == p_selectedTable->GetId()) {
      vector<GeomConstraint*> *constraints = getSelectedTableConstraints();
      if (constraints != 0) {
        GridElement *element =
          dynamic_cast<GridElement*>(table->getElement(event.GetRow(), event.GetCol()));
        if (element != 0) {
          double val;
          StringConverter::toDouble(element->getValue().c_str(), val);
          GeomConstraint *constraint = (*constraints)[event.GetRow()];
          GeomRestraint *restraint = dynamic_cast<GeomRestraint*>(constraint);
          if ((restraint) && (event.GetCol() == table->getColCount()-1)) {
            restraint->setRestraintValue(val);
          }
          else {
            constraint->setConstraintValue(val);
          }
          notify();
        }
        delete constraints;
      }
    }
  }
}



