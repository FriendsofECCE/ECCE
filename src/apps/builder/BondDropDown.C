#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "BondDropDown.H"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "util/EventDispatcher.H"
#include "util/Event.H"

#include "tdat/TBond.H"

#include "wxgui/ewxBitmap.H"
#include "wxgui/ewxWindowUtils.H"
#include "wxgui/WindowEvent.H"
  using namespace ecce;

#include "BondDropDown.H"


IMPLEMENT_DYNAMIC_CLASS( BondDropDown, wxToolBar )

BEGIN_EVENT_TABLE( BondDropDown, wxToolBar )
    EVT_TOOL( wxID_ANY, BondDropDown::OnToolClick )
END_EVENT_TABLE()


BondDropDown::BondDropDown( )
  : wxToolBar(),
    p_lastId(SINGLE)
{
}



BondDropDown::BondDropDown(double order, wxWindow* parent, wxWindowID id,
        const wxPoint& pos, const wxSize& size, long style,
        const wxString& name)
  : wxToolBar(),
    p_lastId(SINGLE)
{
  Create(order, parent, id, pos, size, style);
}



bool BondDropDown::Create(double order, wxWindow* parent, wxWindowID id,
        const wxPoint& pos, const wxSize& size, long style,
        const wxString& name)
{
   if (!wxToolBar::Create(parent, id, pos, size, style, name)) {
      wxFAIL_MSG( wxT("BondDropDown creation failed") );
      return false;
   }

   AddTool(SINGLE, TBond::orderToString(TBond::Single),
           ewxBitmap(TBond::orderToImage(TBond::Single)),
           TBond::orderToString(TBond::Single), wxITEM_RADIO);
   AddTool(DOUBLE, TBond::orderToString(TBond::Double),
           ewxBitmap(TBond::orderToImage(TBond::Double)),
           TBond::orderToString(TBond::Double), wxITEM_RADIO);
   AddTool(TRIPLE, TBond::orderToString(TBond::Triple),
           ewxBitmap(TBond::orderToImage(TBond::Triple)),
           TBond::orderToString(TBond::Triple), wxITEM_RADIO);
   AddTool(AROMATIC, TBond::orderToString(TBond::Aromatic),
           ewxBitmap(TBond::orderToImage(TBond::Aromatic)),
           TBond::orderToString(TBond::Aromatic), wxITEM_RADIO);
   AddTool(METAL, TBond::orderToString(TBond::Metal),
           ewxBitmap(TBond::orderToImage(TBond::Metal)),
           TBond::orderToString(TBond::Metal), wxITEM_RADIO);

   Realize();

   // now select the right one
   select(order);

   return true;
}



BondDropDown::~BondDropDown()
{
}


void BondDropDown::select(double order)
{
   int id = SINGLE;
   if (order == TBond::Single) {
      id = SINGLE;
   } else if (order == TBond::Double) {
      id = DOUBLE;
   } else if (order == TBond::Triple) {
      id = TRIPLE;
   } else if (order == TBond::Aromatic) {
      id = AROMATIC;
   } else if (order == TBond::Metal) {
      id = METAL;
   }
   ToggleTool(id,true);
   p_lastId = id;
}


double BondDropDown::idToBondOrder(int id)
{
   double value = TBond::Single;
   if (id == BondDropDown::SINGLE) {
      value = TBond::Single;
   } else if (id == BondDropDown::DOUBLE) {
      value = TBond::Double;
   } else if (id == BondDropDown::TRIPLE) {
      value = TBond::Triple;
   } else if (id == BondDropDown::AROMATIC) {
      value = TBond::Aromatic;
   } else if (id == BondDropDown::METAL) {
      value = TBond::Metal;
   }
   return value;
}

void BondDropDown::OnToolClick( wxCommandEvent& event )
{
   double value = idToBondOrder(event.GetId());
   p_lastId = event.GetId();
   string strorder = TBond::orderToString(value);

   // Somebody else will do the real processing
   EventDispatcher::getDispatcher().publish(Event("BondChanged",strorder));

   event.Skip();
}

