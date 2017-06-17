/**
 * @file
 *
 * @author Jeff Daily
 *
 */

#include <iostream>
  using std::ofstream;
  using std::ostream;

#include <stdio.h>

#include "wx/image.h"

#include "util/Preferences.H"
#include "util/PreferenceLabels.H"
#include "util/ResourceUtils.H"
#include "util/TDateTime.H"
#include "util/TempStorage.H"
#include "util/NullPointerException.H"

#include "dsm/EDSIFactory.H"
#include "dsm/MdTask.H"
#include "dsm/Resource.H"
#include "dsm/ResourceProperty.H"
#include "dsm/GridProperty.H"
#include "dsm/ResourceTool.H"
#include "dsm/Session.H"
#include "dsm/ToolPropertyView.H"
#include "dsm/VDoc.H"

#include "wxgui/ewxButton.H"
#include "wxgui/ewxNonBoldLabel.H"
#include "wxgui/ewxTextCtrl.H"
#include "wxgui/EcceTool.H"
#include "wxgui/WxResourceImageList.H"
#include "wxgui/WxResourceTreeItemData.H"
#include "wxgui/WxState.H"

#include "CalcMgrGUI.H"
#include "CalculationContextPanel.H"



/**
 * Constructor with default parameters.
 */
CalculationContextPanel::CalculationContextPanel( CalcMgr * calcMgr,
                                               WxResourceTreeItemData *itemData,
                                               wxWindow *parent,
                                               wxWindowID id,
                                               const wxPoint& pos,
                                               const wxSize& size,
                                               long style ):
  ewxScrolledWindow(parent, id, pos, size, style),
  ContextPanelInterface(calcMgr),
  p_itemData(itemData),
  p_state(NULL),
  p_properties(NULL),
  p_inpProvProps(NULL)
{
  // set the scroll rate of the scrolled window
  SetScrollbars(10, 10, 0, 0);

  // generate the tool portion of the panel
  createControls();

  // update data fields
  refresh();
}



/**
 * Destructor.
 */
CalculationContextPanel::~CalculationContextPanel( )
{
  p_itemData = 0;
}



/**
 * Updates all data fields to the current metadata property values.
 */
void CalculationContextPanel::refresh()
{
  // update the properties first, since this is why refresh is likely being
  // called
  if (dynamic_cast<MdTask*>(p_itemData->getResource()))
  {
    p_inpProvProps = p_itemData->getResource()->getProps(true);
  }
  p_properties = p_itemData->getResource()->getProps();

  // The tools, their properties, and their views are defined in the 
  // ResourceDescriptor class.
  ResourceDescriptor &rd = ResourceDescriptor::getResourceDescriptor();

  // Define and initialize the toolPropertyViews multimap, as well as an
  // iterator to traverse it.
  multimap<int, pair<ResourceTool*,ToolPropertyView*> > toolPropertyViews;
  multimap<int, pair<ResourceTool*,ToolPropertyView*> >::iterator tpvIt;
  toolPropertyViews = rd.getToolPropertyViews(
      p_itemData->getResource()->getDescriptor());

  for (tpvIt = toolPropertyViews.begin();
       tpvIt != toolPropertyViews.end();
       tpvIt++) {

    // get the ResourceProperties for this tool and iterate through them
    multimap<int, ResourceProperty*> properties;
    multimap<int, ResourceProperty*>::iterator propIt;
    properties = tpvIt->second.second->getProperties();

    for (propIt = properties.begin(); propIt != properties.end(); propIt++) {
      setPropValue(propIt->second);
    }
  } 

  // lastly, the annotation is added if one exists
  // annotations don't search input providers since annotations are local to the
  // resource at hand
  wxWindow *annotationWin = FindWindow("annotation");
  ewxTextCtrl *annotationCtrl = dynamic_cast<ewxTextCtrl*>(annotationWin);
  NULLPOINTEREXCEPTION(annotationCtrl, "failed to reacquire annotation window");
  string annotation = "";
  p_properties->findValue(VDoc::getEcceNamespace() + ":annotation", annotation);
  annotationCtrl->SetValue(annotation);

  // get the thumbnail and replace the builder button icon
  // variables to get the jpeg thumbnail resource
  SFile *thumbnail = TempStorage::getTempFile();
  ChemistryTask * chemTask = 0;
  chemTask = dynamic_cast<ChemistryTask *>(p_itemData->getResource());
  if (chemTask) {
    if (chemTask->getThumbnail(thumbnail)) {
      wxWindow *window = this->FindWindow(_T(BUILDER));
      if (window) {
        EcceTool *button = dynamic_cast<EcceTool*>(window);
        if (button) {
          wxBitmap bitmap(_T(thumbnail->path()), 
                          wxBITMAP_TYPE_JPEG);
          if (bitmap.Ok()) { // bitmap data found
            button->setBitMap(bitmap);
          }
        }
      }
    }
  }

  // the temp thumbnail has served its purpose, we can delete it now
  thumbnail->remove();
  delete thumbnail;

  // manage memory
  if (p_properties != 0) delete p_properties;
  if (p_inpProvProps != 0) delete p_inpProvProps;
}



