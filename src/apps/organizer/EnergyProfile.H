/**
 * @file
 *
 * Energy profile of one reaction with a transition state (Reaction overview):
 * reactant, TS and product as levels for dG and dE, the dG barrier as a
 * double arrow with k beside it.  Every label is placed by a collision check
 * against the drawn segments and the other labels, so the layout is computed
 * first (EnergyProfileLayout) and painting only draws it; tests read the same
 * layout through dump().
 */

#ifndef ENERGYPROFILE_HH
#define ENERGYPROFILE_HH

#include <string>
#include <vector>

#include <wx/colour.h>
#include <wx/gdicmn.h>
#include <wx/panel.h>

#include "wxgui/PlotStyle.H"

class wxDC;

struct EnergyProfileData
{
  std::string title;                 ///< the reaction, shortened names
  std::string names[3];              ///< reactant, TS, product
  double E[3] = {0, 0, 0};           ///< electronic, kJ/mol from the reactants
  double G[3] = {0, 0, 0};
  double kelvin = 298.15;
  std::vector<std::string> rateLines; ///< "k (TST) 7.97 × 10⁷ s⁻¹", ...
  /** The numbers as plain text (Copy values, the dialog's text). */
  std::string text() const;
};

struct EnergyProfileLayout
{
  struct Seg
  {
    double x1, y1, x2, y2;
    wxColour colour;
    int width = 1;
    bool dotted = false;
    std::string what;
  };
  struct Text
  {
    wxRect r;
    std::string text;
    wxColour colour;
    bool vertical = false;           ///< drawn rotated by 90 degrees
    std::string what;
  };
  std::vector<Seg> segs;
  std::vector<Text> texts;
  wxSize size;
  bool placed = true;                ///< false when a label found no free place
  /** "size W H", "text x y w h what", "seg x1 y1 x2 y2 width what", one per line. */
  std::string dump() const;
};

EnergyProfileLayout layoutEnergyProfile(wxDC& dc, const EnergyProfileData& d,
                                        const wxSize& size, const PlotPalette& pal);
void paintEnergyProfile(wxDC& dc, const EnergyProfileLayout& l, const PlotPalette& pal);

/** The profile on screen; right-click copies its numbers. */
class EnergyProfileCanvas : public wxPanel
{
public:
  explicit EnergyProfileCanvas(wxWindow *parent);
  void setProfile(const EnergyProfileData& d);
  const EnergyProfileData& profile() const { return p_data; }

private:
  void onPaint(wxPaintEvent&);
  EnergyProfileData p_data;
};

/** Size of an exported profile; the layout hooks use the same. */
const wxSize ENERGY_PROFILE_PNG(900, 480);
/** The layout of d at the export size, on white (EnergyProfileLayout::dump()). */
EnergyProfileLayout exportLayout(const EnergyProfileData& d);
/** All profiles one under the other, on white. */
bool saveEnergyProfilesPng(const std::vector<EnergyProfileData>& profiles, const std::string& file);

/** 1.23 with a real minus sign. */
std::string profileNum(double v, const char *fmt = "%.1f");

#endif
