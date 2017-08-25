#include "util/BrowserHelp.H"
#include "util/JMSMessage.H"
#include "util/JMSPublisher.H"
#include "util/StringConverter.H"
#include "util/StringTokenizer.H"

#include "tdat/Fragment.H"
#include "tdat/MetaPotential.H"
#include "tdat/MetaPotentialModel.H"

#include "dsm/EDSIFactory.H"
#include "dsm/Resource.H"
#include "dsm/ResourceType.H"

#include "wxgui/ewxGrid.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/GridModel.H"
#include "wxgui/WxFeedback.H"
#include "wxgui/WxGridView.H"
#include "wxgui/WxJMSMessageDispatch.H"

#include "MetaEd.H"
#include "MetaPotentials.H"



/** Used to subscribe to messaging, publish, identify */
static const string WHOAMI = METADYNAMICS;


IMPLEMENT_CLASS(MetaPotentials, wxDialog)


/*!
 * MetaPotentials constructors
 */
MetaPotentials::MetaPotentials(MetaEd *metaed,
                                 wxWindow* parent,
                                 wxWindowID id,
                                 const wxString& caption,
                                 const wxPoint& pos,
                                 const wxSize& size,
                                 long style ):
  MetaPotentialsGUI(parent, id, caption, pos, size, style)
{
  initMessaging();

  connectEventHandlers();

  p_metaEd = metaed;

  p_bondMetaDynamicsPotential = 0;
  p_angleMetaDynamicsPotential = 0;
  p_selectedTable = 0;

  p_bondPotentialSizer->SetMinSize(200, 40);
  p_anglePotentialSizer->SetMinSize(200, 40);

  p_visibleRows = 5;

  createTables();

  initDisplay();

  GetSizer()->Layout();
  GetSizer()->SetSizeHints(this);
}


MetaPotentials::~MetaPotentials( )
{
  if (p_msgDispatcher != 0)
    delete p_msgDispatcher;
}


void MetaPotentials::createTables() 
{
  createBondTables();
  createAngleTables();
}


void MetaPotentials::clearTables()
{
  // Clear model and redisplay tables
  MetaPotentialModel *model = getModel();
  if (model && model->size() > 0) {
    model->clear();
    notify();
  }
  initDisplay();
}


void MetaPotentials::hideTable(WxGridView *table, wxSizer *container)
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


void MetaPotentials::initConstraintTable(WxGridView *table)
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


void MetaPotentials::setTableSize(WxGridView *table) 
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


void MetaPotentials::createBondTables()
{
  // Bond Potentials
  p_bondMetaDynamicsPotential = new WxGridView(this, wxID_ANY, METADYNAMICS);
  p_bondPotentialSizer->Add(p_bondMetaDynamicsPotential, 0, wxEXPAND, 0);
  GridModel *gridModel = new GridModel();
  gridModel->InsertCols(0, 5);
  p_bondMetaDynamicsPotential->setModel(gridModel);
  p_bondMetaDynamicsPotential->setColLabel(0, "Atom 1");
  p_bondMetaDynamicsPotential->setColLabel(1, "Atom 2");
  p_bondMetaDynamicsPotential->setColLabel(2, "W (au)");
  p_bondMetaDynamicsPotential->setColLabel(3, "Sigma (au)");
  p_bondMetaDynamicsPotential->setColLabel(4, "Relative Error");
  initConstraintTable(p_bondMetaDynamicsPotential);
}


void MetaPotentials::createAngleTables()
{
  // Angle Potentials
  p_angleMetaDynamicsPotential = new WxGridView(this, wxID_ANY, METADYNAMICS);
  p_anglePotentialSizer->Add(p_angleMetaDynamicsPotential, 0, wxEXPAND, 0);
  GridModel *gridModel = new GridModel();
  gridModel->InsertCols(0, 6);
  p_angleMetaDynamicsPotential->setModel(gridModel);
  p_angleMetaDynamicsPotential->setColLabel(0, "Atom 1");
  p_angleMetaDynamicsPotential->setColLabel(1, "Atom 2");
  p_angleMetaDynamicsPotential->setColLabel(2, "Atom 3");
  p_angleMetaDynamicsPotential->setColLabel(3, "W (au)");
  p_angleMetaDynamicsPotential->setColLabel(4, "Sigma (rad.)");
  p_angleMetaDynamicsPotential->setColLabel(5, "Relative Error");
  initConstraintTable(p_angleMetaDynamicsPotential);
}


/*
 * Redisplay all tables
 */