/**
 * Generates the tools and their corresponding data displays.
 */
void CalculationContextPanel::createControls()
{
  // The tools, their properties, and their views are defined in the 
  // ResourceDescriptor class.
  ResourceDescriptor &rd = ResourceDescriptor::getResourceDescriptor();

  // Define and initialize the toolPropertyViews multimap, as well as an
  // iterator to traverse it.
  multimap<int, pair<ResourceTool*,ToolPropertyView*> > toolPropertyViews;
  multimap<int, pair<ResourceTool*,ToolPropertyView*> >::iterator tpvIt;
  toolPropertyViews = rd.getToolPropertyViews(
      p_itemData->getResource()->getDescriptor());

  // generate the main sizer for this panel
  this->SetSizer(new wxBoxSizer(wxVERTICAL));

  // Now generate, for each tool/properties set, a sizer and populate it with
  // a button to represent the tool and a grid of the Resource's metadata
  // associated with the tool.
  wxBoxSizer *sizer;
  for (tpvIt = toolPropertyViews.begin();
       tpvIt != toolPropertyViews.end();
       tpvIt++) {
    pair<ResourceTool*,ToolPropertyView*> pairRT_TPV = tpvIt->second;

    // create the sizer for this tool
    sizer = new wxBoxSizer(wxHORIZONTAL);

    // create the button for this tool and add it to its sizer
    sizer->Add(createToolButton(pairRT_TPV.first),
          0,                        // do not stretch horizontally
          wxALIGN_CENTER_VERTICAL|  // center vertically
          wxALL,                    // make border all around
          1);                       // set border width to 1

    // these variables help format the grid used to display the data
    int rows = 4;       // max number of rows to use in the grid
    int cols = 2;       // max number of columns to use in the grid
    int vgap = 0;       // size of vertical gap between children in the grid
    int hgap = 5;       // size of horizontal gap between children in the grid
    int currentRow = 0; // current row in grid being filled with children
    wxFlexGridSizer *grid = new wxFlexGridSizer(
        rows,   // number of rows to use
        cols,   // number of coluns to use
        vgap,   // size of vertical gap between children
        hgap);  // size of horizontal gap between children

    // get the ResourceProperties for this tool and iterate through them
    multimap<int, ResourceProperty*> properties;
    multimap<int, ResourceProperty*>::iterator propIt;
    properties = pairRT_TPV.second->getProperties();

    for (propIt = properties.begin(); propIt != properties.end(); propIt++) {
      // For the properties grid, increment current row so we know which row we 
      // are currently writing on.
      currentRow++;

      // add the property label to the grid
      grid->Add(createPropLabel(propIt->second),
          0,            // do not stretch,
          wxALIGN_LEFT| // place in left
          wxALIGN_TOP|  //    upper area of grid cell,
          wxALL,        // and make border all around
          1);           // of border with 1

      // add the property container to the grid
      grid->Add(createPropHolder(propIt->second),
          0,            // do not stretch,
          wxALIGN_LEFT| // place in left
          wxALIGN_TOP|  //    upper area of grid cell,
          wxALL,        // and make border all around
          1);           // of border width 1

      // If the current grid sizer is full (reached max number of rows), create
      // a new grid sizer after adding the full one to the current tool/prop
      // sizer
      if (currentRow % rows == 0)
      {
        sizer->Add(grid, 
            0,            // do not stretch,
            wxALIGN_LEFT| // place in left
            wxALIGN_TOP|  //    upper area of tool/prop sizer
            wxALL,        // and make border all around
            1);           // of border width 1

        sizer->Add(30,0); // spacer only in width

        grid = new wxFlexGridSizer(
            rows,   // number of rows to use (see above)
            cols,   // number of coluns to use (see above)
            vgap,   // size of vertical gap between children (see above)
            hgap);  // size of horizontal gap between children (see above)
      }
    } // end of ResourceProperties for loop

    // add the grid sizer we were last filling when we ran out of properties to
    // display in it
    sizer->Add(grid,
        0,            // do not stretch,
        wxALIGN_LEFT| // place in left
        wxALIGN_TOP|  //    upper area of tool/prop sizer
        wxALL,        // and make border all around
        1);           // of border width 1

    // finally, add the completely generated button and metadata grid (contained
    // in their own sizer, remember) to the main panel sizer
    GetSizer()->Add(sizer,
        0,                        // do not stretch,
        wxALIGN_LEFT|             // place as far left as possible,
        wxALIGN_CENTER_VERTICAL|  // center vertically,
        wxALL,                    // and make border all around
        5);                       // of border width 5
  } // end of ToolPropertyView for loop

  // the annotation is added last
  // we used to check for blank annotations, but we wanted to emphasize the use
  // of annotations so now they always appear

  // fill all the space between the tool/prop sizers and the annotation
  GetSizer()->AddStretchSpacer();

  // create the button which launches the edit session
  ewxButton *button = new ewxButton(this,
      // unique id given to connect event handling later
      CalcMgrGUI::wxID_ANNOTATION,    
      "Annotation:",      // button label
      wxDefaultPosition,  // default only because I want to use
      wxDefaultSize,      //    a non-default style next
      wxBU_EXACTFIT);     // size of button is just bigger than label

  // add the button to the main panel sizer
  GetSizer()->Add(button, 
      0,                // do not stretch,
      wxALIGN_LEFT|     // place as far left as possible,
      wxLEFT|           // make a border to the left
      wxTOP,            //    and at the top
      5);               // of border width 5

  // attach an event handler to the button click
  Connect( CalcMgrGUI::wxID_ANNOTATION, 
           wxEVT_COMMAND_BUTTON_CLICKED,
           wxCommandEventHandler( CalculationContextPanel::OnButtonClick) );

  // the text control to display the annotation in
  ewxTextCtrl *anno = new ewxTextCtrl(this, 
      wxID_ANY,           // don't care what id it has,
      "",                 // don't put any default text inside
      wxDefaultPosition,  // default only because I want to use
      wxDefaultSize,      //    a non-default style next
      wxTE_MULTILINE|     // text will display on multiple lines with scrollbar,
      wxTE_READONLY|      // text cannot be edited,
      wxTE_BESTWRAP,      // and wrap long lines the "best" way possible
      wxDefaultValidator, // default only because I want non-default name next
      "annotation");      // the name of the control, should be unique
  // the text control employs its own generic validator internally if one is not
  // used at creation time.  Reset validator to the one we wanted.
  anno->SetValidator(wxDefaultValidator);

  // add the text control to the main sizer
  GetSizer()->Add(anno, 
      0,        // do not stretch vertically,
      wxEXPAND| // but stretch horizontally
      wxALL,    // make a border all around
      5);       // of border width 5
}



