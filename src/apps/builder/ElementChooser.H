// The Builder's element chooser (#222): a drawn periodic table with real
// proportions, a search box and rows of recent and common elements.  The
// Basis Set Tool and the standalone table keep PerTabPanel.
#ifndef ELEMENTCHOOSER_H
#define ELEMENTCHOOSER_H

#include <string>
#include <vector>

#include <wx/bitmap.h>
#include <wx/dc.h>
#include <wx/panel.h>
#include <wx/textctrl.h>

class ElementChooser : public wxPanel
{
public:
  ElementChooser(wxWindow *parent, const std::string& current);

  // The chooser is opt-in: ECCE_NEW_PERTABLE=1 or /ElementChooser/Enabled=1
  // in wxbuilder.ini; otherwise the Builder keeps the PerTabPanel popup.
  static bool enabled();

  // Hooks for scripts and tests; each acts as the matching click or key.
  bool pick(const std::string& symbol);
  int search(const std::string& text, bool pressEnter);
  std::vector<std::string> recent() const { return p_recent; }
  std::string currentElement() const { return p_current; }

  // kind: 0 table, 1 recent, 2 common; z < 0 is a "57-71"/"89-103" placeholder.
  struct Tile { int z; int kind; wxRect rect; };

  // The table as painted (not the search box), for headless screenshots.
  wxBitmap render(const wxSize& size);

  std::vector<Tile> layout(const wxSize& size, int *tileSize = 0) const;

protected:
  void OnPaint(wxPaintEvent&);
  void paintTo(wxDC& dc);
  void OnSize(wxSizeEvent&);
  void OnMotion(wxMouseEvent&);
  void OnLeave(wxMouseEvent&);
  void OnLeftUp(wxMouseEvent&);
  void OnText(wxCommandEvent&);
  void OnEnter(wxCommandEvent&);
  wxSize DoGetBestSize() const override;
  int searchHeight() const;
  const Tile *tileAt(const wxPoint& p, const std::vector<Tile>& tiles) const;
  void loadRecent();
  void saveRecent() const;

  wxTextCtrl *p_search;
  std::string p_current;
  std::vector<std::string> p_recent;
  std::vector<int> p_matches;
  int p_hover;
  bool p_searching;
};

#endif