void MetaPotentials::initDisplay(bool isEnabled)
{
  // Get model
  MetaPotentialModel *model = getModel();

  // Display tables
  if (model != 0) {
    displayBondMetaDynamicsPotential(model);
    displayAngleMetaDynamicsPotential(model);
  }
  else {
    hideTable(p_bondMetaDynamicsPotential, p_bondPotentialSizer);
    hideTable(p_angleMetaDynamicsPotential, p_anglePotentialSizer);
  }
    

  // Enable/Disable buttons
  if (isEnabled) {
    FindWindow(ID_METADYNAMICS_POTENTIAL_BUTTON)->Enable();
    FindWindow(ID_GEOM_REMOVE_BUTTON)->Enable();
    FindWindow(ID_GEOM_CLEAR_ALL_BUTTON)->Enable();

    p_bondMetaDynamicsPotential->Enable();
    p_angleMetaDynamicsPotential->Enable();
  }
  else {
    FindWindow(ID_METADYNAMICS_POTENTIAL_BUTTON)->Disable();
    FindWindow(ID_GEOM_REMOVE_BUTTON)->Disable();
    FindWindow(ID_GEOM_CLEAR_ALL_BUTTON)->Disable();

    p_bondMetaDynamicsPotential->Disable();
    p_angleMetaDynamicsPotential->Disable();
  }

  // Fix layout
  GetSizer()->Layout();
  GetSizer()->SetSizeHints(this);
}


void MetaPotentials::notify() 
{
  p_metaEd->enableSave();
}


MetaPotentialModel *MetaPotentials::getModel()
{
  MetaPotentialModel *ret = 0;
  Fragment *frag = p_metaEd->getFrag();

  if (frag) {
     ret = frag->getPotentials();
  }

  return ret;
}


void MetaPotentials::displayBondMetaDynamicsPotential(MetaPotentialModel *model)
{
  vector<MetaPotential*> *potentials = model->get2SitePotentials();
  displayPotentials(p_bondPotentialSizer, 
                    p_bondMetaDynamicsPotential, 
                    *potentials);
  delete potentials;
}


void MetaPotentials::displayAngleMetaDynamicsPotential(MetaPotentialModel *model)
{
  vector<MetaPotential*> *potentials = model->get3SitePotentials();
  displayPotentials(p_anglePotentialSizer, 
                    p_angleMetaDynamicsPotential, 
                    *potentials);
  delete potentials;
}