/**
 * Given a ResourceTool, create and return a button for the tool app.
 *
 * @param resTool   the ResourceTool to create a button for
 * @return          the wxBitmapButton of the new button
 */
EcceTool* CalculationContextPanel::createToolButton(ResourceTool *resTool)
{
  EcceTool * tool = new EcceTool(this, resTool);
  Connect( resTool->getId(),
           wxEVT_COMMAND_BUTTON_CLICKED,
           wxCommandEventHandler( CalculationContextPanel::OnButtonClick) );

  return tool;
}



/**
 * Given a ResourceProperty, create a named label inside a wxBoxSizer and
 * return it.
 *
 * @param resProp   the ResourceProperty to create a label for
 * @return          the wxBoxSizer containing the new label
 */
wxBoxSizer* CalculationContextPanel::createPropLabel(ResourceProperty *resProp)
{
  // the sizer to return, containing the controls we need
  wxBoxSizer* box = new wxBoxSizer(wxHORIZONTAL);

  // the type of property doesn't matter, all properties have labels
 
  // create the label
  ewxNonBoldLabel *text = new ewxNonBoldLabel(this, 
      wxID_STATIC,              // don't care about the id
      _T(resProp->getLabel()),  // text for use in label
      wxDefaultPosition,        // default only because I want to use a
      wxDefaultSize,            //    unique name next
      0,                        // no special styles set (default)
      _T(resProp->getLabel())); // how we will refer to this window later

  // add label to sizer to return
  box->Add(text,
      0,                        // make horizontally unstretchable
      wxALIGN_LEFT|             // no border, but align left and
      wxALIGN_CENTER_VERTICAL); // center it up/down

  return box;
}



