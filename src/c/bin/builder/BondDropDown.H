#ifndef BONDDROPDOWN_H
#define BONDDROPDOWN_H

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "BondDropDown.C"
#endif

#include <wx/toolbar.h>

#include "wxviz/WxVizTool.H"


class BondDropDown : public wxToolBar,
                     public WxVizTool
{    
   DECLARE_DYNAMIC_CLASS( BondDropDown )
   DECLARE_EVENT_TABLE()

   enum BondOrder { SINGLE=9220, DOUBLE, TRIPLE, AROMATIC, METAL };

   public:
      BondDropDown();
      BondDropDown(double order, wxWindow* parent, wxWindowID id = wxID_ANY,
              const wxPoint& pos = wxDefaultPosition,
              const wxSize& size = wxDefaultSize,
              long style = wxTB_VERTICAL|wxNO_BORDER,
              const wxString& name = "Bond Order");
      virtual ~BondDropDown();

      bool Create(double order, wxWindow* parent, wxWindowID id = wxID_ANY,
              const wxPoint& pos = wxDefaultPosition,
              const wxSize& size = wxDefaultSize,
              long style = wxTB_VERTICAL|wxNO_BORDER,
              const wxString& name = "Bond Order");

      void select(double order);
      void select(int id);

   protected:
      virtual void OnToolClick( wxCommandEvent& event );
      double idToBondOrder(int id);

      int p_lastId;
};

#endif // BONDDROPDOWN_H