void MetaPotentials::displayPotentials(wxSizer *container,
                                        WxGridView *table,
                                        vector<MetaPotential*>& potentials)
{
  char buf[32];
  GridModel *gridModel = table->getModel();  

  // Clear table
  gridModel->DeleteRows(0, gridModel->GetNumberRows());

  // Clear container
  table->Show(false);
  container->Detach(table);
  container->Clear(true);


  if (potentials.size() > 0) {

    for (int idx=0; idx<potentials.size(); idx++) {

      gridModel->InsertRows(idx);

      MetaPotential *potential = potentials[idx];
      vector<int> *atoms = potential->getAtoms();
      vector<double> *params = potential->getParams();

      // Construct a name.  Make sure to start counting at 1, not 0
      int adx;
      for (adx=0; adx<atoms->size(); adx++) {
        sprintf(buf, "%d", (*atoms)[adx]+1);
//        sprintf(buf, "%s%d", (*atoms)[adx]->atomicSymbol().c_str(),
//            (*atoms)[adx]->index()+1);
        GridElement *atomCell = new GridElement(buf, new GridProperty(GridProperty::PT_CDATA));
        gridModel->SetValueAsGridElement(idx, adx, atomCell);

      }
      for (adx=0; adx<2; adx++) {
        sprintf(buf, "%lf", (*params)[adx]);
        GridElement *valCell = new GridElement(buf, new GridProperty(GridProperty::PT_FLOAT));
        gridModel->SetValueAsGridElement(idx, adx+atoms->size(), valCell);
      }  
      // Add relative error (This is completely hardwired and assumes that the
      // number of parameters = 2 and that the defaults are 0.00005 and 0.1
      double relErr = sqrt((*params)[0]*(*params)[1]/(0.00005*0.1));
      sprintf(buf, "%lf", relErr);
      GridElement *valCell = new GridElement(buf, new GridProperty(GridProperty::PT_FLOAT));
      gridModel->SetValueAsGridElement(idx, 2+atoms->size(), valCell);

      delete atoms;
      delete params;
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

  GetSizer()->Layout();
  GetSizer()->SetSizeHints(this);
}


void MetaPotentials::requestBuilder(const string& actionTopic) 
{
  string url = p_metaEd->getContext();

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
void MetaPotentials::OnCloseWindow( wxCloseEvent& event )
{
  this->Show(false);
}


/**
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_METADYNAMICS_POTENTIAL_BUTTON
 */
void MetaPotentials::OnPotentialButtonClick( wxCommandEvent& event )
{
  requestBuilder("getselection");
  event.Skip();
}


/**
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_GEOM_REMOVE_BUTTON
 */
void MetaPotentials::OnRemoveButtonClick( wxCommandEvent& event )
{
  if (p_selectedTable != 0) {
    MetaPotentialModel *model = getModel();
    vector<MetaPotential*> *constraints = getSelectedTableConstraints();
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


vector<MetaPotential*> *MetaPotentials::getSelectedTableConstraints()
{
  vector<MetaPotential*> *ret = 0;
  MetaPotentialModel *model = getModel();

  if (p_bondMetaDynamicsPotential->GetId() == p_selectedTable->GetId()) {
    ret = model->get2SitePotentials();
  }
  if (p_angleMetaDynamicsPotential->GetId() == p_selectedTable->GetId()) {
    ret = model->get3SitePotentials();
  }

  return ret;
}


/**
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_GEOM_CLEAR_ALL_BUTTON
 */
void MetaPotentials::OnClearAllButtonClick( wxCommandEvent& event )
{
  clearTables();
  p_selectedTable = 0;
  event.Skip();
}


/**
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_GEOM_CLOSE_BUTTON
 */
void MetaPotentials::OnCloseButtonClick( wxCommandEvent& event )
{
  wxCloseEvent evt;
  OnCloseWindow(evt);
}


/**
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_GEOM_CLOSE_BUTTON
 */
void MetaPotentials::OnHelpButtonClick( wxCommandEvent& event )
{
  BrowserHelp help;
  help.showPage(help.URL("HomeFallback"));
}


void MetaPotentials::initMessaging()
{
  p_msgDispatcher = new WxJMSMessageDispatch(WHOAMI, false);

  p_msgDispatcher->subscribe("ecce_getselection",
       this, (wxJmsCBFunc)&MetaPotentials::selectionMCB);

  p_msgDispatcher->startSubscriber();
  p_msgDispatcher->registerTopShell(this);
  p_msgDispatcher->notifyReady();
}


void MetaPotentials::selectionMCB(JMSMessage& msg)
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
  MetaPotentialModel *model = getModel();
  if ((model) && (numAtoms > 0)) {

    try {
          p_metaEd->getFeedback()->setMessage("Create meta-Potential operation", WxFeedback::INFO);
          if (model->addPotential(highlight)) {
            p_metaEd->getFeedback()->setMessage("Potential added", WxFeedback::INFO);
          }
          if (numAtoms == 2) {
            displayBondMetaDynamicsPotential(model);
          }
          if (numAtoms == 3) {
            displayAngleMetaDynamicsPotential(model);
          }

      notify();

    }
    catch (InvalidException& ex) {
      p_metaEd->getFeedback()->setMessage(ex.what(), WxFeedback::ERROR);
    }

  }
}

void MetaPotentials::connectEventHandlers()
{
    // Left click on a table cell.
    this->Connect(wxEVT_GRID_CELL_LEFT_CLICK,
                  wxGridEventHandler(MetaPotentials::gridCellLeftClick));

    // Value changed
    this->Connect(wxEVT_GRID_CELL_CHANGE,
                  wxGridEventHandler(MetaPotentials::gridCellChange));
}


void MetaPotentials::gridCellLeftClick(wxGridEvent& event)
{
  ewxGrid *table = (ewxGrid *)(event.GetEventObject());
  p_selectedTable = (WxGridView*)table->GetParent();

  if (p_bondMetaDynamicsPotential->GetId() != p_selectedTable->GetId()) {
    p_bondMetaDynamicsPotential->getGrid()->ClearSelection();
  }
  if (p_angleMetaDynamicsPotential->GetId() != p_selectedTable->GetId()) {
    p_angleMetaDynamicsPotential->getGrid()->ClearSelection();
  }
  event.Skip();
}


void MetaPotentials::gridCellChange(wxGridEvent& event)
{
  WxGridView *table = (WxGridView*)(((wxWindow*)(event.GetEventObject()))->GetParent());

  if (p_selectedTable != 0) {
    if (table->GetId() == p_selectedTable->GetId()) {
      vector<MetaPotential*> *potentials = getSelectedTableConstraints();
      if (potentials != 0) {
        GridElement *element =
          dynamic_cast<GridElement*>(table->getElement(event.GetRow(), event.GetCol()));
        if (element != 0) {
          double val;
          StringConverter::toDouble(element->getValue().c_str(),val);
          MetaPotential *potential = (*potentials)[event.GetRow()];
          int natoms = potential->getNumAtoms();
          potential->setPotentialValue(val,event.GetCol()-natoms);
          vector<double> *params = potential->getParams();
          // Another hardwired estimate of relative error
          double relErr = sqrt((*params)[0]*(*params)[1]/(0.00005*0.1));
          GridElement *element2 =
            dynamic_cast<GridElement*>(table->getElement(event.GetRow(), natoms+2));
          char buf[32];
          sprintf(buf, "%lf", relErr);
          element2->setValue(buf);
          delete params;
          notify();
        }
        delete potentials;
      }
    }
  }
}