/**
 * Given a ResourceProperty, create a property field (wxBoxSizer) and return it.
 *
 * @param resProp   the ResourceProperty to create property windows for
 * @return          the wxBoxSizer containing the new property windows
 */
wxBoxSizer* CalculationContextPanel::createPropHolder(ResourceProperty *resProp)
{
  // get the type of the property, which determines what controls to create
  GridProperty::PropertyType type = resProp->getType();

  // the sizer to return, containing the controls we need
  wxBoxSizer* box = new wxBoxSizer(wxHORIZONTAL);

  // all property fields contain at least a text portion which occurs last in
  // sizer, so handle all special property cases first
  if (type == GridProperty::PT_RESOURCE_ICON)
  {
    // create bitmap from image list
    wxBitmap codeBitmap(WxResourceImageList::getBitmap(
                        p_itemData->getResource()));

    // create the static bitmap from the above bitmap
    wxStaticBitmap *code = new wxStaticBitmap(this, 
        wxID_STATIC,        // don't really care about the id
        codeBitmap,         // bitmap image to show
        wxDefaultPosition,  // position required explicitly, but we default
        wxDefaultSize,      // default so we can get to "name"
        0,                  // default style
        "codeBitmap");      // so we can find this window later

    // add static bitmap to sizer to return
    box->Add(code,
        0,
        wxALIGN_LEFT|
        wxALIGN_CENTER_VERTICAL);

    box->AddSpacer(5); // spacer
  }
  else if (type == GridProperty::PT_RUNSTATE_ICON)
  {
    p_state = new WxState(this);
    box->Add(p_state,
        0,                        // do not stretch
        wxALIGN_LEFT|             // place in left
        wxALIGN_CENTER_VERTICAL); //    upper corner of sizer

    box->AddSpacer(5); // spacer
  }


  // create the text portion of the property
  ewxStaticText *text = new ewxStaticText(this, 
      wxID_STATIC,              // don't really care about the id
      "",                       // temporary place-holder for real value
      wxDefaultPosition,        // default only because I want to give a
      wxDefaultSize,            //      unique name next
      0,                        // no special styles set (default)
      _T(resProp->getName()));  // how we will refer to this window later

  // add text portion of the property to sizer to return
  box->Add(text,
      0,                        // make horizontally unstretchable
      wxALIGN_LEFT|             // no border, but align left and
      wxALIGN_CENTER_VERTICAL); // center it up/down

  return box;
}



/**
 * Given the ResourceProperty, find both the label and the property windows and
 * update their values if needed.
 *
 * @param resProp   the ResourceProperty to update the display for
 */
