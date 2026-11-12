/**
 * @file
 *
 *
 */
#include <iostream>
  using namespace std;

#include <set>
  using std::set;
#include <algorithm>
  using std::find;
#include <cmath>
#include <chrono>
#include "tdat/LatticeDef.H"

#include "util/EventDispatcher.H"
#include "util/Event.H"
using namespace ecce;

#include "tdat/LinkCellList.H"
#include "tdat/TPerTab.H"
#include "tdat/TBond.H"
#include "tdat/TPerTab.H"

#include "viz/HydrogenBondsCmd.H"
#include "viz/SGFragment.H"
#include "viz/SGContainer.H"
#include "viz/AtomLine.H"
#include "viz/AtomMeasureDist.H"

HydrogenBondsCmd::HydrogenBondsCmd(const string& name, Receiver * receiver)
    : Command(name, receiver)
{
   init();
}



/**
 * Destructor.
 * Deletes undo related data.
 */
HydrogenBondsCmd::~HydrogenBondsCmd()
{

}



double HydrogenBondsCmd::p_maxDist = 3.5;
double HydrogenBondsCmd::p_maxAngle = 30.0;
int HydrogenBondsCmd::p_lastCount = 0;
bool HydrogenBondsCmd::p_halides = true;

void HydrogenBondsCmd::setHalideAcceptors(bool on) { p_halides = on; }

void HydrogenBondsCmd::setCriteria(double dist, double angle)
{
   if (dist > 0.0) p_maxDist = dist;
   if (angle > 0.0) p_maxAngle = angle;
}
double HydrogenBondsCmd::maxDistance() { return p_maxDist; }
double HydrogenBondsCmd::maxAngle() { return p_maxAngle; }
int HydrogenBondsCmd::lastCount() { return p_lastCount; }
double HydrogenBondsCmd::lastSearchMs() { return p_searchMs; }
double HydrogenBondsCmd::lastDrawMs() { return p_drawMs; }
double HydrogenBondsCmd::p_searchMs = 0.0;
double HydrogenBondsCmd::p_drawMs = 0.0;


void HydrogenBondsCmd::init()
{
   setHelpMessage("HydrogenBondsCmd");
   addParameter(new CommandParameter("switch", true));
}


namespace {

struct V3 {
   double v[3];
   V3() { v[0] = v[1] = v[2] = 0.0; }
   V3(double x, double y, double z) { v[0] = x; v[1] = y; v[2] = z; }
   double x() const { return v[0]; }
   double y() const { return v[1]; }
   double z() const { return v[2]; }
};

// Donors N, O, F, S; acceptors N, O, F, and Cl/Br ions (no covalent bond).
bool donorElement(int z) { return z == 7 || z == 8 || z == 9 || z == 16; }
bool acceptorElement(int z) { return z == 7 || z == 8 || z == 9; }
bool halideElement(int z) { return z == 17 || z == 35; }

// Minimum-image difference vector for the lattice a[0..2] (triclinic safe).
struct MinImage {
   V3 a[3];
   V3 cr[3];      // a_j x a_k / V: fractional coordinate i is d . cr[i]
   bool periodic;
   MinImage(LatticeDef *lat) : periodic(false) {
      if (!lat) return;
      vector<MPoint> *v = lat->toVectors();
      if (v->size() == 3) {
         for (int i = 0; i < 3; i++)
            a[i] = V3((*v)[i].x(), (*v)[i].y(), (*v)[i].z());
         double vol = dot(a[0], cross(a[1], a[2]));
         if (fabs(vol) > 1e-8) {
            cr[0] = scale(cross(a[1], a[2]), 1.0/vol);
            cr[1] = scale(cross(a[2], a[0]), 1.0/vol);
            cr[2] = scale(cross(a[0], a[1]), 1.0/vol);
            periodic = true;
         }
      }
      delete v;
   }
   static double dot(const V3& p, const V3& q) {
      return p.x()*q.x() + p.y()*q.y() + p.z()*q.z();
   }
   static V3 cross(const V3& p, const V3& q) {
      return V3(p.y()*q.z()-p.z()*q.y(), p.z()*q.x()-p.x()*q.z(),
                    p.x()*q.y()-p.y()*q.x());
   }
   static V3 scale(const V3& p, double f) {
      return V3(p.x()*f, p.y()*f, p.z()*f);
   }
   // Shortest of the images of d: round to the nearest cell, then check the
   // 27 neighbours, since a skewed cell's rounding is not always the minimum.
   V3 reduce(const V3& d) const {
      if (!periodic) return d;
      double f[3];
      for (int i = 0; i < 3; i++) f[i] = floor(dot(d, cr[i]) + 0.5);
      V3 base(d.x() - f[0]*a[0].x() - f[1]*a[1].x() - f[2]*a[2].x(),
                  d.y() - f[0]*a[0].y() - f[1]*a[1].y() - f[2]*a[2].y(),
                  d.z() - f[0]*a[0].z() - f[1]*a[1].z() - f[2]*a[2].z());
      V3 best = base;
      double bl = dot(base, base);
      for (int i = -1; i <= 1; i++)
      for (int j = -1; j <= 1; j++)
      for (int k = -1; k <= 1; k++) {
         if (!i && !j && !k) continue;
         V3 t(base.x() + i*a[0].x() + j*a[1].x() + k*a[2].x(),
                  base.y() + i*a[0].y() + j*a[1].y() + k*a[2].y(),
                  base.z() + i*a[0].z() + j*a[1].z() + k*a[2].z());
         double l = dot(t, t);
         if (l < bl) { bl = l; best = t; }
      }
      return best;
   }
};

}


