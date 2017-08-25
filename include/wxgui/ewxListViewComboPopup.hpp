#include "wx/listctrl.h"
#include "wx/combo.h"

class ewxListViewComboPopup : public wxListView, public wxComboPopup
{
public:

  ewxListViewComboPopup(int id);
  virtual void Init();
  virtual bool Create( wxWindow* parent );
  virtual wxWindow *GetControl();
  virtual void SetStringValue( const wxString& s );
  virtual wxString GetStringValue() const;

  // Mouse hot-tracking
  void OnMouseMove(wxMouseEvent& event);

  // On mouse left, set the value and close the popup
  void OnMouseClick(wxMouseEvent& WXUNUSED(event));

  // Utilies for item manipulation
  void AddSelection( const wxString& selstr );

protected:

  int m_value; // current item index
  int m_itemHere; // hot item in popup

  int m_comboId;
private:
  DECLARE_EVENT_TABLE()
};
