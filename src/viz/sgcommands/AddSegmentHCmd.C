/**
 * @file
 *
 *
 */
#include <iostream>
  using namespace std;

#include "util/EventDispatcher.H"
#include "util/Event.H"
using namespace ecce;
#include "util/FFConfigData.H"
#include "util/StringConverter.H"

#include "dsm/SegFactory.H"

#include "tdat/Segment.H"
#include "tdat/FragUtil.H"
#include "tdat/TPerTab.H"
#include "tdat/ShapeData.H"
#include "tdat/TAtm.H"
#include "tdat/TBond.H"
#include "tdat/Residue.H"
#include <cmath>
#include <set>
#include <array>

#include "viz/AddSegmentHCmd.H"
#include "viz/SGFragment.H"
#include "viz/SGContainer.H"

#define PI 3.14159265358979323846

namespace {

typedef double V3[3];

double bondOrderFromLength(const string& a, const string& b, double d)
{
  string x = a < b ? a : b, y = a < b ? b : a;
  if (x == "C" && y == "C") return d < 1.25 ? 3 : d < 1.32 ? 2 : d < 1.46 ? 1.5 : 1;
  if (x == "C" && y == "N") return d < 1.20 ? 3 : d < 1.30 ? 2 : d < 1.40 ? 1.5 : 1;
  if (x == "C" && y == "O") return d < 1.30 ? 2 : 1;
  if (x == "C" && y == "S") return d < 1.65 ? 2 : 1;
  return 1;
}

void unit(V3 v)
{
  double n = sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
  if (n > 1e-9) for (int i = 0; i < 3; i++) v[i] /= n;
}

void cross(const V3 a, const V3 b, V3 c)
{
  c[0] = a[1]*b[2] - a[2]*b[1];
  c[1] = a[2]*b[0] - a[0]*b[2];
  c[2] = a[0]*b[1] - a[1]*b[0];
}

// A unit vector perpendicular to v, in the plane of v and ref when ref is
// not parallel to v.
void perpendicular(const V3 v, const V3 ref, V3 out)
{
  V3 n;
  cross(v, ref, n);
  double len = sqrt(n[0]*n[0] + n[1]*n[1] + n[2]*n[2]);
  if (len < 1e-3) {
    V3 axis = {1, 0, 0};
    if (fabs(v[0]) > 0.9) { axis[0] = 0; axis[1] = 1; }
    cross(v, axis, n);
  }
  unit(n);
  cross(n, v, out);
  unit(out);
}

// Hydrogens for atoms of residues the templates do not cover, from the
// heavy-atom geometry: the bond order of each bond is read off its length
// (a ring C-C of 1.36-1.46 A is aromatic), what the valence leaves open is
// filled by hydrogens in the ideal directions.  Returns the number added.
int addValenceHydrogens(SGFragment *frag, const set<const Residue*>& residues)
{
  int added = 0;
  vector<TAtm*> *all = frag->atoms();
  vector<TAtm*> todo;
  for (size_t i = 0; i < all->size(); i++)
    if (residues.count((*all)[i]->getResidue())) todo.push_back((*all)[i]);
  delete all;

  for (size_t t = 0; t < todo.size(); t++) {
    TAtm *atm = todo[t];
    const string sym = atm->atomicSymbol();
    int valence = sym == "C" ? 4 : sym == "N" ? 3 : (sym == "O" || sym == "S") ? 2 : 0;
    if (valence == 0) continue;
    const double *p = atm->coordinates();
    vector<V3*> dirs;
    double sum = 0;
    const vector<TBond*>& bl = atm->bondList();
    for (size_t b = 0; b < bl.size(); b++) {
      TAtm *o = bl[b]->atom1() == atm ? bl[b]->atom2() : bl[b]->atom1();
      if (o->atomicSymbol() == "Nub") continue;
      V3 *d = new V3[1];
      for (int k = 0; k < 3; k++) (*d)[k] = o->coordinates()[k] - p[k];
      double len = sqrt((*d)[0]*(*d)[0] + (*d)[1]*(*d)[1] + (*d)[2]*(*d)[2]);
      sum += o->atomicSymbol() == "H" ? 1 : bondOrderFromLength(sym, o->atomicSymbol(), len);
      unit(*d);
      dirs.push_back(d);
    }
    int k = dirs.size();
    int m = (int)floor(valence - sum + 0.5);
    vector<std::array<double,3>> out;
    if (k >= 1 && m >= 1 && k + m <= 4) {
      const double tet = 1.0/3.0, ts = 0.9428090416;
      if (k == 1) {
        V3 b = {(*dirs[0])[0], (*dirs[0])[1], (*dirs[0])[2]};
        V3 ref = {0, 0, 0};
        // The other neighbour of the neighbour fixes the plane, if any.
        TAtm *nb = 0;
        for (size_t q = 0; q < bl.size() && !nb; q++) {
          TAtm *o = bl[q]->atom1() == atm ? bl[q]->atom2() : bl[q]->atom1();
          if (o->atomicSymbol() != "Nub") nb = o;
        }
        const vector<TBond*>& nbl = nb->bondList();
        for (size_t q = 0; q < nbl.size(); q++) {
          TAtm *o = nbl[q]->atom1() == nb ? nbl[q]->atom2() : nbl[q]->atom1();
          if (o == atm || o->atomicSymbol() == "Nub") continue;
          for (int c = 0; c < 3; c++) ref[c] = o->coordinates()[c] - p[c];
          break;
        }
        V3 e1, e2;
        perpendicular(b, ref, e1);
        cross(b, e1, e2);
        if (m == 1 && sym == "C") {
          out.push_back({-b[0], -b[1], -b[2]});
        } else if (m == 2) {
          for (int sgn = -1; sgn <= 1; sgn += 2) {
            V3 d;
            for (int c = 0; c < 3; c++) d[c] = -0.5*b[c] + sgn*0.8660254*e1[c];
            out.push_back({d[0], d[1], d[2]});
          }
        } else {
          for (int h = 0; h < m && h < 3; h++) {
            double phi = 2.0943951*h;
            V3 d;
            for (int c = 0; c < 3; c++)
              d[c] = -tet*b[c] + ts*(cos(phi)*e1[c] + sin(phi)*e2[c]);
            out.push_back({d[0], d[1], d[2]});
          }
        }
      } else if (k == 2) {
        V3 s, n;
        for (int c = 0; c < 3; c++) s[c] = -((*dirs[0])[c] + (*dirs[1])[c]);
        unit(s);
        cross(*dirs[0], *dirs[1], n);
        unit(n);
        if (m == 1) {
          out.push_back({s[0], s[1], s[2]});
        } else {
          for (int sgn = -1; sgn <= 1; sgn += 2) {
            V3 d;
            for (int c = 0; c < 3; c++) d[c] = 0.5774*s[c] + sgn*0.8165*n[c];
            out.push_back({d[0], d[1], d[2]});
          }
        }
      } else if (k == 3) {
        V3 s;
        for (int c = 0; c < 3; c++)
          s[c] = -((*dirs[0])[c] + (*dirs[1])[c] + (*dirs[2])[c]);
        unit(s);
        out.push_back({s[0], s[1], s[2]});
      }
    }
    for (size_t b = 0; b < dirs.size(); b++) delete[] dirs[b];

    const double len = sym == "C" ? 1.09 : sym == "N" ? 1.01 : sym == "O" ? 0.96 : 1.34;
    for (size_t h = 0; h < out.size(); h++) {
      double xyz[3] = {p[0] + len*out[h][0], p[1] + len*out[h][1], p[2] + len*out[h][2]};
      TAtm *hatm = new TAtm("H", xyz);
      frag->addAtom(hatm, atm);
      hatm->atomName(hatm->getHydrogenName().c_str());
      TBond *bond = new TBond(atm, hatm, TBond::Single);
      bond->displayStyle(frag->getMainDisplayStyle());
      added++;
    }
  }
  return added;
}

}

