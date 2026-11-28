#include "tdat/DockFragmentIO.H"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <fstream>

#include "dock/DockPrep.H"
#include "tdat/Fragment.H"
#include "tdat/TAtm.H"
#include "tdat/TBond.H"

static std::string s_error;

static std::string upper(const std::string& s)
{
  std::string r = s;
  for (size_t i = 0; i < r.size(); i++) r[i] = (char)toupper((unsigned char)r[i]);
  return r;
}

bool DockFragmentIO::isNative(const std::string& type)
{
  std::string t = upper(type);
  return t == "SDF" || t == "MOL" || t == "SD" || t == "MOL2" || t == "PDBQT";
}

bool DockFragmentIO::canRead(const std::string& type)
{
  return isNative(type) || DockObabel::canRead(type);
}

const std::string& DockFragmentIO::lastError()
{
  return s_error;
}

bool DockFragmentIO::read(Fragment& frag, const std::string& path,
                          const std::string& type)
{
  s_error.clear();
  DockMol mol;
  if (!DockReader::readStructure(path, type, mol, s_error)) return false;

  int first = (int)frag.numAtoms();
  for (int i = 0; i < mol.numAtoms(); i++) {
    const DockAtom& a = mol.atoms[i];
    double xyz[3] = { a.x, a.y, a.zc };
    frag.addAtom(DockMol::symbolFromElement(a.z), xyz);
    frag.atomRef(first + i)->partialCharge(a.charge);
  }
  for (size_t b = 0; b < mol.bonds.size(); b++) {
    int o = mol.bonds[b].order;
    double order = (o == 4) ? TBond::Aromatic : (double)o;
    frag.addBond(first + mol.bonds[b].i, first + mol.bonds[b].j, order);
  }
  // PDBQT bonds come from distances alone; give them ECCE's usual
  // distance-based orders, as for any structure read without them.
  if (upper(type) == "PDBQT") frag.generateBondOrders();
  frag.setEnvIndices();
  return true;
}

bool DockFragmentIO::read(Fragment& frag, std::istream& in,
                          const std::string& type)
{
  // The readers want a path, and obabel picks its reader from the type we
  // pass, so any temporary name will do.
  const char* d = getenv("TMPDIR");
  if (!d) d = getenv("TEMP");
  std::string tmp = std::string(d ? d : "/tmp") + "/ecce-structure-" +
                    std::to_string((long)rand()) + "." + type;
  {
    std::ofstream os(tmp.c_str(), std::ios::binary);
    if (!os) { s_error = "cannot write " + tmp; return false; }
    os << in.rdbuf();
  }
  bool ok = read(frag, tmp, type);
  std::remove(tmp.c_str());
  return ok;
}
