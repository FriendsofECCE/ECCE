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

#include "viz/AddSegmentHCmd.H"
#include "viz/SGFragment.H"
#include "viz/SGContainer.H"

#define PI 3.14159265358979323846

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
   setHelpMessage("Add hydrogens to structure based on available fragment or "
                  "segment files");
}

bool AddSegmentHCmd::execute() throw(EcceException)
{
  bool ret = false;

  SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
  SGFragment *frag = sg->getFragment();
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

    frag->touchNumbers();
    sg->touchChemDisplay();

    Event event(ename);
    EventDispatcher::getDispatcher().publish(event);
  }
  return ret;
}