AddSegmentHCmd::AddSegmentHCmd(const string& name, Receiver * receiver)
    : FragCloneCmd(name, receiver)
{
   init();
}

/**
 * Destructor.
 * Deletes undo related data.
 */
AddSegmentHCmd::~AddSegmentHCmd()
{
}

void AddSegmentHCmd::init()
{
   string dir = StringConverter::toString(FFConfigData::getSegDirs());
   addParameter(new CommandParameter("directories", dir));
   addParameter(new CommandParameter("fftype", "Amber"));
   // Residues with no template (ligands) get hydrogens from their geometry.
   addParameter(new CommandParameter("valence", false));
   setHelpMessage("Add hydrogens to structure based on available fragment or "
                  "segment files");
}

bool AddSegmentHCmd::execute()
{
  bool ret = false;

  SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
  SGFragment *frag = sg->getFragment();
  // Heavy atoms from an XYZ or CIF file have no residues: every bonded
  // group is filled from its geometry (lone atoms have no direction to
  // fill and are left alone).
  if (frag->numAtoms() > 0 && frag->numResidues() == 0 &&
      getParameter("valence")->getBoolean()) {
    cloneFragment(frag);
    set<const Residue*> none;
    none.insert(static_cast<const Residue*>(0));
    addValenceHydrogens(frag, none);
    frag->touchNumbers();
    sg->touchChemDisplay();
    Event event("GeomChange");
    EventDispatcher::getDispatcher().publish(event);
    return true;
  }
  if (frag->numAtoms() > 0 && frag->numResidues() > 0) {
    ret = true;

    cloneFragment(frag);

    string ename = "GeomChange";
    if (frag->numResidues() > 0) {
      ename = "ResidueChange";
    }

    FragUtil fragutil((Fragment*)frag);
    vector<TAtm*> newNubs;
    TAtm *nub;
    vector<int> rotationList, rotationList2;
    vector<int> adjustList, adjustList2;
    vector<int> nubParent;
    int idx;
    TPerTab tpt;

    // OK, You've got the scene graph fragment, now add hydrogens.
    // Start by compiling a list of directories containing segment
    // and fragment files.
    string segdirs = getParameter("directories")->getString();
    string fftype = getParameter("fftype")->getString();

    SegFactory segfactory;
    segfactory.clearEDSI();

    char* sdirect = strdup(segdirs.c_str());
    char* segptr = strtok(sdirect,":\n");

    for (;segptr != NULL; segptr = strtok(NULL, ":\n")) {
      segfactory.addEDSI(segptr);
    }
    delete [] sdirect;

    // Scan through all residues
    int nresidues = frag->numResidues();

    int saveResidue = frag->editResidueIndex();
    frag->editResidueMode(0,saveResidue);
    int i;
    for (i=0; i<nresidues; i++) {

      // First check residue against available segments to see if it is
      // generatable. If not, then no attempt will be made to add hydrogens
      // to this residue.

      Residue* residue = frag->residue(i);
      frag->editResidueMode(1,i);
      Residue::Coupling coupling = residue->couplingInfo();
      Segment* segment = segfactory.getSegment(residue,coupling,frag);
      Residue::ResidueStatus status;
      if (segment != (Segment*)0) {
        status = segment->compare(residue,frag);
      } else {
        status = Residue::UNKNOWN;
      }
      residue->residueStatus(status);

      // If residue is generatable, then add hydrogens to it.

      if (status == Residue::GENERATABLE) {
        vector <TAtm*> ListResidueAtoms = frag->findResidueAtoms(residue);
        int nratom = ListResidueAtoms.size();
        int nsatom = segment->natom();
        int nsbond = segment->nbond();
        vector<string> snames = segment->atomName();
        vector<int> sibnd = segment->iIndex();
        vector<int> sjbnd = segment->jIndex();
        vector<int> setyp = segment->envType();
        for (int j=0; j<nratom; j++) {
          TAtm* ratm = ListResidueAtoms[j];
          string rname = ratm->atomName();

          // Scan through atoms in segment and find atom corresponding to residue
          // atom.

          vector<int> sngbr;
          vector<bool> sused;
          int idihedral;
          for (int k=0; k<nsatom; k++) {
            if (rname == snames[k]) {

              // Found corresponding atom in segment. Record whether atom is part of
              // an improper dihedral and find atoms that are supposed to be bonded
              // to it.

              idihedral = setyp[k];
              for (int l=0; l<nsbond; l++) {
                if (sibnd[l] == k) {
                  sngbr.push_back(sjbnd[l]);
                  sused.push_back(false);
                } else if (sjbnd[l] == k) {
                  sngbr.push_back(sibnd[l]);
                  sused.push_back(false);
                }
              }
              break;
            }
          }

          // Found indices of all atoms connect to segment atom. Now check
          // residue atom to find out if it has all its neighbors. If atom
          // has a nonzero connection index, then assume it has a bond to
          // one atom outside the residue.

          int snum = sngbr.size();
          if (ratm->connectionIndex() != 0) {
            snum++;
          }
          if (snum > 0) {
            vector<TAtm*>* rngbr = ratm->connectedAtoms();
            int rnum = rngbr->size();
            delete rngbr;
            if (rnum < snum) {
              string ratsym = ratm->atomicSymbol();
              string tratnam;

              // We now have a list of missing atoms and their names. Add missing
              // atoms. Start by determining shape of residue atom.

              string shape;
              if (snum == 1) {
                shape = ShapeData::shapeToString(ShapeData::terminal);
              } else if (snum ==2) {
                if (ratsym == "C") {
                  shape = ShapeData::shapeToString(ShapeData::linear);
                } else {
                  shape = ShapeData::shapeToString(ShapeData::bent);
                }
              } else if (snum ==3) {
                if (ratsym == "C" || idihedral == 1 ) {
                  shape = ShapeData::shapeToString(ShapeData::trigonalPlanar);
                } else {
                  shape = ShapeData::shapeToString(ShapeData::pyramidal);
                }
              } else if (snum ==4) {
                shape = ShapeData::shapeToString(ShapeData::tetrahedral);
              } else if (snum ==5) {
                shape = ShapeData::shapeToString(ShapeData::trigonalBipyramidal);
              } else if (snum ==6) {
                shape = ShapeData::shapeToString(ShapeData::octahedral);
              } else if (snum ==8) {
                shape = ShapeData::shapeToString(ShapeData::cubic);
              } else if (snum ==10) {
                shape = ShapeData::shapeToString(ShapeData::decahedral);
              } else {
                shape = ShapeData::shapeToString(ShapeData::lone);
              }
              if (shape != ShapeData::shapeToString(ShapeData::lone)) {
                adjustList.push_back(ratm->index());
                for (idx=0; idx<snum-rnum; idx++) {
                  nub = new TAtm(tpt.nubAtom());
                  nubParent.push_back(ratm->index());
                  newNubs.push_back(nub);
                  ratm->shapeString(shape);
                  nub->shape(ShapeData::terminal);
                  nub->atomResidue(ratm->getResidue());
                  nub->displayStyle(ratm->displayStyle());
                }
              }
              // If original residue atom was not bonded to anything then mark it. After
              // nubs are added, apply an arbitrary rotation to it so that hydrogens are
              // not all oriented in the same direction. This is especially useful when
              // adding hydrogens to water molecules.

              if (rnum == 0 && snum > 0)
                rotationList.push_back(ratm->index());
            }
          }
        }
      }
      frag->editResidueMode(0,i);
    }
    // All nubs have now been created. The list of original atoms and nubs must now
    // be interleaved into a single list
    vector<TAtm*> newAtomList;
    vector<int> iindex, jindex;
    int iat=0, inub=0, jat, irot=0, iadj=0;
    int rsize = rotationList.size();
    int asize = adjustList.size();
    int nsize = nubParent.size();

    int natoms = frag->numAtoms();
    for (i=0; i<natoms; i++) {
      newAtomList.push_back(frag->atomRef(i));
      jat = iat;
      while (irot < rsize && rotationList[irot] < i)
        irot++;
      if (irot < rsize && rotationList[irot] == i) {
        rotationList2.push_back(iat);
        irot++;
      }
      while (iadj < asize && adjustList[iadj] < i)
        iadj++;
      if (iadj < asize && adjustList[iadj] == i) {
        adjustList2.push_back(iat);
        iadj++;
      }
      iat++;
      while (inub < nsize && nubParent[inub]<i)
        inub++;
      while (inub < nsize && nubParent[inub] == i) {
        newAtomList.push_back(newNubs[inub]);
        iindex.push_back(iat);
        jindex.push_back(jat);
        iat++;
        inub++;
      }
    }

    // Reset the fragment atom list
    frag->resetAtomList(newAtomList);

    // add bonds to fragment
    int bsize = iindex.size();
    for (i=0; i<bsize; i++)
      frag->addBond(jindex[i], iindex[i], 1.0);

    // adjust geometries of newly added nubs
    asize = adjustList2.size();
    for (i=0; i<asize; i++)
      fragutil.adjustExistingNubs(frag->atomRef(adjustList2[i]));

    // Apply arbitrary rotation to molecules with only a single heavy atom
    rsize = rotationList2.size();
    TAtm *ratm;
    for (i=0; i<rsize; i++) {
      ratm = frag->atomRef(rotationList2[i]);

      // Generate three random Euler angles and use these to construct rotation
      // matrix.
      int irandnum = (rand() % 10000) + 1;
      double phi = ((double)irandnum)*0.0001*PI;
      irandnum = (rand() % 10000) + 1;
      double psi = ((double)irandnum)*0.0001*PI;
      irandnum = (rand() % 10000) + 1;
      double theta = ((double)irandnum)*0.00005*PI;

      double cphi = cos(phi);
      double sphi = sin(phi);
      double cpsi = cos(psi);
      double spsi = sin(psi);
      double ctheta = cos(theta);
      double stheta = sin(theta);

      double rxx = cpsi*cphi-ctheta*sphi*spsi;
      double rxy = cpsi*sphi+ctheta*cphi*spsi;
      double rxz = spsi*stheta;
      double ryx = -spsi*cphi-ctheta*sphi*cpsi;
      double ryy = -spsi*sphi+ctheta*cphi*cpsi;
      double ryz = cpsi*stheta;
      double rzx = stheta*sphi;
      double rzy = -stheta*cphi;
      double rzz = ctheta;

      const double *r0 = ratm->coordinates();
      vector<TAtm*>* catoms = ratm->connectedAtoms();
      int snum = catoms->size();
      double x,y,z;
      for (int m=0; m < snum; m++) {
        const double * r1 = (*catoms)[m]->coordinates();
        x = rxx*(r1[0]-r0[0])+rxy*(r1[1]-r0[1])+rxz*(r1[2]-r0[2]);
        y = ryx*(r1[0]-r0[0])+ryy*(r1[1]-r0[1])+ryz*(r1[2]-r0[2]);
        z = rzx*(r1[0]-r0[0])+rzy*(r1[1]-r0[1])+rzz*(r1[2]-r0[2]);
        x += r0[0];
        y += r0[1];
        z += r0[2];
        (*catoms)[m]->coordinates(x,y,z);
      }
      delete catoms;
    }

    frag->editResidueMode(1,saveResidue);

    sg->addHydrogens(frag, true, false);

    if (getParameter("valence")->getBoolean()) {
      set<const Residue*> rest;
      vector<SGFragment::AtomRole> roles = frag->atomRoles();
      for (int r = 0; r < frag->numResidues(); r++) {
        const Residue *res = frag->residue(r);
        if (res->residueStatus() == Residue::GENERATABLE) continue;
        TAtm *first = res->firstAtomPtr();
        if (first && roles[first->index()] == SGFragment::SOLUTE)
          rest.insert(res);
      }
      addValenceHydrogens(frag, rest);
    }

    frag->touchNumbers();
    sg->touchChemDisplay();

    Event event(ename);
    EventDispatcher::getDispatcher().publish(event);
  }
  return ret;
}
