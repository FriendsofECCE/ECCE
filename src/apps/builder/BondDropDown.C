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

   SetToolBitmapSize(wxSize(24, 24));
   //  A bond is covalent or metal-pi; orders are not a user choice (#439).
   AddTool(SINGLE, "Covalent",
           ewxBitmap::colorIcon(TBond::orderToImage(TBond::Single), wxSize(24, 24)),
           "Covalent bond", wxITEM_RADIO);
   AddTool(METAL, "Metal-pi",
           ewxBitmap::colorIcon(TBond::orderToImage(TBond::Metal), wxSize(24, 24)),
           "Metal-pi bond (a pi ligand over a metal)", wxITEM_RADIO);

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
   int id = order == TBond::Metal ? METAL : SINGLE;
   ToggleTool(id,true);
   p_lastId = id;
}


double BondDropDown::idToBondOrder(int id)
{
   return id == BondDropDown::METAL ? TBond::Metal : TBond::Single;
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

