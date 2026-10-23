// The Density Map's picture of a calculation folder, without the Builder or
// any ECCE service (#215): the same Evaluator and the same paint function the
// panel uses, into a PNG.  Needs a display (Xvfb) for fonts.
//
//   densitymap_shot CALC OUT.png [--mo N|homo|lumo] [--beta]
//       [--plane molecular | atoms A B C | axial A B] [--sum]
//       [--offset A] [--view RX RY RZ UX UY UZ] [--size W H]
#include <cstdlib>
#include <iostream>
#include <string>
#include <unistd.h>

#include <wx/app.h>
#include <wx/bitmap.h>
#include <wx/dcmemory.h>
#include <wx/image.h>

#include "util/EcceURL.H"
#include "tdat/DensityPlane.H"
#include "dsm/EDSIFactory.H"
#include "dsm/EDSIServerCentral.H"
#include "dsm/IPropCalculation.H"
#include "dsm/Resource.H"

#include "DensityMapDraw.H"

using namespace std;
using namespace DensityPlane;

int main(int argc, char **argv)
{
  if (argc < 3) {
    cerr << "usage: densitymap_shot CALC OUT.png [--mo N|homo|lumo] [--beta] "
            "[--plane molecular | atoms A B C | axial A B] [--sum] [--offset A] "
            "[--view RX RY RZ UX UY UZ] [--size W H]\n";
    return 2;
  }
  string target = argv[1], out = argv[2], mo;
  bool beta = false;
  PlaneKind kind = MOLECULAR;
  int atoms[3] = { -1, -1, -1 }, width = 900, height = 700;
  bool sum = false, view = false;
  double offset = 0.0, axes[6] = { 1, 0, 0, 0, 1, 0 };
  for (int i = 3; i < argc; i++) {
    string a = argv[i];
    if (a == "--mo" && i+1 < argc) mo = argv[++i];
    else if (a == "--beta") beta = true;
    else if (a == "--sum") sum = true;
    else if (a == "--offset" && i+1 < argc) offset = atof(argv[++i]);
    else if (a == "--view" && i+6 < argc) { view = true; for (int n = 0; n < 6; n++) axes[n] = atof(argv[++i]); }
    else if (a == "--size" && i+2 < argc) { width = atoi(argv[++i]); height = atoi(argv[++i]); }
    else if (a == "--plane" && i+1 < argc) {
      string k = argv[++i];
      int need = 0;
      if (k == "atoms") { kind = THREE_ATOMS; need = 3; }
      else if (k == "axial") { kind = AXIAL; need = 2; }
      for (int n = 0; n < need && i+1 < argc; n++) atoms[n] = atoi(argv[++i]) - 1;
    }
  }

  wxApp::SetInstance(new wxApp);
  if (!wxInitialize()) { cerr << "no display\n"; return 1; }
  wxInitAllImageHandlers();

  if (target[0] != '/') {
    char buf[4096];
    if (getcwd(buf, sizeof buf)) target = string(buf) + "/" + target;
  }
  EDSIServerCentral central;
  Resource *res = EDSIFactory::getResource(EcceURL("file://" + target));
  IPropCalculation *calc = dynamic_cast<IPropCalculation*>(res);
  if (!calc) { cerr << target << " is not a calculation\n"; return 1; }
  Evaluator ev;
  if (!ev.build(calc)) { cerr << ev.complaint << endl; return 1; }
  int orbital = 0;
  if (!mo.empty())
    orbital = mo == "homo" ? ev.homo() : mo == "lumo" ? ev.homo() + 1 : atoi(mo.c_str());

  Plane plane;
  string why;
  Map map;
  bool have = (view ? makeViewPlane(ev.coordsAng, axes, axes + 3, plane, why)
                    : makePlane(kind, ev.coordsAng, ev.atomLabels(), atoms[0],
                                atoms[1], atoms[2], plane, why));
  if (have) {
    plane = offsetPlane(plane, offset);
    have = ev.map(plane, sum && orbital ? ORBITAL_SET : orbital ? ORBITAL : DENSITY,
                  orbital, beta, ev.symbols.size() > 20 ? 0.1 : 0.05, 1.5, map);
  }
  wxBitmap bmp(width, height);
  wxMemoryDC dc(bmp);
  paintDensityMap(dc, wxSize(width, height), have, map, wxString::FromUTF8(why.c_str()));
  dc.SelectObject(wxNullBitmap);
  const bool saved = bmp.SaveFile(wxString::FromUTF8(out.c_str()), wxBITMAP_TYPE_PNG);
  if (have)
    cerr << map.title << ", " << plane.description << ", " << map.nu << "x" << map.nv
         << ", " << map.zmin << " .. " << map.zmax << endl;
  wxUninitialize();
  return saved ? 0 : 1;
}