void CalculationContextPanel::setPropValue(ResourceProperty* resProp)
{
  // get the type of the property, which determines what controls to create
  GridProperty::PropertyType type = resProp->getType();

  // get the label for this property
  wxWindow *labelWin = FindWindow(_T(resProp->getLabel()));
  ewxNonBoldLabel *label = dynamic_cast<ewxNonBoldLabel*>(labelWin);
  NULLPOINTEREXCEPTION(label, "failed to relocate label window");

  // get the text field associated with the property
  wxWindow *propWin = FindWindow(_T(resProp->getName()));
  ewxStaticText *prop = dynamic_cast<ewxStaticText*>(propWin);
  NULLPOINTEREXCEPTION(prop, "failed to relocate property window");

  // get the property value string for this property from either the 
  // calculation's own set of properties or its input providers, depending on 
  // its setting
  string valuestr = "";
  if (resProp->getReadOnly()) { // use own properties
    p_properties->findValue(resProp->getQName(), valuestr);
  } else {
    p_inpProvProps->findValue(resProp->getQName(), valuestr);
  }

  // some properties will have no value, but we want them to be displayed on the
  // panel anyway, so show the user a place-holder
  if (valuestr == "") {
    if (! resProp->getDisplay()) // do we force the label/prop's display?
    {
      label->Hide();
      prop->Hide();
    }
  } else {
    label->Show();
    prop->Show();
  }

  // the property might need special formatting or other considerations based
  // on its type
  if (type == GridProperty::PT_CDATA)
  {
    prop->SetLabel(valuestr);
  }
  else if (type == GridProperty::PT_DATE_TIME)
  {
    bool success = true;
    Preferences * eccePref = new Preferences(PrefLabels::GLOBALPREFFILE);
    int dateFormat;
    int timeFormat;
    if (! eccePref->getInt(PrefLabels::DATEFORMAT, dateFormat) ||
        ! eccePref->getInt(PrefLabels::TIMEFORMAT, timeFormat))
    {
      success = false;
    }

    // double check that the timedate formats are within the enum bounds
    if ((TDateTime::DateFormat)dateFormat >= TDateTime::AmericanLocale &&
        (TDateTime::DateFormat)dateFormat <= TDateTime::TOTALDATE &&
        (TDateTime::TimeFormat)timeFormat >= TDateTime::HM24 &&
        (TDateTime::TimeFormat)timeFormat <= TDateTime::TOTALTIME)
    {
      success &= true;
    }
    else
    {
      success = false;
    }

    if (success && valuestr != "") {
      TDateTime datetime(valuestr);
      valuestr = datetime.toString((TDateTime::DateFormat)dateFormat,
                                   (TDateTime::TimeFormat)timeFormat);
    }
    prop->SetLabel(valuestr);
  }
  else if (type == GridProperty::PT_CHECK)
  {
    if (valuestr == "true")
    {
      prop->SetLabel("Yes");
    }
    else if (valuestr == "false")
    {
      prop->SetLabel("No");
    }
    else if (valuestr == "Y")
    {
      prop->SetLabel("Spherical");
    }
    else if (valuestr == "N")
    {
      prop->SetLabel("Cartesian");
    }
    else
    {
      prop->SetLabel("");
    }

  }
  else if (type == GridProperty::PT_ICON)
  {
  }
  else if (type == GridProperty::PT_RESOURCE_ICON)
  {
    prop->SetLabel(valuestr);
    // update the resource icon, too
    // needed for ES calc application type changes
    wxWindow *codeWin = FindWindowByName("codeBitmap");
    wxStaticBitmap *codeBitmapWin = dynamic_cast<wxStaticBitmap*>(codeWin);
    wxBitmap codeBitmap(WxResourceImageList::getBitmap(
                        p_itemData->getResource()));
    codeBitmapWin->SetBitmap(codeBitmap);
  }
  else if (type == GridProperty::PT_RUNSTATE_ICON)
  {
    p_state->setRunState(ResourceUtils::stringToState(valuestr));
    prop->SetLabel(p_itemData->getName());
  }
  else if (type == GridProperty::PT_UNDEFINED)
  {
  }
}



/**
 * Event handler for button clicks.
 * The panel contains buttons which are clicked.  These buttons have the same
 * integer ID as the tools envoked from the Tools menu.  This would usually
 * cause some serious errors if these button events were to propagate up to its
 * parent, the Organizer, since the buttons generate a 
 * wxEVT_COMMAND_BUTTON_CLICKED which is not entirely compatible with 
 * wxEVT_COMMAND_MENU_SELECTED. In that case, the event's type needs to change.
 */
void CalculationContextPanel::OnButtonClick(wxCommandEvent &event)
{
  event.SetEventType(wxEVT_COMMAND_MENU_SELECTED);
  event.Skip(); // continue processing up the event ladder (go to Organizer)
}

