#include "util/Event.H"
#include "util/EventDispatcher.H"

#include "wxgui/ewxBitmap.H"
using namespace ecce;

#include "ShapeDropDown.H"


IMPLEMENT_DYNAMIC_CLASS( ShapeDropDown, wxToolBar )

BEGIN_EVENT_TABLE( ShapeDropDown, wxToolBar )

  EVT_TOOL( wxID_ANY, ShapeDropDown::OnToolClick )

END_EVENT_TABLE()


ShapeDropDown::ShapeDropDown( )
  : wxToolBar(),
    p_lastId(ID_SHAPE_BASE)
{
}



ShapeDropDown::ShapeDropDown(ShapeData::Shape shape, wxWindow* parent,
        wxWindowID id, const wxPoint& pos, const wxSize& size,
        long style, const wxString& name)
  : wxToolBar(),
    p_lastId(ID_SHAPE_BASE)
{
  Create(shape, parent, id, pos, size, style, name);
}



bool ShapeDropDown::Create(ShapeData::Shape shape, wxWindow* parent,
        wxWindowID id, const wxPoint& pos, const wxSize& size,
        long style, const wxString& name)
{
  if (!wxToolBar::Create(parent, id, pos, size, style, name)) {
    wxFAIL_MSG( wxT("ShapeDropDown creation failed") );
    return false;
  }

  addShape(ShapeData::lone);
  addShape(ShapeData::terminal);
  addShape(ShapeData::linear);
  addShape(ShapeData::bent);
  addShape(ShapeData::trigonalPlanar);
  addShape(ShapeData::pyramidal);
  addShape(ShapeData::tetrahedral);
  addShape(ShapeData::squarePlanar);
  addShape(ShapeData::trigonalBipyramidal);
  addShape(ShapeData::squarePyramidal);
  addShape(ShapeData::octahedral);
  addShape(ShapeData::cubic);
  addShape(ShapeData::decahedral);

  Realize();

  select(shape);

  return true;
}


ShapeDropDown::~ShapeDropDown()
{
}


void ShapeDropDown::select(ShapeData::Shape shape)
{
  // enums are parallel so just need to add the offset
  int id = shape + ID_SHAPE_BASE;
  ToggleTool(id,true);
  p_lastId = id;
}


void ShapeDropDown::OnToolClick( wxCommandEvent& event )
{
  p_lastId = event.GetId();

  // The two enumerations are ordered the same but ours has an offset
  // so we just need to do the math the get the ShapeData id.
  ShapeData::Shape shapeid = (ShapeData::Shape)(p_lastId - ID_SHAPE_BASE);
  string shape = ShapeData::shapeToString(shapeid);

  // Somebody else will do the real processing
  EventDispatcher::getDispatcher().publish(Event("ShapeChanged",shape));

  event.Skip();
}


void ShapeDropDown::addShape(ShapeData::Shape shape)
{
  AddTool(shape + ID_SHAPE_BASE, ShapeData::shapeToString(shape),
          ewxBitmap(ShapeData::shapeToImage(shape)), "", wxITEM_RADIO);
}