bool HydrogenBondsCmd::execute()
{
   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
   SGFragment *frag = sg->getFragment();

   typedef std::chrono::steady_clock Clock;
   const Clock::time_point t0 = Clock::now();
   double drawSec = 0.0;
   sg->getTopHBonds()->removeAllChildren();
   p_lastCount = 0;
   p_searchMs = p_drawMs = 0.0;

   if (!getParameter("switch")->getBoolean() || !frag || frag->numAtoms() < 2)
      return true;

   vector<TAtm*> *atoms = frag->atoms();
   const int nAtoms = atoms->size();
   const double maxD = p_maxDist;
   const double cosMin = cos(p_maxAngle * M_PI / 180.0);
   MinImage mi(frag->getLattice());

   // 1 = acceptor, 0 = not; computed once so the inner loop has no strings.
   vector<char> acceptor(nAtoms, 0);
   for (int i = 0; i < nAtoms; i++) {
      TAtm *a = (*atoms)[i];
      if (!a->displayStyle().isDisplayed()) continue;
      int z = a->atomicNumber();
      if (acceptorElement(z)) acceptor[i] = 1;
      else if (p_halides && halideElement(z) && a->bondList().empty())
         acceptor[i] = 1;
   }

   // Candidate acceptors near each donor: LinkCellList (cell = cutoff), or,
   // with a lattice, a grid over wrapped fractional coordinates, because
   // LinkCellList knows nothing of periodic images.
   LinkCellList *cellList = 0;
   vector<vector<int> > grid;
   int ng[3] = {1, 1, 1};
   vector<double> frac;
   if (!mi.periodic) {
      cellList = new LinkCellList(atoms, maxD);
   } else {
      for (int i = 0; i < 3; i++) {
         double h = 1.0 / sqrt(MinImage::dot(mi.cr[i], mi.cr[i]));
         ng[i] = std::max(1, (int)floor(h / maxD));
      }
      frac.resize(3*nAtoms);
      grid.resize(ng[0]*ng[1]*ng[2]);
      for (int n = 0; n < nAtoms; n++) {
         const double *c = (*atoms)[n]->coordinates();
         V3 p(c[0], c[1], c[2]);
         int idx[3];
         for (int i = 0; i < 3; i++) {
            double f = MinImage::dot(p, mi.cr[i]);
            f -= floor(f);
            frac[3*n+i] = f;
            idx[i] = std::min(ng[i]-1, (int)(f*ng[i]));
         }
         if (acceptor[n])
            grid[(idx[0]*ng[1]+idx[1])*ng[2]+idx[2]].push_back(n);
      }
   }

   vector<int> cells, cand;
   for (int i = 0; i < nAtoms; i++) {
      TAtm *h = (*atoms)[i];
      if (h->atomicNumber() != 1 || !h->displayStyle().isDisplayed()) continue;

      // The donor: the first single-or-higher bond to N, O, F or S.
      TAtm *donor = 0;
      const vector<TBond*> blist = h->bondList();
      for (size_t j = 0; j < blist.size() && !donor; j++) {
         TAtm *o = blist[j]->atom1() == h ? blist[j]->atom2() : blist[j]->atom1();
         if (blist[j]->order() > 0.99 && donorElement(o->atomicNumber()))
            donor = o;
      }
      if (!donor) continue;
      const int di = donor->index();

      cand.clear();
      if (cellList) {
         cellList->getCells(cells, di);
         const int *header = cellList->getHeader();
         const int *link = cellList->getLinkCellList();
         for (size_t c = 0; c < cells.size(); c++)
            for (int n = header[cells[c]]; n >= 0; n = link[n])
               if (acceptor[n]) cand.push_back(n);
      } else {
         int idx[3];
         for (int k = 0; k < 3; k++)
            idx[k] = std::min(ng[k]-1, (int)(frac[3*di+k]*ng[k]));
         // With fewer than 3 bins on an axis the +-1 neighbours coincide;
         // the cell set below keeps each bin once.
         set<int> seen;
         for (int x = -1; x <= 1; x++)
         for (int y = -1; y <= 1; y++)
         for (int z = -1; z <= 1; z++) {
            int bx = ((idx[0]+x)%ng[0]+ng[0])%ng[0];
            int by = ((idx[1]+y)%ng[1]+ng[1])%ng[1];
            int bz = ((idx[2]+z)%ng[2]+ng[2])%ng[2];
            int b = (bx*ng[1]+by)*ng[2]+bz;
            if (!seen.insert(b).second) continue;
            cand.insert(cand.end(), grid[b].begin(), grid[b].end());
         }
      }

      const double *pd = donor->coordinates();
      const double *ph = h->coordinates();
      V3 dh(ph[0]-pd[0], ph[1]-pd[1], ph[2]-pd[2]);
      double dhLen = sqrt(MinImage::dot(dh, dh));
      if (dhLen < 1e-6) continue;

      for (size_t c = 0; c < cand.size(); c++) {
         TAtm *acc = (*atoms)[cand[c]];
         if (acc == donor || acc == h) continue;
         bool bonded = false;
         const vector<TBond*> ab = acc->bondList();
         for (size_t j = 0; j < ab.size() && !bonded; j++)
            bonded = ab[j]->atom1() == donor || ab[j]->atom2() == donor;
         if (bonded) continue;

         const double *pa = acc->coordinates();
         V3 da = mi.reduce(V3(pa[0]-pd[0], pa[1]-pd[1], pa[2]-pd[2]));
         double len = sqrt(MinImage::dot(da, da));
         if (len > maxD || len < 1e-6) continue;
         if (MinImage::dot(dh, da)/(dhLen*len) < cosMin) continue;

         p_lastCount++;
         const Clock::time_point d0 = Clock::now();
         SoSeparator *sep = new SoSeparator;
         // Line to the acceptor's nearest image, not across the box.
         AtomLine *line = new AtomLine(frag, h, acc, sep);
         line->setShift(pd[0] + da.x() - pa[0], pd[1] + da.y() - pa[1],
                        pd[2] + da.z() - pa[2]);
         sep->addChild(line);
         sg->getTopHBonds()->addChild(sep);
         drawSec += std::chrono::duration<double>(Clock::now() - d0).count();
      }
   }

   delete cellList;
   delete atoms;
   p_drawMs = drawSec * 1000.0;
   p_searchMs = std::chrono::duration<double, std::milli>(Clock::now() - t0).count() - p_drawMs;
   return true;
}



bool HydrogenBondsCmd::isUndoable() const
{
   return false;
}



void HydrogenBondsCmd::undo()
{
}
